#include <math.h>
#include <list>
#include <vector>
#include <string>
#include <string.h>
#include <map>
#include <queue>

#include <windows.h>

#include "good/bitset.h"
#include "good/list.h"
#include "good/file.h"
#include "good/ini_file.h"
#include "good/string.h"
#include "good/string_buffer.h"
#include "good/vector.h"
#include "good/map.h"
#include "good/set.h"
#include "good/heap.h"
#include "good/priority_queue.h"
#include "good/graph.h"
#include "good/astar.h"
#include "good/thread.h"
#include "good/process.h"

class MyClass
{
public:
	MyClass(int argg): arg(argg) { printf("(constructor %d)\n", arg); }
	MyClass(const MyClass& other) { arg = other.arg; printf("(copy constructor %d)\n", arg); }
	~MyClass() { printf("(destructor %d)\n", arg); }

	int GetArg() const { return arg; }
	void SetArg(int argg) { arg = argg; }

protected:
	int arg;
	char *asd;
};

//--------------------------------------------------------------------------------------
bool bExit = false;
void process_write_proc(void* param) 
{
	good::process* cProcess = (good::process*)param;
	char* ss = strdup("0123456789\n");
	int ssSize = strlen(ss);
	char j='A';
	while ( !bExit )
	{
		for (int i = 0; i < 10; ++i)
			ss[i] = j;

		if ( !cProcess->write_stdin(ss, ssSize) )
		{
			printf(cProcess->get_last_error());
			break;
		}

		if ( ++j > 'Z' )
			j = 'A';

		Sleep(100);
	}
	cProcess->close_stdin();
	free(ss);
}

void test_process()
{
	printf("%s()\n\n", __FUNCTION__);

	bool bRedirect = true;
	bool bMakeInput = true;
	good::thread cThread;
	cThread.set_func( &process_write_proc );

	good::process cProcess;
	cProcess.set_params("echo.exe", "echo.exe", bRedirect, false);
	//cProcess.set_params("..\\..\\ff\\ff.exe", "..\\..\\ff\\ff.exe -o domain.pddl -f result-problem-83steps.pddl", bRedirect, true);
	for ( int times = 0; times < 2; ++times )
	{
		if ( !cProcess.launch(times%2 == 0) )
		{
			printf(cProcess.get_last_error());
			return;
		}

		if ( bMakeInput )
			cThread.launch(&cProcess, false);

		bExit = false;
		if ( bRedirect )
		{
			char szBuff[64*1024];
			int i=0;
			while ( i < 25 && (!cProcess.is_finished() || cProcess.has_data_stdout() || cProcess.has_data_stderr()) )
			{
				int iRead;
				if ( cProcess.has_data_stdout() )
				{
					if ( !cProcess.read_output(szBuff, 512, iRead) )
					{
						printf(cProcess.get_last_error());
						break;
					}
					szBuff[iRead] = 0;
					printf(szBuff);
				}

				if ( cProcess.has_data_stderr() )
				{
					if ( !cProcess.read_error(szBuff, 512, iRead) )
					{
						printf(cProcess.get_last_error());
						break;
					}
					szBuff[iRead] = 0;
					fprintf(stderr, "StdErr: '%s'\n",szBuff);
				}
				Sleep(200);
				++i;
			}
		}
		bExit = (times%2 == 0);
		
		Sleep(5000);
		printf("Before terminate\n");
		if ( bMakeInput )
			cThread.terminate();
		if ( cProcess.is_finished() )
			printf("  Already finished\n");
		cProcess.terminate();
		printf("After terminate\n");
		Sleep(500);

		cThread.dispose();
		cProcess.dispose();
	}
}

//--------------------------------------------------------------------------------------
void thread_proc(void* param) 
{
	for (int i=0; i<50; ++i)
	{
		printf("Thread %d: %d\n", param, i);
		Sleep(100);
	}
	printf("Thread %d exited\n", param);
}

void test_threads()
{
	printf("%s()\n\n", __FUNCTION__);
	const int threads_count = 10;
	good::thread pThreads[threads_count];
	for (int i=0; i<threads_count; ++i)
	{
		pThreads[i].set_func(&thread_proc);
		pThreads[i].launch((void*)i, false);
	}

	Sleep(3000);
	for (int i=0; i<threads_count-1; ++i)
	{
		pThreads[i].terminate();
		printf("Thread %d terminated\n", i);
	}
	printf("Before join\n");
	pThreads[threads_count-1].join(10000);
	printf("After join\n");
}

//--------------------------------------------------------------------------------------
void test_bitset()
{
	printf("%s()\n\n", __FUNCTION__);
	good::bitset b;
	b.resize(100);
	b.reset();
	for ( int i=0; i<100; ++i )
		if (i % 3 == 0)
			b.set(i);

	for ( int i=0; i<100; ++i )
		if ( b.test(i) != (i % 3 == 0) )
			printf("\nError at position %d\n", i);
		else
			printf("%d", b.test(i));
	printf("\n");
}

//--------------------------------------------------------------------------------------
void test_list()
{
	printf("%s()\n\n", __FUNCTION__);
	typedef good::list<MyClass> list_t;
	//typedef std::list<MyClass> list_t;

	MyClass c(0);

	list_t l;
	
	c.SetArg(1); l.push_back(c);
	c.SetArg(2); l.push_back(c);
	c.SetArg(3); l.push_back(c);
	c.SetArg(0); l.push_front(c);

	for (list_t::iterator it = l.begin(); it != l.end(); ++ it )
	{
		l.insert(it, *it);
	}
	printf("--------------------------------------------\n");
	for (list_t::const_iterator it = l.begin(); it != l.end(); ++ it )
	{
		printf("%d ", it->GetArg());
	}
	printf(" (must be 0 0 1 1 2 2 3 3)\n");

	for (list_t::iterator it = l.begin(); it != l.end(); ++ it )
	{
		it = l.erase(it);
	}
	l.pop_back();
	l.pop_front();

	printf("--------------------------------------------\nFind 2");

	printf("--------------------------------------------\n");
	for (list_t::const_iterator it = l.rbegin(); it != l.rend(); -- it )
	{
		printf("%d ", (*it).GetArg());
	}
	printf(" (must be 2 1)\n");

	l.clear();
	printf("--------------------------------------------\n");
	for (list_t::const_iterator it = l.begin(); it != l.end(); ++ it )
	{
		printf("%d ", (*it).GetArg());
	}
	printf(" (must be empty list)\n");

	l.push_front(c);
	printf("--------------------------------------------\n");
	for (list_t::const_iterator it = l.begin(); it != l.end(); ++ it )
	{
		printf("%d ", (*it).GetArg());
	}
	printf(" (must be 0)\n");

	// this should make an exception.
	//l.erase(l.end());
	
	list_t l1(l);
	list_t l2; l2 = l;
}


//--------------------------------------------------------------------------------------
void test_shared_ptr()
{
	printf("%s()\n\n", __FUNCTION__);

	good::shared_ptr<good::string> a(new good::string("hi there"));
	printf("--------------------------------------------\n%s (must be 'hi there').\n\n", a->c_str());

	good::shared_ptr<good::string> b(a), c;
	printf("--------------------------------------------\n%s (must be 'hi there').\n\n", b->c_str());

	c = a;
	printf("--------------------------------------------\n%s (must be 'hi there').\n\n", c->c_str());

	c.reset();
	printf("--------------------------------------------\n%d (must be NULL).\n\n", c.get());
}

//--------------------------------------------------------------------------------------
void test_file()
{
	printf("%s()\n\n", __FUNCTION__);

	long int size = good::file::file_size("file.h");
	char* buf = (char*)malloc(size+1);
	long int read = good::file::file_to_memory("file.h", buf, size);
	DebugAssert(size == read);
	buf[size] = 0;

	printf( "--------------------------------------------\n%s\n\n", buf );
	free(buf);

	good::string_buffer path = "c:";
	printf("\n");

	good::file::append_path( path, "dir" );
	printf("\n");

	good::file::append_path( path, "filename.ext" );
	printf( "--------------------------------------------\n%s (must be 'c:\\dir\\filename.ext')\n\n", path.c_str() );

	printf( "--------------------------------------------\ndir: %s, '%s' (must be c:\\dir, '')\n", good::file::file_dir(path).c_str(), good::file::file_dir("hello.txt").c_str() );
	printf("\n");

	printf( "--------------------------------------------\nname: %s, '%s' (must be filename.ext, '')\n", good::file::file_name(path).c_str(), good::file::file_name("c:\\wtf\\").c_str() );
	printf("\n");

	printf( "--------------------------------------------\next: %s, '%s' (must be ext, '')\n", good::file::file_ext(path).c_str(), good::file::file_ext("c:\\dir.ext\\file").c_str() );
	printf("\n");
}

//--------------------------------------------------------------------------------------
void test_ini_file()
{
	printf("%s()\n\n", __FUNCTION__);

	for (int i=1; i<10; ++i)
	{
		char buf[32];
		sprintf(buf, "ini_examples\\%d.ini", i);
		good::ini_file ini;
		ini.name = buf;
		ini.load();

		//for (good::ini_file::iterator it = ini.begin(); it != ini.end(); ++it)
		//{
		//	printf("Section %s\n", it->name);
		//	for (good::ini_section::iterator confIt = it->begin(); confIt != it->end(); ++confIt)
		//	{
		//		printf("\tKey '%s', value '%s', junk '%s'\n", confIt->key.c_str(), confIt->value.c_str(), confIt->junk.c_str());
		//	}
		//}

		sprintf(buf, "ini_examples\\%d%d.ini", i, i);
		ini.name = buf;
		ini.save();
	}
}

//--------------------------------------------------------------------------------------
void test_string()
{
	printf("%s()\n\n", __FUNCTION__);

	good::string s1("hello"); // hello, static
	printf( "--------------------------------------------\n%s (must be hello)\n", s1.c_str() );
	
	good::string s2 = s1 + " man"; // hello man, dynamic
	printf( "%s\n", s2.c_str() );

	good::string s3 ;//= good::string::concatenate( s2.c_str(), " man" ); // hello man man, dynamic
	printf( "--------------------------------------------\n%s, starts with %s: %d; ends with man: %d; starts with man: %d, ends with hello: %d; starts with h: %d; ends with \\: %d\n",
			s3.c_str(), s1.c_str(), s3.starts_with(s1), s3.ends_with("man"), s3.starts_with("man"), s3.ends_with(s1),
			s3.starts_with('h'), s3.ends_with('\\')  );

	printf( "--------------------------------------------\n%s != %s: %d; %s == %s: %d\n", s1.c_str(), s3.c_str(), s1 != s3, s1.c_str(), "hello", s1 == "hello" );

	s1 = s3;
	printf("\n");

	good::list<good::string> h;
	h.push_back(s2);
	printf("\n");
	h.push_back(s3);
	printf("\n--------------------------------------------\n");
	for (good::list<good::string>::const_iterator it = h.begin(); it != h.end(); ++it)
		printf("'%s' ", it->c_str());
	printf("(must be 'hello man' '')\n");

	s1 = "\n \r\t\r     Hola  \n\r\t \t   ";
	s2 = s1.duplicate(); s2.trim();
	printf("'%s' trimmed '%s'\n", s1, s2);
}

//--------------------------------------------------------------------------------------
void test_string_buffer()
{
	printf("%s()\n\n", __FUNCTION__);

	good::string_buffer s1("hello"); // hello
	printf ( "--------------------------------------------\n%s (must be hello)\n\n", s1.c_str() );

	good::string_buffer s2;
	s2 = "hello man"; // hello man
	printf ( "--------------------------------------------\n%s (must be hello man)\n\n", s2.c_str() );

	s1.append(" man ");
	printf ( "--------------------------------------------\n%s (must be hello man)\n\n", s1.c_str() );
	
	s1.insert("Boris ", 6);
	printf ( "--------------------------------------------\n%s (must be hello Boris man)\n\n", s1.c_str() );

	s1.append("12345678901234567890123456789012345678901234567890123456789012345678901234567890");
	printf ( "--------------------------------------------\n%s (must be hello Boris man1234...)\n\n", s1.c_str() );

	for (int i=1; i < 1000*1024; i<<=1)
	{
		s1.reserve(i);
		printf ( "%d: %s\n", s1.capacity(), s1.c_str() );
	}
	printf("\n");

	good::list<good::string_buffer> h;
	h.push_back(s2);
	printf("\n");
	h.push_back(s1);
	printf("\n");

	good::string s3=s2; // steal buffer from s2.
	s2 = "hello there";
	printf ( "--------------------------------------------\nstr s3='%s' ('hello man'), sb s2='%s' ('hello there')\n\n", s3.c_str(), s2.c_str() );

	s2.erase(0, 6);
	printf ( "--------------------------------------------\n%s (must be 'there')\n\n", s2.c_str() );

	s2.erase(3, 2);
	printf ( "--------------------------------------------\n%s (must be 'the')\n\n", s2.c_str() );
}

//--------------------------------------------------------------------------------------
void test_vector()
{
	printf("%s()\n\n", __FUNCTION__);

	good::vector<MyClass> v;
	{
		MyClass c(1);

		for ( unsigned int i = 1; i < 33; ++ i )
		{
			c.SetArg(i);
			v.push_back(c);
		}
		c.SetArg(0);
		v.push_front(c);
	}

	printf("--------------------------------------------\n");
	for ( good::vector<MyClass>::const_iterator it = v.rbegin(); it != v.rend(); it-=1 )
		printf ( "%d ", it->GetArg() );
	printf("(must be from 32 to 0)\n\n");

	while ( v.size() > 2 )
		v.pop_front();
	printf("--------------------------------------------\nFirst: %d\n", v[0].GetArg());
	printf("\n");

	v.pop_front(); printf("--------------------------------------------\nFirst: %d\n", v[0].GetArg());
	v.pop_back();  printf("--------------------------------------------\nempty(): %d\n\n", v.empty());


	good::list < good::vector<MyClass> > h;
	h.push_back(v);
	printf("\n");
}

//--------------------------------------------------------------------------------------
void test_map()
{
	printf("%s()\n\n", __FUNCTION__);

	typedef good::map<int, good::string> map_t;
	typedef std::map<int, good::string> std_map_t;

	map_t map;
	std_map_t std_map;

	const int count = 65535;

	char buf[5];
	for (int i=0; i<10; ++i)
	{
		sprintf(buf, "%d", i);
		map.insert(good::pair<int, good::string>(i, good::string(buf, true, true)));
	}

	printf("--------------------------------------------\n");
	for (map_t::const_iterator it = map.begin(); it != map.end(); ++it)
		printf("<%d> ", it->first);
	printf("\n\n");

	map[7] = "hello";
	map[-1] = "goodbye";

	printf("--------------------------------------------\n");
	for (map_t::iterator it = --map.end(); it != map.end(); --it)
		printf(" <%s> ", it->second.c_str());
	printf("\n");

	const good::string& tmp = (/*(const map_t)*/map)[-1];
	printf("goodbye? %s\n", tmp);

	map_t::iterator it = map.find(0);
	while ( it != map.end() )
	{
		printf("--------------------------------------------\nErasing %d\n", it->first);
		it = map.erase(it);
		for (map_t::const_iterator iter = map.begin(); iter != map.end(); ++iter)
			printf(" <%d> ", iter->first);
		printf("--------------------------------------------\nSize: %d\n\n", map.size());
	}

	map.erase(-1);
	printf("--------------------------------------------\nSize: %d\n", map.size());

	map_t other = map;
	other = map;
}

//--------------------------------------------------------------------------------------
void test_set()
{
	printf("%s()\n\n", __FUNCTION__);
	typedef good::set<int> set_t;
	set_t set;

	char buf[5];
	for (int i=0; i<20; ++i)
	{
		sprintf(buf, "%d", i);
		set.insert(i);
	}

	printf("--------------------------------------------\n");
	for (set_t::const_iterator it=set.rbegin(); it != set.rend() ; --it)
		printf("<%d> ", *it);

	printf("\n\n--------------------------------------------\nEmpty:%d, size:%d\n", set.empty(), set.size());
	for (int i=0; i<20; ++i)
		printf("find(%d)=%d\n",  i, *set.find(i));

	set.insert(-1);
	set.erase(10);
	set.insert(20);
	printf("--------------------------------------------\n");
	for (set_t::const_iterator it=set.begin(); it != set.end() ; ++it)
		printf("<%d> ", *it);
	printf(" (from -1 to 20, no element 10)\n");

	set.clear();
	printf("--------------------------------------------\nAfter clear(): empty:%d, size:%d\n", set.empty(), set.size());
	for (set_t::const_iterator it=set.begin(); it != set.end(); ++it)
		printf("This should never appear");
	for (set_t::const_iterator it=set.rbegin(); it != set.rend(); --it)
		printf("This should never appear");

	set_t other = set;
	other = set;
}

//--------------------------------------------------------------------------------------
void test_heap()
{
	printf("%s()\n\n", __FUNCTION__);

	//        1             1             1*            6             6
	//      /   \          /  \          /  \          /  \          /  \
	//     2     3*  =>   2*   6   =>   5    6   =>   5    1*  =>   5    3
	//    / \   /        / \  /        / \  /        / \  /        / \  /
	//   4  5  6        4  5 3        4  2 3        4  2 3        4  2 1
	int size = 6;
	good::string a[7] = {"1", "2", "3", "4", "5", "6",   "-1"};
	good::heap_make(a, size);
	for (int i=0; i<size; ++i)
		printf("%s ", a[i].c_str());
	printf("\n");

	//        6             6             7
	//      /   \          /  \          /  \
	//     5     3   =>   5    7*  =>   5    6
	//    / \   / \      / \  / \      / \  / \
	//   4  2  1   7*   4  2 1   3    4  2 1   3
	good::heap_push(a, good::string("7"), size++);
	for (int i=0; i<size; ++i)
		printf("%s ", a[i].c_str());
	printf("\n");

	good::heap_sort(a, size);
	for (int i=0; i<size; ++i)
		printf("%s ", a[i].c_str());
	printf("\n");

	good::priority_queue<good::string> queue;
	queue.push("0");
	queue.push("1");
	queue.push("2");
	queue.push("3");
	queue.push("4");
	while (!queue.empty())
	{
		printf("%s ", queue.top().c_str());
		queue.pop();
	}
	printf("\n");
}


//--------------------------------------------------------------------------------------
void test_graph()
{
	#define sqr(x) ((x)*(x))
	typedef good::pair<float, float> vertex_t;
	typedef good::graph< vertex_t, float > graph_t;

	class dist
	{
	public:
		float operator()(vertex_t const& left, vertex_t const& right)
		{
			float res = sqrt( sqr(right.first - left.first) + sqr(right.second - left.second) );
			printf("%f %f ----%f----> %f %f\n", left.first, left.second, res, right.first, right.second);
			return res;
		}
	};

	class edge_length
	{
	public:
		float operator()(float f) const
		{
			return f;
		}
	};

	class can_use
	{
	public:
		bool operator()(graph_t::node_t const& node) const
		{
			return true;
		}
	};

	typedef good::astar< vertex_t, float, float, dist, edge_length, can_use > astar_t;

	//  "0 2"  ->  "2 2"
	//   |2    2     |2.2
	//   v     1     v
	//  "0 0"  ->  "1 0"
	graph_t g;
	graph_t::node_it nA = g.add_node(vertex_t(0,2));
	graph_t::node_it nB = g.add_node(vertex_t(2,2));
	graph_t::node_it nC = g.add_node(vertex_t(0,0));
	graph_t::node_it nD = g.add_node(vertex_t(1,0));
	g.add_arc(nA, nB, 2);
	g.add_arc(nA, nC, 2);
	g.add_arc(nB, nD, 2.2f);
	g.add_arc(nC, nD, 1);

	astar_t a;
	a.set_graph(g);
	//g.delete_arc(nA, nB);
	//g.delete_node(nD);

	for ( graph_t::const_node_it nodeIt = g.begin(); nodeIt != g.end(); ++nodeIt )
	{
		printf("Node: %f %f\n", nodeIt->vertex.first, nodeIt->vertex.second);
		for ( graph_t::const_arc_it arcIt = nodeIt->neighbours.rbegin(); arcIt != nodeIt->neighbours.rend(); arcIt-- )
		{
			printf("  %d ---%f---> %d\n", nodeIt - g.begin(), arcIt->edge, arcIt->target);
		}
	}
	
	a.setup_search(0, 3, can_use());
	bool b = a.step();
	printf("\nFinished: %d, found path A->D: %d\n", b, a.has_path());
	for (astar_t::path_t::const_iterator it = a.path().begin(); it != a.path().end(); ++it )
		printf("%d -> ", *it);
	printf("(must be 0 -> 2 -> 3)\n", b, a.has_path());

	a.setup_search(3, 0, can_use());
	b = a.step();
	printf("Finished: %d, found path D->A: %d\n", b, a.has_path());

}


//--------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	test_process();
	system("pause");
	system("cls");
	
	//test_threads();
	//system("pause");
	//system("cls");
	//
	//test_bitset();
	//system("pause");
	//system("cls");
	//
	//test_list();
	//system("pause");
	//system("cls");
	//
	//test_shared_ptr();
	//system("pause");
	//system("cls");
	//
	//test_file();
	//system("pause");
	//system("cls");
	//
	//test_ini_file();
	//system("pause");
	//system("cls");
	//
	//test_string();
	//system("pause");
	//system("cls");
	//
	//test_string_buffer();
	//system("pause");
	//system("cls");
	//
	//test_vector();
	//system("pause");
	//system("cls");
	//
	//test_map();
	//system("pause");
	//system("cls");

	//test_set();
	//system("pause");
	//system("cls");

	//test_heap();
	//system("pause");
	//system("cls");

	//test_graph();
	//system("pause");
	//system("cls");

	return 0;
}
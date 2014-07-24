#define DEBUG_VECTOR_PRINT
//#define DEBUG_STRING_PRINT
#include <math.h>
#include <string.h>
#include <memory>
#include <map>
#include <queue>
#include <string>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #define Sleep(x) usleep(x*1000)
#endif

#include <good/bitset.h>
#include <good/file.h>
#include <good/ini_file.h>
#include <good/map.h>
#include <good/set.h>
#include <good/string.h>
#include <good/heap.h>
#include <good/priority_queue.h>
#include <good/graph.h>
#include <good/astar.h>
#include <good/process.h>
#include <good/memory.h>
#include "good/list.h"
#include "good/vector.h"

class MyClass
{
public:
    MyClass(int argg): arg(argg) { ReleasePrint("(MyClass constructor %d)\n", arg); }
    MyClass(const MyClass& other) { arg = other.arg; ReleasePrint("(MyClass copy constructor %d)\n", arg); }
    ~MyClass() { ReleasePrint("(MyClass destructor %d)\n", arg); }

    int GetArg() const { return arg; }
    void SetArg(int argg) { arg = argg; }

protected:
    int arg;
    char *asd;
};

//--------------------------------------------------------------------------------------
#ifdef _WIN32
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
            ReleasePrint(cProcess->get_last_error());
            break;
        }

        if ( ++j > 'Z' )
            j = 'A';

        Sleep(100);
    }
    cProcess->close_stdin();
    free(ss);
}
#endif // _WIN32

void test_process()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);

#ifdef _WIN32
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
            ReleasePrint(cProcess.get_last_error());
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
                    if ( !cProcess.read_stdout(szBuff, 512, iRead) )
                    {
                        ReleasePrint(cProcess.get_last_error());
                        break;
                    }
                    szBuff[iRead] = 0;
                    ReleasePrint(szBuff);
                }

                if ( cProcess.has_data_stderr() )
                {
                    if ( !cProcess.read_stderr(szBuff, 512, iRead) )
                    {
                        ReleasePrint(cProcess.get_last_error());
                        break;
                    }
                    szBuff[iRead] = 0;
                    fReleasePrint(stderr, "StdErr: '%s'\n",szBuff);
                }
                Sleep(200);
                ++i;
            }
        }
        bExit = (times%2 == 0);

        Sleep(5000);
        ReleasePrint("Before terminate\n");
        if ( bMakeInput )
            cThread.terminate();
        if ( cProcess.is_finished() )
            ReleasePrint("  Already finished\n");
        cProcess.terminate();
        ReleasePrint("After terminate\n");
        Sleep(500);

        cThread.dispose();
        cProcess.dispose();
    }
#endif // _WIN32
}

//--------------------------------------------------------------------------------------
void thread_proc(void* param)
{
    for (int i=0; i<50; ++i)
    {
        ReleasePrint("Thread %p: %d\n", param, i);
        Sleep(100);
    }
    ReleasePrint("Thread %p exited\n", param);
}

void test_threads()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);

#ifdef _WIN32
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
        ReleasePrint("Thread %d terminated\n", i);
    }
    ReleasePrint("Before join\n");
    pThreads[threads_count-1].join(10000);
    ReleasePrint("After join\n");
#endif // _WIN32
}

//--------------------------------------------------------------------------------------
void test_bitset()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);
    good::bitset b;
    b.resize(100);
    b.reset();
    for ( int i=0; i<100; ++i )
        if (i % 3 == 0)
            b.set(i);

    for ( int i=0; i<100; ++i )
    {
        DebugAssert( b.test(i) == (i % 3 == 0) );
        ReleasePrint("%d", b.test(i));
    }
    ReleasePrint("\n");
}

//--------------------------------------------------------------------------------------
void test_list()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);

    typedef good::list<MyClass> list_t;
    //typedef good::list<MyClass> list_t;

    MyClass c(0);

    list_t l;

    c.SetArg(1); l.push_back(c);
    c.SetArg(2); l.push_back(c);
    c.SetArg(3); l.push_back(c);
    c.SetArg(0); l.push_front(c);

    for (list_t::iterator it = l.begin(); it != l.end(); ++it )
        l.insert(it, *it);

    int aValues1[] = { 0, 0, 1, 1, 2, 2, 3, 3 };
    int idx=0;
    for (list_t::const_iterator it = l.begin(); it != l.end(); ++it, ++idx )
        DebugAssert(it->GetArg() == aValues1[idx]);

    for (list_t::iterator it = l.begin(); it != l.end(); ++it )
        it = l.erase(it);

    l.pop_back();
    l.pop_front();

    int aValues2[] = { 1, 2 };
    idx=0;
    for (list_t::const_iterator it = l.begin(); it != l.end(); ++it, ++idx )
        DebugAssert(it->GetArg() == aValues2[idx]);

    l.clear();
    for (list_t::const_iterator it = l.begin(); it != l.end(); ++it )
        DebugAssert(false);

    DebugAssert( l.empty() );

    l.push_front(c);
    DebugAssert( l.begin()->GetArg() == c.GetArg() );

    // this should make an exception.
    //l.erase(l.end());

    list_t l1(l);
    DebugAssert( l.empty() );
    DebugAssert( l1.size() == 1 );
    DebugAssert( l1.begin()->GetArg() == c.GetArg() );

    list_t l2; l2 = l1;
    DebugAssert( l1.empty() );
    DebugAssert( l2.size() == 1 );
    DebugAssert( l2.begin()->GetArg() == c.GetArg() );
}


//--------------------------------------------------------------------------------------
void test_shared_ptr()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);

    const char* szStr = "hi there";
    good::shared_ptr<good::string> a(new good::string(szStr));
    DebugAssert(*a == szStr);

    good::shared_ptr<good::string> b(a), c;
    DebugAssert(*b == szStr);

    c = a;
    DebugAssert(*c == szStr);

    c.reset();
    DebugAssert(c.get() == NULL);
}

//--------------------------------------------------------------------------------------
void test_file()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);

#ifdef _WIN32
    const char* szFile = "file.h";
#else
    const char* szFile = "/usr/include/string.h";
#endif
    long int size = good::file::file_size(szFile);
    char* buf = (char*)malloc(size+1);
    long int read = good::file::file_to_memory(szFile, buf, size);
    DebugAssert(size == read);
    buf[size] = 0;

    ReleasePrint( "--------------------------------------------\n%s\n\n", buf );
    free(buf);

    good::string path("c:");
    path = good::file::append_path( path, "dir" PATH_SEPARATOR_STRING );
    path = good::file::append_path( path, "filename.ext" );

    DebugAssert(path == "c:" PATH_SEPARATOR_STRING "dir" PATH_SEPARATOR_STRING "filename.ext");
    DebugAssert(good::file::dir(path) == "c:" PATH_SEPARATOR_STRING "dir");
    DebugAssert(good::file::fname(path) == "filename.ext");
    DebugAssert(good::file::ext(path) == "ext");

    DebugAssert(good::file::dir(good::string("hello.txt")) == "");
    DebugAssert(good::file::fname(good::string("c:" PATH_SEPARATOR_STRING "dir" PATH_SEPARATOR_STRING)) == "");
    DebugAssert(good::file::ext(good::string("c:" PATH_SEPARATOR_STRING "dir" PATH_SEPARATOR_STRING "filename")) == "");
}

//--------------------------------------------------------------------------------------
void test_ini_file()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);

    for (int i=1; i<10; ++i)
    {
        char buf[32];
#ifndef _WIN32
#   define LINUX_SRC_DIR "../"
#endif
        sprintf(buf, LINUX_SRC_DIR "ini_examples" PATH_SEPARATOR_STRING "%d.ini", i);
        good::ini_file ini;
        ini.name = buf;

        good::TIniFileError iError = ini.load();
        DebugAssert(iError <= good::IniFileBadSyntax);

//#define COMPARE_INI_FILES
#ifdef COMPARE_INI_FILES
        static char szFile1[MAX_INI_FILE_SIZE], szFile2[MAX_INI_FILE_SIZE];
        if (iError == good::IniFileNoError)
            DebugAssert(good::file::file_to_memory(ini.name.c_str(), szFile1, MAX_INI_FILE_SIZE, 0) > 0);
#endif

        //for (good::ini_file::iterator it = ini.begin(); it != ini.end(); ++it)
        //{
        //	ReleasePrint("Section %s\n", it->name);
        //	for (good::ini_section::iterator confIt = it->begin(); confIt != it->end(); ++confIt)
        //	{
        //		ReleasePrint("\tKey '%s', value '%s', junk '%s'\n", confIt->key.c_str(), confIt->value.c_str(), confIt->junk.c_str());
        //	}
        //}

#ifdef _WIN32
        sprintf(buf, LINUX_SRC_DIR "ini_examples" PATH_SEPARATOR_STRING "%d%d.ini", i, i);
#else
        sprintf(buf, "%d.ini", i);
#endif
        ini.name = buf;
        ini.save();

#ifdef COMPARE_INI_FILES
        if (iError == good::IniFileNoError)
        {
            DebugAssert(good::file::file_to_memory(ini.name.c_str(), szFile2, MAX_INI_FILE_SIZE, 0) > 0);

            good::string sFile1(szFile1), sFile2(szFile2);
            DebugAssert(sFile1 == sFile2);
        }
#endif
        ReleasePrint("\n");
    }
}

//--------------------------------------------------------------------------------------
void test_string()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);

    good::string s1("hello"); // hello, static
    DebugAssert(s1 == "hello" && s1 != "HELLO" );

    good::string s2 = s1 + " man"; // hello man, dynamic
    DebugAssert(s2 == "hello man");

    good::string s3 = good::string::concatenate( s2.c_str(), " man" ); // hello man man, dynamic
    DebugAssert(s3 == "hello man man");

    DebugAssert( good::starts_with(s1, 'h') );
    DebugAssert( good::ends_with(s1, 'o') );

    DebugAssert( good::starts_with(s2, s1) );

    DebugAssert( good::starts_with(s3, s2) );
    DebugAssert( good::ends_with(s3, "man") );

    DebugAssert( !good::starts_with(s1, "man") );
    DebugAssert( !good::ends_with(s1, '\\') );

    s1 = s3; // s1 = "hello man man"; s3 = "";

    good::list<good::string> h;
    h.push_back(s2);
    h.push_back(s3);

    DebugAssert(*h.begin() == "hello man");
    DebugAssert(*(++h.begin()) == "");

    s1 = "\n \r\t\r     Hola  \n\r\t \t   ";
    s2 = s1.duplicate();
    DebugAssert(s1 == s2);

    good::trim(s2);
    DebugAssert(s2 == "Hola");
}

//--------------------------------------------------------------------------------------
void test_string_buffer()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);

    good::string_buffer s1("hello"); // hello
    DebugAssert(s1 == "hello");

    good::string_buffer s2;
    s2 = "hello man"; // hello man
    DebugAssert(s2 == "hello man");

    s1.append(" man");
    DebugAssert(s1 == "hello man");

    s1.insert("Boris ", 6);
    DebugAssert(s1 == "hello Boris man");

    const char* szAppend = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
    s1.append(szAppend);
    DebugAssert(s1 == good::string("hello Boris man") + szAppend);

    for (int i=1; i < 1000*1024; i<<=1)
    {
        s1.reserve(i);
        DebugAssert(s1.capacity() >= i);
    }

    good::list<good::string_buffer> h;
    h.push_back(s2);
    DebugAssert(s2 == "hello man");
    h.push_back(s1);
    DebugAssert(s1 == good::string("hello Boris man") + szAppend);

    good::string s3=s2.duplicate();
    DebugAssert(s3 == "hello man");

    s2 = "hello there";
    DebugAssert(s2 == "hello there");

    s2.erase(0, 6);
    DebugAssert(s2 == "there");

    s2.erase(3, 2);
    DebugAssert(s2 == "the");
}

//--------------------------------------------------------------------------------------
void test_vector()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);

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

    ReleasePrint("--------------------------------------------\n");
    int idx=32;
    for ( good::vector<MyClass>::const_iterator it = v.rbegin(); it != v.rend(); it-=1,--idx )
        DebugAssert( it->GetArg() == idx );

    while ( v.size() > 2 )
        v.pop_front();
    DebugAssert(v[0].GetArg() == 31);
    DebugAssert(v[1].GetArg() == 32);

    v.pop_front();
    DebugAssert(v[0].GetArg() == 32);

    v.pop_back();
    DebugAssert(v.empty() && v.size() == 0);

    good::list < good::vector<MyClass> > h;
    h.push_back(v);
    DebugAssert(v.empty() && v.size() == 0);
}

//--------------------------------------------------------------------------------------
void test_map()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);

    typedef good::map<int, good::string> map_t;
    //typedef good::map<int, good::string> std_map_t;

    map_t map;
    //std_map_t std_map;

    const int iCount = 10;

    char buf[5];
    for (int i=0; i<iCount; ++i)
    {
        sprintf(buf, "%d", i);
        map.insert(good::pair<int, good::string>(i, good::string(buf, true, true)));
    }

    int idx=0;
    for (map_t::const_iterator it = map.begin(); it != map.end(); ++it, ++idx)
        DebugAssert(it->first == idx);

    map[7] = "hello";
    map[-1] = "goodbye";

    idx=iCount-1;
    for (map_t::iterator it = --map.end(); it != map.end(); --it, --idx)
    {
        DebugAssert(it->first == idx);
        if (idx == -1)
            DebugAssert(it->second == "goodbye");
        else  if (idx == 7)
            DebugAssert(it->second == "hello");
    }

    const good::string& tmp = (/*(const map_t)*/map)[-1];
    DebugAssert(tmp == "goodbye");

    map_t::iterator it = map.find(0);
    while ( it != map.end() )
    {
        ReleasePrint("Erasing %d\n", it->first);
        it = map.erase(it);
        for (map_t::const_iterator iter = map.begin(); iter != map.end(); ++iter)
            ReleasePrint(" <%d> ", iter->first);
        ReleasePrint("--------------------------------------------\nSize: %d\n\n", map.size());
    }

    DebugAssert(map.size()==1);

    map_t other = map;
    other = map;
    DebugAssert( (map.size() == 0) && map.empty() );

    other.erase(-1);
    DebugAssert( (other.size() == 0) && other.empty() );
}

//--------------------------------------------------------------------------------------
void test_set()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);
    typedef good::set<int> set_t;
    set_t set;

    const int iCount = 20;
    for (int i=0; i<iCount; ++i)
        set.insert(i);

    int idx=iCount-1;
    for (set_t::const_iterator it=set.rbegin(); it != set.rend() ; --it,--idx)
        DebugAssert( *it == idx );

    DebugAssert( !set.empty() );
    DebugAssert( set.size() == iCount );

    for (int i=0; i<iCount; ++i)
        DebugAssert( *set.find(i) == i );

    set.insert(-1);
    set.erase(10);
    set.insert(iCount);
    idx=-1;
    for (set_t::const_iterator it=set.begin(); it != set.end() ; ++it,++idx)
    {
        if (idx == 10)
            ++idx;
        DebugAssert(*it==idx);
    }

    set_t other = set;
    DebugAssert(set.size()==0 && other.size() == iCount+1);

    other.clear();
    DebugAssert( other.empty() );
    DebugAssert( other.size() == 0 );

    for (set_t::const_iterator it=set.begin(); it != set.end(); ++it)
        DebugAssert(false);
    for (set_t::const_iterator it=set.rbegin(); it != set.rend(); --it)
        DebugAssert(false);

    other = set;
    DebugAssert( other.empty() );
    DebugAssert( other.size() == 0 );
}

//--------------------------------------------------------------------------------------
void test_heap()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);

    //        1             1             1*            6             6
    //      (   )          (  )          (  )          (  )          (  )
    //     2     3*  =>   2*   6   =>   5    6   =>   5    1*  =>   5    3
    //    ( )   (        ( )  (        ( )  (        ( )  (        ( )  (
    //   4  5  6        4  5 3        4  2 3        4  2 3        4  2 1
    int size = 6;
    good::string a[7] = {"1", "2", "3", "4", "5", "6",   "-1"};
    good::heap_make(a, size);
    for (int i=0; i<size; ++i)
        ReleasePrint("%s ", a[i].c_str());
    ReleasePrint("\n");

    //        6             6             7
    //      (   )          (  )          (  )
    //     5     3   =>   5    7*  =>   5    6
    //    ( )   ( )      ( )  ( )      ( )  ( )
    //   4  2  1   7*   4  2 1   3    4  2 1   3
    good::heap_push(a, good::string("7"), size++);
    for (int i=0; i<size; ++i)
        ReleasePrint("%s ", a[i].c_str());
    ReleasePrint("\n");

    good::heap_sort(a, size);
    for (int i=0; i<size; ++i)
        ReleasePrint("%s ", a[i].c_str());
    ReleasePrint("\n");

    good::priority_queue<good::string> queue;
    queue.push("0");
    queue.push("1");
    queue.push("2");
    queue.push("3");
    queue.push("4");
    while (!queue.empty())
    {
        ReleasePrint("%s ", queue.top().c_str());
        queue.pop();
    }
    ReleasePrint("\n");
}


//--------------------------------------------------------------------------------------
void test_graph()
{
    ReleasePrint("%s()\n\n", __FUNCTION__);

    #define sqr(x) ((x)*(x))
    typedef good::pair<float, float> vertex_t;
    typedef good::graph< vertex_t, float > graph_t;

    class dist
    {
    public:
        float operator()(vertex_t const& left, vertex_t const& right)
        {
            float res = sqrt( sqr(right.first - left.first) + sqr(right.second - left.second) );
            ReleasePrint("%f %f ----%f----> %f %f\n", left.first, left.second, res, right.first, right.second);
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
        bool operator()(graph_t::node_t const& /*node*/) const
        {
            return true;
        }
    };

    typedef good::astar< vertex_t, float, float, dist, edge_length, can_use > astar_t;

    //  "0 2"  ->  "2 2"
    //   |2    2     |2.2
    //   v     1     v
    //  "0 0"  ->  "1 0"
    graph_t g(4);
    graph_t::node_it nA = g.add_node(vertex_t(0,2),2);
    graph_t::node_it nB = g.add_node(vertex_t(2,2),1);
    graph_t::node_it nC = g.add_node(vertex_t(0,0),1);
    graph_t::node_it nD = g.add_node(vertex_t(1,0),0);
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
        ReleasePrint("Node: %f %f\n", nodeIt->vertex.first, nodeIt->vertex.second);
        for ( graph_t::const_reverse_arc_it arcIt = nodeIt->neighbours.rbegin(); arcIt != nodeIt->neighbours.rend(); arcIt++ )
        {
            ReleasePrint("  %d ---%f---> %d\n", nodeIt - g.begin(), arcIt->edge, arcIt->target);
        }
    }

    a.setup_search(0, 3, can_use());
    bool b = a.step();
    ReleasePrint("\nFinished: %d, found path A->D: %d\n", b, a.has_path());
    for (astar_t::path_t::const_iterator it = a.path().begin(); it != a.path().end(); ++it )
        ReleasePrint("%d -> ", *it);
    ReleasePrint("(must be 0 -> 2 -> 3)\n");
    DebugAssert( b && a.has_path() );

    a.setup_search(3, 0, can_use());
    b = a.step();
    ReleasePrint("Finished: %d, found path D->A: %d\n", b, a.has_path());
    DebugAssert( b && !a.has_path() );
}

//--------------------------------------------------------------------------------------
void stop()
{
#ifdef _WIN32
    system("pause");
    system("cls");
#else
    //system("bash -c 'read -p \"Press ENTER to continue...\" -n 1'");
    //system("clear");
    fflush(stdout);
    //sleep(5);
#endif
}

//--------------------------------------------------------------------------------------
int main(int, char**)
{
    test_process();
    stop();

    test_threads();
    stop();

    test_bitset();
    stop();

    test_list();
    stop();

    test_shared_ptr();
    stop();

    test_file();
    stop();

    test_ini_file();
    stop();

    test_string();
    stop();

    test_string_buffer();
    stop();

    test_vector();
    stop();

    test_map();
    stop();

    test_set();
    stop();

    test_heap();
    stop();

    test_graph();
    stop();

    return 0;
}

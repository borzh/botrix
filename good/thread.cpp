#ifdef WIN32

#include <windows.h>

#include "good/thread.h"

namespace good
{

	DWORD WINAPI thread_impl_thread_proc(LPVOID lpThreadParameter); // Forward declaration.

	//----------------------------------------------------------------------------------------------------------------
	// Thread implementation.
	//----------------------------------------------------------------------------------------------------------------
	class thread_impl
	{
	public:
		thread_impl(good::thread::thread_func_t thread_func): m_hThread(NULL), m_pThreadFunc(thread_func) {}
		~thread_impl()
		{
			dispose();
		}

		/// Set function.
		inline void set_func( thread::thread_func_t thread_func ) { m_pThreadFunc = thread_func; }

		// Execute thread. 
		inline void launch( void* pThreadParameter, bool bDaemon = false )
		{
			DebugAssert( m_hThread == NULL );
			m_bDaemon = bDaemon;
			m_pThreadParameter = pThreadParameter;
			m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(&thread_impl_thread_proc), (LPVOID)this, 0, NULL);
		}

		/// Free all handles and memory. Terminate thread if not daemon.
		inline void dispose()
		{
			if ( m_hThread )
			{
				if ( !m_bDaemon && !is_finished() )
					TerminateThread(m_hThread, 2);
				CloseHandle(m_hThread);
				m_hThread = NULL;
			}
		}

		// Wait for this thread.  Return true if thread is terminated.
		inline bool join( int iMSecs )
		{
			DebugAssert( m_hThread );
			return WaitForSingleObject(m_hThread, iMSecs) == WAIT_OBJECT_0;
		}

		// Terminate thread.
		inline void terminate()
		{
			DebugAssert( m_hThread );
			TerminateThread(m_hThread, 1);
		}

		// Check if thread was launched previously.
		inline bool is_launched() { return m_hThread != NULL; }

		// Check if thread is finished.
		inline bool is_finished()
		{
			DebugAssert( m_hThread );
			DWORD iExitCode = 0;
			GetExitCodeThread(m_hThread, &iExitCode); // TODO: Handle error.
			return iExitCode != STILL_ACTIVE;
		}

	protected:
		friend DWORD WINAPI thread_impl_thread_proc(LPVOID lpThreadParameter);

		bool m_bDaemon;
		HANDLE m_hThread;
		good::thread::thread_func_t m_pThreadFunc;
		LPVOID m_pThreadParameter;
	};


	//----------------------------------------------------------------------------------------------------------------
	// Thread functions.
	//----------------------------------------------------------------------------------------------------------------
	void thread::sleep( int iMSecs )
	{
		Sleep(iMSecs);
	}

	//----------------------------------------------------------------------------------------------------------------
	void thread::exit( int iExitCode )
	{
		ExitThread(iExitCode);
	}

	//----------------------------------------------------------------------------------------------------------------
	thread::thread()
	{
		m_pImpl = new thread_impl( NULL );
	}

	thread::thread( thread_func_t thread_func )
	{
		m_pImpl = new thread_impl( thread_func );
	}

	//----------------------------------------------------------------------------------------------------------------
	thread::~thread()
	{
		delete (thread_impl*)m_pImpl;
	}

	//----------------------------------------------------------------------------------------------------------------
	void thread::set_func( thread_func_t thread_func )
	{
		((thread_impl*)m_pImpl)->set_func(thread_func);
	}

	//----------------------------------------------------------------------------------------------------------------
	void thread::launch( void* pThreadParameter, bool bDaemon )
	{
		((thread_impl*)m_pImpl)->launch(pThreadParameter, bDaemon);
	}

	//----------------------------------------------------------------------------------------------------------------
	void thread::terminate()
	{
		((thread_impl*)m_pImpl)->terminate();
	}

	//----------------------------------------------------------------------------------------------------------------
	void thread::dispose()
	{
		((thread_impl*)m_pImpl)->dispose();
	}

	//----------------------------------------------------------------------------------------------------------------
	bool thread::join( int iMSecs )
	{
		return ((thread_impl*)m_pImpl)->join(iMSecs);
	}

	//----------------------------------------------------------------------------------------------------------------
	bool thread::is_launched()
	{
		return ((thread_impl*)m_pImpl)->is_launched();
	}

	//----------------------------------------------------------------------------------------------------------------
	bool thread::is_finished()
	{
		return ((thread_impl*)m_pImpl)->is_finished();
	}


	//----------------------------------------------------------------------------------------------------------------
	// Thread function.
	//----------------------------------------------------------------------------------------------------------------
	DWORD WINAPI thread_impl_thread_proc(LPVOID lpThreadParameter)
	{
		good::thread_impl* pImpl = (good::thread_impl*)lpThreadParameter;
		DebugAssert( pImpl->m_pThreadFunc );
		pImpl->m_pThreadFunc(pImpl->m_pThreadParameter);
		ExitThread(0);
		//return 0;
	}

} // namespace good

#endif // WIN32

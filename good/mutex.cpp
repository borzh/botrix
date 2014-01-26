#include "good/defines.h"
#include "good/mutex.h"

#ifdef _WIN32
	#include "windows.h"
#endif

namespace good
{

#ifdef _WIN32
	mutex::mutex()
	{
		m_hMutex = CreateMutex(NULL, FALSE, NULL); // Default security attibutes, initially not owned, mutex not named.
		DebugAssert(m_hMutex);
	}

	mutex::~mutex()
	{
		CloseHandle(m_hMutex);
	}

	void mutex::lock()
	{
		WaitForSingleObject(m_hMutex, INFINITE);
	}

	bool mutex::try_lock()
	{
		DWORD iResult = WaitForSingleObject(m_hMutex, 0);
		return (iResult == WAIT_ABANDONED) || (iResult == WAIT_OBJECT_0);
	}

	void mutex::unlock()
	{
		ReleaseMutex(m_hMutex);
	}

#else
#	error Mutex not implemented
#endif // _WIN32

} // namespace good
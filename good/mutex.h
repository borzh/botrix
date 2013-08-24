#ifndef __GOOD_MUTEX_H__
#define __GOOD_MUTEX_H__


namespace good
{
	
	//************************************************************************************************************
	/// Mutex class.
	//************************************************************************************************************
	class mutex
	{
	public:
		/// Constructor.
		mutex();
		
		/// Destructor.
		virtual ~mutex();
		
		/// Lock mutex.
		void lock();
		
		/// Try to lock mutex, true when could lock.
		bool try_lock();
		
		/// Unlock mutex.
		void unlock();

	protected:
		void *m_hMutex; // Mutex implementation.
	};

} // namespace good


#endif // __GOOD_MUTEX_H__

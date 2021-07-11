
class ThreadLocker
{
public:
  ThreadLocker(pthread_mutex_t * mutex) { {
    Init(mutex);
  }
  ~ThreadLocker() {
    pthread_mutex_unlock(m_mutex);
  }

  void Init(pthread_mutex_t* mutex) {
    m_mutex = mutex;
    if (pthread_mutex_lock(mutex))
      error_message("Error locking mutex");
  }
  pthread_mutex_t* m_mutex;

};


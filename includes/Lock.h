#ifndef LOCK_H
#define LOCK_H

#include <pthread.h>
#include <uv.h>

class Lock {
public:
  Lock(pthread_mutex_t &mutex);
  ~Lock();
private:
  pthread_mutex_t &mMutex;
};

class UVLock {
public:
  UVLock(uv_mutex_t &mutex);
  ~UVLock();

private:
  uv_mutex_t &mMutex;
};

#endif

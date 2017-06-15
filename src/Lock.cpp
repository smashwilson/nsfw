#include "../includes/Lock.h"

Lock::Lock(pthread_mutex_t &mutex)
  : mMutex(mutex) {
  pthread_mutex_lock(&mMutex);
}

Lock::~Lock() {
  pthread_mutex_unlock(&mMutex);
}

UVLock::UVLock(uv_mutex_t &mutex)
  : mMutex(mutex) {
  uv_mutex_lock(&mMutex);
}

UVLock::~UVLock() {
  uv_mutex_unlock(&mMutex);
}

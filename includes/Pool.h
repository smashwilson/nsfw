#ifndef POOL_H
#define POOL_H

#include <set>
#include <uv.h>
#include "NativeInterface.h"

class NSFW;

class Pool {
public:
  Pool();
  ~Pool();

  void add(NSFW* nsfw);
  void remove(NSFW* nsfw);
private:
  Pool(const Pool&) {}
  Pool& operator=(const Pool&) {}

  void acceptIntake();
  void poll();
  void runLoop();

  NativeInterface mNativeInterface;

  uv_mutex_t mIntakeMutex;
  set::set<NSFW*> *mToAdd;
  set::set<NSFW*> *mToRemove;

  std::set<NSFW*> mNSFWs;

  uv_thread_t mPollThread;
  uv_thread_t mRunLoopThread;

  bool mRunning;
};

#endif

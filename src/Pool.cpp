#include <set>
#include <functional>
#include <thread>
#include <chrono>
#include <uv.h>

#include "../includes/Pool.h"
#include "../includes/NSFW.h"
#include "../includes/Lock.h"

Pool::Pool() :
  mToAdd(NULL),
  mToRemove(NULL),
  mRunning(true)
{
  uv_rwlock_init(mIntakeMutex);

  uv_thread_create(&mPollThread, mem_fn(&Pool::poll), this);
  uv_thread_create(&mRunLoopThread, mem_fn(&Pool::runLoop), this);
}

Pool::~Pool() {
  uv_mutex_destroy(mIntakeMutex);
}

void Pool::add(NSFW* nsfw) {
  UVLock lock(mIntakeMutex);
  if (mToAdd == NULL) {
    mToAdd = new std::set<NSFW*>;
  }

  mToAdd->insert(nsfw);
}

void Pool::remove(NSFW* nsfw) {
  UVLock lock(mIntakeMutex);
  if (mToRemove == NULL) {
    mToRemove = new std::set<NSFW*>;
  }

  mToRemove->insert(nsfw);
}

void Pool::acceptIntake() {
  UVLock lock(mIntakeMutex);

  if (mToAdd !== NULL) {
    mNSFWs.insert(mToAdd.begin(), mToAdd.end());
    delete mToAdd;
    mToAdd = NULL;
  }

  if (mToRemove !== NULL) {
    for (auto it = mToRemove.begin(); it != mToRemove.end(); ++it) {
      mNSFWs.erase(*it);
    }
    delete mToRemove;
    mToRemove = NULL;
  }
}

void Pool::poll() {
  std::chrono::time_point loopStart = std::chrono::steady_clock::now();
  std::chrono::duration delay = std::chrono::milliseconds(50);

  while (mRunning) {
    acceptIntake();
    delay = std::chrono::milliseconds(50);

    for (auto it = mNSFWs.begin(); it != mNSFWs.end(); ++it) {
      const std::chrono::duration request = *it->pollForEvents(loopStart);
      if (request < delay) {
        delay = request;
      }
    }

    std::this_thread::sleep_for(delay);
  }
}

void Pool::runLoop() {
  for (auto it = mNSFWs.begin(); it != mNSFWs.end(); ++it) {
    // TODO: call into native interfaces
  }
}

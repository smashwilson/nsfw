#include "../../includes/osx/FSEventsService.h"

FSEventsService::FSEventsService() : mRunning(false), mRunLoop(NULL) {}

void FSEventService::start() {
  mRunLoop = CFRunLoopGetCurrent();
}

void FSEventService::stop() {
  while(!CFRunLoopIsWaiting(mRunLoop)) {}

  CFRunLoopStop(mRunLoop);
}

bool FSEventService::isRunning() {
  return mRunning;
}

CFRunLoopRef FSEventService::currentRunLoop() {
  return mRunLoop;
}

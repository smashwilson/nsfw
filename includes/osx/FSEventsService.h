#ifndef NSFW_FS_EVENTS_SERVICE_H
#define NSFW_FS_EVENTS_SERVICE_H

#include <CoreServices/CoreServices.h>

class FSEventsService {
public:
  FSEventsService();

  void start();
  void stop();
  bool isRunning();

  CFRunLoopRef currentRunLoop();
private:
  CFRunLoopRef mRunLoop;
  bool mRunning;
};

#endif

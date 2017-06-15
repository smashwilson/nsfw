#ifndef NSFW_FS_EVENTS_WATCHER_H
#define NSFW_FS_EVENTS_WATCHER_H

#include "../Queue.h"

#include <CoreServices/CoreServices.h>
#include <vector>
#include <string>

void fsEventsWatcherCallback(
  ConstFSEventStreamRef streamRef,
  void *clientCallBackInfo,
  size_t numEvents,
  void *eventPaths,
  const FSEventStreamEventFlags eventFlags[],
  const FSEventStreamEventId eventIds[]
);

class FSEventsService;

class FSEventsWatcher {
public:
  FSEventsWatcher(FSEventsService *service, EventQueue &queue, std::string path);
  ~FSEventsWatcher();

  friend void fsEventsWatcherCallback(
    ConstFSEventStreamRef streamRef,
    void *clientCallBackInfo,
    size_t numEvents,
    void *eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[]
  );

  void activate();
  void deactivate();

  std::string getError();
  bool hasErrored();

private:
  void create(std::string path);
  void demangle(std::string path);
  void dispatch(EventType action, std::string path);
  void modify(std::string path);
  void remove(std::string path);
  void rename(std::vector<std::string> *paths);
  void splitFilePath(std::string &directory, std::string &name, std::string path);

  std::string mPath;
  EventQueue &mQueue;

  FSEventsService *mService;
  FSEventStreamRef mEventStream;
};

#endif

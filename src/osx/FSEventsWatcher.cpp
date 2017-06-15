#include "../../includes/osx/FSEventsWatcher.h"
#include <iostream>

FSEventsWatcher::FSEventsWatcher(FSEventsService *service, EventQueue &queue, std::string path) :
  mPath(path),
  mQueue(queue),
  mService(service),
  mEventStream(NULL) { }

FSEventsWatcher::~FSEventsWatcher() { }

void FSEventsWatcher::activate() {
  CFRunLoopRef runLoop = mService->currentRunLoop();

  CFAbsoluteTime latency = 0.001;
  CFStringRef fileWatchPath = CFStringCreateWithCString(
    NULL,
    mPath.c_str(),
    kCFStringEncodingUTF8
  );
  CFArrayRef pathsToWatch = CFArrayCreate(
    NULL,
    (const void **)&fileWatchPath,
    1,
    NULL
  );
  FSEventStreamContext callbackInfo {0, (void *) this, nullptr, nullptr, nullptr};

  mEventStream = FSEventStreamCreate(
    NULL,
    &fsEventsWatcherCallback,
    &callbackInfo,
    pathsToWatch,
    kFSEventStreamEventIdSinceNow,
    latency,
    kFSEventStreamCreateFlagFileEvents
  );

  FSEventStreamScheduleWithRunLoop(mEventStream, mRunLoop, kCFRunLoopDefaultMode);
  FSEventStreamStart(mEventStream);
  CFRelease(pathsToWatch);
  CFRelease(fileWatchPath);

  CFRunLoopRun();
  mExited = true;
}

void FSEventsWatcher::deactivate() {
  //
}

void FSEventsServiceCallback(
  ConstFSEventStreamRef streamRef,
  void *clientCallBackInfo,
  size_t numEvents,
  void *eventPaths,
  const FSEventStreamEventFlags eventFlags[],
  const FSEventStreamEventId eventIds[]
) {
  FSEventsWatcher *eventsService = (FSEventsWatcher *)clientCallBackInfo;
  char **paths = (char **)eventPaths;
  std::vector<std::string> *renamedPaths = new std::vector<std::string>;
  for (size_t i = 0; i < numEvents; ++i) {
    bool isCreated = (eventFlags[i] & kFSEventStreamEventFlagItemCreated) == kFSEventStreamEventFlagItemCreated;
    bool isRemoved = (eventFlags[i] & kFSEventStreamEventFlagItemRemoved) == kFSEventStreamEventFlagItemRemoved;
    bool isModified = (eventFlags[i] & kFSEventStreamEventFlagItemModified) == kFSEventStreamEventFlagItemModified ||
                      (eventFlags[i] & kFSEventStreamEventFlagItemInodeMetaMod) == kFSEventStreamEventFlagItemInodeMetaMod ||
                      (eventFlags[i] & kFSEventStreamEventFlagItemFinderInfoMod) == kFSEventStreamEventFlagItemFinderInfoMod ||
                      (eventFlags[i] & kFSEventStreamEventFlagItemChangeOwner) == kFSEventStreamEventFlagItemChangeOwner ||
                      (eventFlags[i] & kFSEventStreamEventFlagItemXattrMod) == kFSEventStreamEventFlagItemXattrMod;
    bool isRenamed = (eventFlags[i] & kFSEventStreamEventFlagItemRenamed) == kFSEventStreamEventFlagItemRenamed;

    if (isCreated && !(isRemoved || isModified || isRenamed)) {
      eventsService->create(paths[i]);
    } else if (isRemoved && !(isCreated || isModified || isRenamed)) {
      eventsService->remove(paths[i]);
    } else if (isModified && !(isCreated || isRemoved || isRenamed)) {
      eventsService->modify(paths[i]);
    } else if (isRenamed && !(isCreated || isModified || isRemoved)) {
      renamedPaths->push_back(paths[i]);
    } else {
      eventsService->demangle(paths[i]);
    }
  }
  eventsService->rename(renamedPaths);
  delete renamedPaths;
}

void FSEventsWatcher::create(std::string path) {
  dispatch(CREATED, path);
}

void FSEventsWatcher::demangle(std::string path) {
  struct stat file;
  if (stat(path.c_str(), &file) != 0) {
    remove(path);
    return;
  }

  if (file.st_birthtimespec.tv_sec != file.st_mtimespec.tv_sec) {
    modify(path);
  } else {
    create(path);
  }
}

void FSEventsWatcher::dispatch(EventType action, std::string path) {
  std::string directory, name;

  splitFilePath(directory, name, path);

  mQueue.enqueue(action, directory, name);
}

std::string FSEventsWatcher::getError() {
  return "Service shutdown unexpectedly";
}

bool FSEventsWatcher::hasErrored() {
  struct stat root;
  return !isWatching() || stat(mPath.c_str(), &root) < 0;
}

bool FSEventsWatcher::isWatching() {
  return mRunLoop != NULL && mRunLoop->isLooping();
}

void FSEventsWatcher::modify(std::string path) {
  dispatch(MODIFIED, path);
}

void FSEventsWatcher::remove(std::string path) {
  dispatch(DELETED, path);
}

void FSEventsWatcher::rename(std::vector<std::string> *paths) {
  auto *binNamesByPath = new std::map<std::string, std::vector<std::string> *>;

  for (auto pathIterator = paths->begin(); pathIterator != paths->end(); ++pathIterator) {
    std::string directory, name;
    splitFilePath(directory, name, *pathIterator);
    if (binNamesByPath->find(directory) == binNamesByPath->end()) {
      (*binNamesByPath)[directory] = new std::vector<std::string>;
    }
    (*binNamesByPath)[directory]->push_back(name);
  }

  for (auto binIterator = binNamesByPath->begin(); binIterator != binNamesByPath->end(); ++binIterator) {
    if (binIterator->second->size() == 2) {
      std::string sideA = (*binIterator->second)[0],
                  sideB = (*binIterator->second)[1];
      std::string fullSideA = binIterator->first + "/" + sideA,
                  fullSideB = binIterator->first + "/" + sideB;
      struct stat renameSideA, renameSideB;
      bool sideAExists = stat(fullSideA.c_str(), &renameSideA) == 0,
           sideBExists = stat(fullSideB.c_str(), &renameSideB) == 0;

      if (sideAExists && !sideBExists) {
        mQueue.enqueue(RENAMED, binIterator->first, sideB, sideA);
      } else if (!sideAExists && sideBExists) {
        mQueue.enqueue(RENAMED, binIterator->first, sideA, sideB);
      } else {
        demangle(fullSideA);
        demangle(fullSideB);
      }

    } else {
      for (auto pathIterator = binIterator->second->begin(); pathIterator != binIterator->second->end(); ++pathIterator) {
        demangle(binIterator->first + "/" + *pathIterator);
      }
    }
    delete binIterator->second;
    binIterator->second = NULL;
  }
  delete binNamesByPath;
}

void FSEventsWatcher::splitFilePath(std::string &directory, std::string &name, std::string path) {
  if (path.length() == 1 && path[0] == '/') {
    directory = "/";
    name = "";
  } else {
    uint32_t location = path.find_last_of("/");
    directory = (location == 0) ? "/" : path.substr(0, location);
    name = path.substr(location + 1);
  }
}

#ifndef NSFW_NATIVE_INTERFACE_H
#define NSFW_NATIVE_INTERFACE_H

#include "Queue.h"
#include <vector>

class NativeInterface {
public:
  NativeInterface(std::string path);
  ~NativeInterface();

  void activate();
  void deactivate();

  std::string getError();
  std::vector<Event *> *getEvents();
  bool hasErrored();
  bool isWatching();
private:
  EventQueue mQueue;
  void *mNativeInterface;
};

#endif

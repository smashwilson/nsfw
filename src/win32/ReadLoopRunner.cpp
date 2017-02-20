#include <iostream>
#include "../../includes/win32/ReadLoopRunner.h"

ReadLoopRunner::ReadLoopRunner(std::wstring directory, EventQueue &queue, HANDLE directoryHandle):
  mBufferSize(1024),
  mDirectory(directory),
  mDirectoryHandle(directoryHandle),
  mErrorMessage(""),
  mQueue(queue) {
  ZeroMemory(&mOverlapped, sizeof(OVERLAPPED));
  mBuffer = new BYTE[mBufferSize * BUFFER_KB];
  mSwap = new BYTE[mBufferSize * BUFFER_KB];

  std::cerr << (void*) this << " ReadLoopRunner() ==> mBuffer address: " << (void*) mBuffer
    << " mSwap address: " << (void*) mSwap << std::endl;
}

ReadLoopRunner::~ReadLoopRunner() {
  std::cerr << (void*) this << " ~ReadLoopRunner() ==> mBuffer address: " << (void*) mBuffer
    << " mSwap address: " << (void*) mSwap << std::endl;
  delete[] mBuffer;
  delete[] mSwap;
  std::cerr << (void*) this << " ~ReadLoopRunner() ==> both pointers deleted" << std::endl;
}

VOID CALLBACK ReadLoopRunner::eventCallback(DWORD errorCode, DWORD numBytes, LPOVERLAPPED overlapped) {
	std::shared_ptr<ReadLoopRunner> *runner = (std::shared_ptr<ReadLoopRunner> *)overlapped->hEvent;
  std::cerr << (void*) runner->get() << " eventCallback() ==> mBuffer address: " << (void*) runner->get()->mBuffer
    << " mSwap address: " << (void*) runner->get()->mSwap << std::endl;

  if (errorCode != ERROR_SUCCESS) {
    std::cerr << (void*) runner->get() << " eventCallback() ==> errorCode was not ERROR_SUCCESS" << std::endl;
    if (errorCode == ERROR_NOTIFY_ENUM_DIR) {
      std::cerr << (void*) runner->get() << " eventCallback() ==> errorCode was ERROR_NOTIFY_ENUM_DIR" << std::endl;
      runner->get()->setError("Buffer filled up and service needs a restart");
    } else if (errorCode == ERROR_INVALID_PARAMETER) {
      std::cerr << (void*) runner->get() << " eventCallback() ==> errorCode was ERROR_INVALID_PARAMETER" << std::endl;
      // resize the buffers because we're over the network, 64kb is the max buffer size for networked transmission
      runner->get()->resizeBuffers(64);
      runner->get()->read();
      return;
    } else {
      std::cerr << (void*) runner->get() << " eventCallback() ==> errorCode was SHRUG " << errorCode << std::endl;
      runner->get()->setError("Service shutdown unexpectedly");
    }
    std::cerr << (void*) runner->get() << " eventCallback() ==> about to delete runner" << std::endl;
  	delete (std::shared_ptr<ReadLoopRunner> *)runner;
    std::cerr << (void*) runner->get() << " eventCallback() ==> runner deleted" << std::endl;
    return;
  }

  std::cerr << (void*) runner->get() << " eventCallback() ==> about to swap" << std::endl;
  runner->get()->swap(numBytes);
  std::cerr << (void*) runner->get() << " eventCallback() ==> about to read" << std::endl;
  BOOL readScheduled = runner->get()->read();
  std::cerr << (void*) runner->get() << " eventCallback() ==> about to handleEvents" << std::endl;
  runner->get()->handleEvents();
  std::cerr << (void*) runner->get() << " eventCallback() ==> complete" << std::endl;

  if (!readScheduled) {
    delete runner;
  }
}

std::string ReadLoopRunner::getError() {
  return mErrorMessage;
}

std::wstring getWStringFileName(LPWSTR cFileName, DWORD length) {
  LPWSTR nullTerminatedFileName = new WCHAR[length + 1]();
  memcpy(nullTerminatedFileName, cFileName, length);
  std::wstring fileName = nullTerminatedFileName;
  delete[] nullTerminatedFileName;
  return fileName;
}

std::string ReadLoopRunner::getUTF8Directory(std::wstring path) {
  std::wstring::size_type found = path.rfind('\\');
  std::wstringstream utf16DirectoryStream;

  utf16DirectoryStream << mDirectory;

  if (found != std::wstring::npos) {
    utf16DirectoryStream
      << "\\"
      << path.substr(0, found);
  }

  std::wstring uft16DirectoryString = utf16DirectoryStream.str();
  int utf8length = WideCharToMultiByte(
    CP_UTF8,
    0,
    uft16DirectoryString.data(),
    -1,
    0,
    0,
    NULL,
    NULL
  );
  char *utf8CString = new char[utf8length];
  int failureToResolveToUTF8 = WideCharToMultiByte(
    CP_UTF8,
    0,
    uft16DirectoryString.data(),
    -1,
    utf8CString,
    utf8length,
    NULL,
    NULL
  );

  std::string utf8Directory = utf8CString;
  delete[] utf8CString;

  return utf8Directory;
}

std::string getUTF8FileName(std::wstring path) {
  std::wstring::size_type found = path.rfind('\\');
  if (found != std::wstring::npos) {
    path = path.substr(found + 1);
  }

  int utf8length = WideCharToMultiByte(
    CP_UTF8,
    0,
    path.data(),
    -1,
    0,
    0,
    NULL,
    NULL
    );

  // TODO: failure cases for widechar conversion
  char *utf8CString = new char[utf8length];
  int failureToResolveToUTF8 = WideCharToMultiByte(
    CP_UTF8,
    0,
    path.data(),
    -1,
    utf8CString,
    utf8length,
    NULL,
    NULL
    );

  std::string utf8Directory = utf8CString;
  delete[] utf8CString;

  return utf8Directory;
}

void ReadLoopRunner::handleEvents() {
  std::cerr << (void*) this << " handleEvents() ==> mBuffer address: " << (void*) mBuffer
    << " mSwap address: " << (void*) mSwap << std::endl;
  BYTE *base = mSwap;
  for (;;) {
    PFILE_NOTIFY_INFORMATION info = (PFILE_NOTIFY_INFORMATION)base;
    std::cerr << (void*) this << " handleEvents() ==> info address: " << (void*) info << std::endl;
    std::wstring fileName = getWStringFileName(info->FileName, info->FileNameLength);

    if (info->Action == FILE_ACTION_RENAMED_OLD_NAME) {
      if (info->NextEntryOffset != 0) {
        base += info->NextEntryOffset;
        info = (PFILE_NOTIFY_INFORMATION)base;
        if (info->Action == FILE_ACTION_RENAMED_NEW_NAME) {
          std::wstring fileNameNew = getWStringFileName(info->FileName, info->FileNameLength);

          mQueue.enqueue(
            RENAMED,
            getUTF8Directory(fileName),
            getUTF8FileName(fileName),
            getUTF8FileName(fileNameNew)
          );
        }
        else {
          mQueue.enqueue(DELETED, getUTF8Directory(fileName), getUTF8FileName(fileName));
          continue;
        }
      }
      else {
        mQueue.enqueue(DELETED, getUTF8Directory(fileName), getUTF8FileName(fileName));
        break;
      }
    }

    switch (info->Action) {
    case FILE_ACTION_ADDED:
    case FILE_ACTION_RENAMED_NEW_NAME: // in the case we just receive a new name and no old name in the buffer
      mQueue.enqueue(CREATED, getUTF8Directory(fileName), getUTF8FileName(fileName));
      break;
    case FILE_ACTION_REMOVED:
      mQueue.enqueue(DELETED, getUTF8Directory(fileName), getUTF8FileName(fileName));
      break;
    case FILE_ACTION_MODIFIED:
    default:
      mQueue.enqueue(MODIFIED, getUTF8Directory(fileName), getUTF8FileName(fileName));
    };

    if (info->NextEntryOffset == 0) {
      break;
    }
    base += info->NextEntryOffset;
  }

  std::cerr << (void*) this << " handleEvents() ==> complete" << std::endl;
}

bool ReadLoopRunner::hasErrored() {
  return mErrorMessage != "";
}

BOOL ReadLoopRunner::read() {
  if (mDirectoryHandle == NULL) {
    return FALSE;
  }

  DWORD bytes;

  if (!ReadDirectoryChangesW(
    mDirectoryHandle,
    mBuffer,
    mBufferSize * BUFFER_KB,
    TRUE,
    FILE_NOTIFY_CHANGE_FILE_NAME
    | FILE_NOTIFY_CHANGE_DIR_NAME
    | FILE_NOTIFY_CHANGE_ATTRIBUTES
    | FILE_NOTIFY_CHANGE_SIZE
    | FILE_NOTIFY_CHANGE_LAST_WRITE
    | FILE_NOTIFY_CHANGE_LAST_ACCESS
    | FILE_NOTIFY_CHANGE_CREATION
    | FILE_NOTIFY_CHANGE_SECURITY,
    &bytes,
    &mOverlapped,
    &ReadLoopRunner::eventCallback
  )) {
    std::cerr << (void*) this << " read() ==> error encountered during read call" << std::endl;

    DWORD dw = GetLastError();
    LPSTR lpMsgBuf;
    DWORD size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPSTR) &lpMsgBuf,
      0, NULL
    );
    std::string message(lpMsgBuf, size);
    std::cerr << (void*) this << " read() ==> error was " << message << std::endl;
    LocalFree(lpMsgBuf);

    setError("Service shutdown unexpectedly");
    return FALSE;
  }

  std::cerr << (void*) this << " read() ==> async directory watch call successfully scheduled" << std::endl;
  return TRUE;
}

void ReadLoopRunner::resizeBuffers(unsigned int bufferSize) {
  mBufferSize = bufferSize;
  delete[] mBuffer;
  delete[] mSwap;
  mBuffer = new BYTE[mBufferSize * BUFFER_KB];
  mSwap = new BYTE[mBufferSize * BUFFER_KB];
}

void ReadLoopRunner::setError(std::string error) {
  mErrorMessage = error;
}

void ReadLoopRunner::setSharedPointer(std::shared_ptr<ReadLoopRunner> *ptr) {
  mOverlapped.hEvent = new std::shared_ptr<ReadLoopRunner>(*ptr);
}

void ReadLoopRunner::swap(DWORD numBytes) {
  std::cerr << (void*) this << " swap() ==> mBuffer address: " << (void*) mBuffer
    << " mSwap address: " << (void*) mSwap << std::endl;
  void *dest = memcpy(mSwap, mBuffer, numBytes);
  std::cerr << (void*) this << " swap() ==> complete. dest " << (void*) dest
    << " should equal mSwap " << (void*) mSwap << std::endl;

void ReadLoopRunner::prepareForShutdown() {
  mDirectoryHandle = NULL;
}

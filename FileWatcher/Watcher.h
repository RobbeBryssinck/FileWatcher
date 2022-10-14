#pragma once

#include <Windows.h>
#include <vector>
#include <string>

#include "Utils.h"

class Watcher
{
public:
  NOCOPYMOVE(Watcher);

  enum class InitResult
  {
    kSuccess = 0,
    kCreateFileFailed,
    kCreateEventFailed,
    kReadDirectoryChangesFailed,
  };

  enum class ProcessResult
  {
    kSuccess = 0,
    kUnknown,
    kTimeOut,
  };

  Watcher() = default;
  ~Watcher();

  [[nodiscard]] InitResult Init(LPCTSTR pDir);
  ProcessResult ProcessUpdates(std::vector<std::pair<std::wstring, DWORD>>& out);

private:
  bool RegisterChangesSink();
  [[nodiscard]] FILE_NOTIFY_INFORMATION* UpdateCurrentEvent(FILE_NOTIFY_INFORMATION* pCurrentEvent) const;

private:
  HANDLE dirHandle{};
  OVERLAPPED overlapped{};
  uint8_t events[1024]{};
};

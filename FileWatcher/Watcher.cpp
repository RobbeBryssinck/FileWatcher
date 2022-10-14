#include "Watcher.h"

#include <Windows.h>

Watcher::~Watcher()
{
  if (dirHandle != NULL && dirHandle != INVALID_HANDLE_VALUE)
    CloseHandle(dirHandle);

  if (overlapped.hEvent != NULL && overlapped.hEvent != INVALID_HANDLE_VALUE)
    CloseHandle(overlapped.hEvent);
}

Watcher::InitResult Watcher::Init(LPCTSTR pDir)
{
  dirHandle = CreateFile(pDir, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

  if (dirHandle == NULL || dirHandle == INVALID_HANDLE_VALUE)
    return InitResult::kCreateFileFailed;

  overlapped.hEvent = CreateEvent(NULL, FALSE, 0, NULL);

  if (overlapped.hEvent == NULL || overlapped.hEvent == INVALID_HANDLE_VALUE)
    return InitResult::kCreateEventFailed;

  if (!RegisterChangesSink())
    return InitResult::kReadDirectoryChangesFailed;

  return InitResult::kSuccess;
}

Watcher::ProcessResult Watcher::ProcessUpdates(std::vector<std::pair<std::wstring, DWORD>>& out)
{
  DWORD result = WaitForSingleObject(overlapped.hEvent, INFINITE);
  if (result == WAIT_TIMEOUT)
    return ProcessResult::kTimeOut;

  DWORD bytesTransferred{};
  if (!GetOverlappedResult(dirHandle, &overlapped, &bytesTransferred, FALSE))
    return ProcessResult::kUnknown;

  FILE_NOTIFY_INFORMATION* pEvent = nullptr;

  do
  {
    if (!pEvent)
      pEvent = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(events);
    else
      pEvent = UpdateCurrentEvent(pEvent);

    out.push_back({ std::wstring(pEvent->FileName, pEvent->FileNameLength / 2), pEvent->Action });
  } while (pEvent->NextEntryOffset);

  if (!RegisterChangesSink())
    return ProcessResult::kUnknown;

  return ProcessResult::kSuccess;
}

bool Watcher::RegisterChangesSink()
{
  DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES 
    | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS 
    | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;
  return ReadDirectoryChangesW(dirHandle, events, sizeof(events), TRUE, filter, NULL, &overlapped, NULL);
}

FILE_NOTIFY_INFORMATION* Watcher::UpdateCurrentEvent(FILE_NOTIFY_INFORMATION* pCurrentEvent) const
{
  return reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<uint8_t*>(pCurrentEvent) + reinterpret_cast<FILE_NOTIFY_INFORMATION*>(pCurrentEvent)->NextEntryOffset);
}
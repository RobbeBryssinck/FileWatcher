#include <iostream>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <set>
#include <filesystem>

#define NOCOPYMOVE(className) \
    className(className&&) = delete; \
    className(const className&) = delete; \
    className& operator=(const className&) = delete; \
    className& operator=(className&&) = delete;

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
  ~Watcher()
  {
    if (dirHandle != NULL && dirHandle != INVALID_HANDLE_VALUE)
      CloseHandle(dirHandle);

    if (overlapped.hEvent != NULL && overlapped.hEvent != INVALID_HANDLE_VALUE)
      CloseHandle(overlapped.hEvent);
  }

  [[nodiscard]] InitResult Init(LPCTSTR pDir)
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

  ProcessResult ProcessUpdates(std::vector<std::pair<std::wstring, DWORD>>& out)
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

private:
  bool RegisterChangesSink()
  {
    DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES 
      | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS 
      | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;
    return ReadDirectoryChangesW(dirHandle, events, sizeof(events), TRUE, filter, NULL, &overlapped, NULL);
  }

  [[nodiscard]] FILE_NOTIFY_INFORMATION* UpdateCurrentEvent(FILE_NOTIFY_INFORMATION* pCurrentEvent) const
  {
    return reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<uint8_t*>(pCurrentEvent) + reinterpret_cast<FILE_NOTIFY_INFORMATION*>(pCurrentEvent)->NextEntryOffset);
  }

private:
  HANDLE dirHandle{};
  OVERLAPPED overlapped{};
  uint8_t events[1024]{};
};

namespace Mover
{
  namespace fs = std::filesystem;

  static fs::path s_watchedPath;
  static fs::path s_destinationPath;

  void CopyNewFile(const std::wstring& file)
  {
    const fs::path src(s_watchedPath / file);
    const fs::path dest(s_destinationPath / file);

    try
    {
      fs::copy_file(src, dest, fs::copy_options::update_existing);

      std::wcout << L"Copied file: " << file << std::endl;
    }
    catch (std::exception e)
    {
      std::wcerr << L"Failed to copy file: " << file << std::endl;
    }
  }
}

[[noreturn]] DWORD ReportError(std::string pErrorString)
{
  DWORD errorCode = GetLastError();
  std::cerr << "Error: " << pErrorString << "\nError code: " << errorCode << std::endl;
  return errorCode;
}

[[nodiscard]] DWORD WatchDirectory(LPCTSTR pDir)
{
  Watcher watcher{};

  Mover::s_watchedPath = std::wstring(pDir);

  {
    using WIR = Watcher::InitResult;
    switch (watcher.Init(pDir))
    {
      case WIR::kCreateFileFailed:
        return ReportError("CreateFile function failed.");
      case WIR::kCreateEventFailed:
        return ReportError("CreateEvent function failed.");
      case WIR::kReadDirectoryChangesFailed:
        return ReportError("ReadDirectoryChanges function failed.");
      case WIR::kSuccess:
        std::cout << "Watcher created successfully." << std::endl;
    }
  }

  while (true)
  {
    std::vector<std::pair<std::wstring, DWORD>> files{};
    Watcher::ProcessResult result = watcher.ProcessUpdates(files);

    switch (result)
    {
    case Watcher::ProcessResult::kTimeOut:
      std::cout << "No changes in the timeout period." << std::endl;
      break;
    case Watcher::ProcessResult::kSuccess:
      break;
    case Watcher::ProcessResult::kUnknown:
    default:
      return ReportError("unhandled waitStatus.");
    }

    std::set<std::wstring> processedFiles{};

    for (const auto& file : files)
    {
      if (processedFiles.contains(file.first))
        continue;

      std::wcout << file.first << ": " << file.second << std::endl;

      if (file.second == 1 || file.second == 3) // if file is created or edited
      {
        Mover::CopyNewFile(file.first);
        processedFiles.insert(file.first);
      }
    }
  }

  return NULL;
}

int main(int argc, TCHAR* argv[])
{
  DWORD errorCode = NULL;

  switch (argc)
  {
#ifdef _DEBUG
  case 1:
    Mover::s_destinationPath = L"C:\\dev\\destination\\";
    errorCode = WatchDirectory(L"C:\\dev\\test\\");
    break;
#endif
  case 3:
    Mover::s_destinationPath = argv[2];
    errorCode = WatchDirectory(argv[1]);
    break;
  default:
    std::wcout << L"Usage: " << argv[0] << L" watched_path destination_path" << std::endl;
  }

  if (errorCode)
    ExitProcess(errorCode);
}

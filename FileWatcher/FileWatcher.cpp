#include <Windows.h>
#include <tchar.h>
#include <set>

#include "Utils.h"
#include "Watcher.h"
#include "Mover.h"

[[nodiscard]] DWORD WatchDirectory(LPCTSTR pDir)
{
  Watcher watcher{};

  Mover::SetWatchedPath(pDir);

  {
    using WIR = Watcher::InitResult;
    switch (watcher.Init(pDir))
    {
      case WIR::kCreateFileFailed:
        return Logging::ReportError("CreateFile function failed.");
      case WIR::kCreateEventFailed:
        return Logging::ReportError("CreateEvent function failed.");
      case WIR::kReadDirectoryChangesFailed:
        return Logging::ReportError("ReadDirectoryChanges function failed.");
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
    case Watcher::ProcessResult::kSuccess:
      break;
    case Watcher::ProcessResult::kTimeOut:
      std::cout << "No changes in the timeout period." << std::endl;
      break;
    case Watcher::ProcessResult::kUnknown:
    default:
      return Logging::ReportError("unhandled waitStatus.");
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
    Mover::SetDestinationPath(L"C:\\dev\\destination\\");
    errorCode = WatchDirectory(L"C:\\dev\\test\\");
    break;
#endif
  case 3:
    Mover::SetDestinationPath(argv[2]);
    errorCode = WatchDirectory(argv[1]);
    break;
  default:
    std::wcout << L"Usage: " << argv[0] << L" watched_path destination_path" << std::endl;
  }

  if (errorCode)
    ExitProcess(errorCode);
}

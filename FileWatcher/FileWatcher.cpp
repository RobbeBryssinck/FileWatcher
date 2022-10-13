#include <iostream>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <string>

namespace
{
  HANDLE s_changeHandle = NULL;
}

void CloseChangeHandle()
{
  if (s_changeHandle != NULL && s_changeHandle != INVALID_HANDLE_VALUE)
    FindCloseChangeNotification(s_changeHandle);
}

[[noreturn]] void ErrorAndExit(std::string pErrorString)
{
  CloseChangeHandle();

  DWORD errorCode = GetLastError();
  std::cerr << "Error: " << pErrorString << std::endl;
  ExitProcess(errorCode);
}

void WatchDirectory(LPCTSTR pDir)
{
  TCHAR drive[4]{};
  TCHAR file[_MAX_FNAME]{};
  TCHAR extension[_MAX_EXT]{};

  _tsplitpath_s(pDir, drive, 4, NULL, 0, file, _MAX_FNAME, extension, _MAX_EXT);

  s_changeHandle = FindFirstChangeNotification(pDir, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
 
  if (s_changeHandle == NULL || s_changeHandle == INVALID_HANDLE_VALUE)
    ErrorAndExit("FindFirstChangeNotification function failed.");

  while (TRUE)
  {
    DWORD waitStatus = WaitForSingleObject(s_changeHandle, INFINITE);

    switch (waitStatus)
    { 
      case WAIT_OBJECT_0:
        _tprintf(L"Directory %s changed.\n", pDir);

        if (FindNextChangeNotification(s_changeHandle) == FALSE)
          ErrorAndExit("FindNextChangeNotification function failed.");
        break;

      case WAIT_TIMEOUT:
        std::cout << "No changes in the timeout period." << std::endl;
        break;

      default:
        ErrorAndExit("unhandled dwWaitStatus.");
    }
  }

  CloseChangeHandle();
}

int main(int argc, TCHAR* argv[])
{
  switch (argc)
  {
  case 1:
    WatchDirectory(L"C:\\dev\\test");
    break;
  case 2:
    WatchDirectory(argv[1]);
    break;
  default:
    std::cerr << "Path to directory required!" << std::endl;
  }
}

#include <iostream>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <string>

void ErrorAndExit(std::string pErrorString)
{
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

  HANDLE changeHandle = FindFirstChangeNotification(pDir, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
 
  if (changeHandle == NULL || changeHandle == INVALID_HANDLE_VALUE)
    ErrorAndExit("FindFirstChangeNotification function failed.");

  while (TRUE)
  {
    DWORD waitStatus = WaitForSingleObject(changeHandle, INFINITE);

    switch (waitStatus)
    { 
      case WAIT_OBJECT_0:
        _tprintf(L"Directory %s changed.\n", pDir);

        if (FindNextChangeNotification(changeHandle) == FALSE)
          ErrorAndExit("FindNextChangeNotification function failed.");
        break;

      case WAIT_TIMEOUT:
        std::cout << "No changes in the timeout period." << std::endl;
        break;

      default:
        ErrorAndExit("unhandled dwWaitStatus.");
    }
  }
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

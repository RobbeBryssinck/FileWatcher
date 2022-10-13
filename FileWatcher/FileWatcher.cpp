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

  drive[2] = static_cast<TCHAR>('\\');
  drive[3] = static_cast<TCHAR>('\0');

  HANDLE changeHandles[2]{};
 
  changeHandles[0] = FindFirstChangeNotification(pDir, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
 
  if (changeHandles[0] == INVALID_HANDLE_VALUE) 
    ErrorAndExit("FindFirstChangeNotification function failed.");

  changeHandles[1] = FindFirstChangeNotification(drive, TRUE, FILE_NOTIFY_CHANGE_DIR_NAME);
 
  if (changeHandles[1] == INVALID_HANDLE_VALUE) 
    ErrorAndExit("FindFirstChangeNotification function failed.");

  if ((changeHandles[0] == NULL) || (changeHandles[1] == NULL))
    ErrorAndExit("unexpected NULL from FindFirstChangeNotification.");

  while (TRUE) 
  {
    std::cout << "Waiting for notification..." << std::endl;

    DWORD dwWaitStatus = WaitForMultipleObjects(2, changeHandles, FALSE, INFINITE); 

    switch (dwWaitStatus) 
    { 
      case WAIT_OBJECT_0: 
        _tprintf(L"Directory %s changed.\n", pDir);

        if (FindNextChangeNotification(changeHandles[0]) == FALSE)
          ErrorAndExit("FindNextChangeNotification function failed.");
        break; 

      case WAIT_OBJECT_0 + 1: 
        _tprintf(L"Directory tree %s changed.\n", drive);

        if (FindNextChangeNotification(changeHandles[1]) == FALSE)
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

#pragma once

#include <iostream>

namespace Logging
{
  inline DWORD ReportError(const char* pErrorString)
  {
    DWORD errorCode = GetLastError();
    std::cerr << "Error: " << pErrorString << "\nError code: " << errorCode << std::endl;
    return errorCode;
  }
}

#define NOCOPYMOVE(className) \
    className(className&&) = delete; \
    className(const className&) = delete; \
    className& operator=(const className&) = delete; \
    className& operator=(className&&) = delete;


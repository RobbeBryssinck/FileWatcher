#include <filesystem>
#include <iostream>

namespace Mover
{
  namespace fs = std::filesystem;

  static fs::path s_watchedPath;
  static fs::path s_destinationPath;

  void SetWatchedPath(const std::wstring& watchedPath)
  {
    s_watchedPath = watchedPath;
  }

  void SetDestinationPath(const std::wstring& destinationPath)
  {
    s_destinationPath = destinationPath;
  }

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

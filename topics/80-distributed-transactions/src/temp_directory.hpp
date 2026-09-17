#pragma once
#include <filesystem>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>
namespace tutorial {
class TempDirectory {
 public:
  std::filesystem::path path;
  TempDirectory() {
    auto pattern=(std::filesystem::temp_directory_path()/"cmu445-2pc-XXXXXX").string();
    std::vector<char> buffer(pattern.begin(),pattern.end()); buffer.push_back('\0');
    char* p=::mkdtemp(buffer.data()); if(!p) throw std::runtime_error("mkdtemp failed"); path=p;
  }
  ~TempDirectory() {std::error_code ec; std::filesystem::remove_all(path,ec);}
  TempDirectory(const TempDirectory&)=delete;
  TempDirectory& operator=(const TempDirectory&)=delete;
};
}

#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace waddup {
  class Finder {
  public:
    explicit Finder(std::string directory, std::string destination);
    std::vector<std::string> findDuplicates();

  private:
    struct FileInfo {
      std::string path;
      std::size_t size;
    };

    std::string directory_;
    std::string destination_;
    std::unordered_map<std::string, std::vector<FileInfo>>
        hash_map_;  // hash -> vector of files with that hash

    static std::string calculateFileHash(const std::string &filepath);

    std::size_t file_counter_ = 0;  // Counter for generating unique filenames
    std::string makeDestinationPath(const std::string &originalPath);
  };
}  // namespace waddup

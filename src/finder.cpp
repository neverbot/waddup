#include "finder.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/evp.h>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

namespace waddup {

  const unsigned int BUFFER_SIZE = 4096;

  /**
   * @brief Constructor for the Finder class.
   * @param directory The directory to search for duplicate files.
   * @throws std::runtime_error if the directory does not exist or is not a
   * directory.
   */
  Finder::Finder(std::string directory, std::string destination) {
    directory_   = std::move(directory);
    destination_ = std::move(destination);

    // Now you can add more initialization code here
    if (!fs::exists(directory_)) {
      throw std::runtime_error("Directory does not exist: " + directory_);
    }

    if (!fs::is_directory(directory_)) {
      throw std::runtime_error("Path is not a directory: " + directory_);
    }

    // Create destination directory if specified and doesn't exist
    if (!destination_.empty()) {
      if (!fs::exists(destination_)) {
        if (!fs::create_directories(destination_)) {
          throw std::runtime_error("Could not create destination directory: " +
                                   destination_);
        }
      } else if (!fs::is_directory(destination_)) {
        throw std::runtime_error("Destination path is not a directory: " +
                                 destination_);
      }
    }
  }

  /**
   * @brief Calculate the SHA-256 hash of a file.
   * @param filepath The path to the file.
   * @return The SHA-256 hash of the file as a hexadecimal string.
   */
  std::string Finder::calculateFileHash(const std::string &filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
      std::cerr << "Could not open file: " << filepath << "\n";
      return "";
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
      std::cerr << "Could not create hash context" << "\n";
      return "";
    }

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr)) {
      EVP_MD_CTX_free(ctx);
      std::cerr << "Could not initialize hash context" << "\n";
      return "";
    }

    char buffer[BUFFER_SIZE];
    while (file.read(buffer, sizeof(buffer))) {
      if (!EVP_DigestUpdate(ctx, buffer, file.gcount())) {
        EVP_MD_CTX_free(ctx);
        std::cerr << "Could not update hash" << "\n";
        return "";
      }
    }

    // Don't forget to process the last chunk if file size isn't a multiple of
    // buffer size
    if (file.gcount() > 0) {
      if (!EVP_DigestUpdate(ctx, buffer, file.gcount())) {
        EVP_MD_CTX_free(ctx);
        std::cerr << "Could not update hash with final chunk" << "\n";
        return "";
      }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  hash_len;
    if (!EVP_DigestFinal_ex(ctx, hash, &hash_len)) {
      EVP_MD_CTX_free(ctx);
      std::cerr << "Could not finalize hash" << "\n";
      return "";
    }

    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
      ss << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(hash[i]);
    }

    return ss.str();
  }

  /**
   * @brief Find duplicate files in the specified directory.
   * @return A vector of strings describing the duplicate files found.
   *         Each string contains the hash and the paths of the duplicate files.
   */
  std::vector<std::string> Finder::findDuplicates() {
    std::vector<std::string>                                  results;
    std::unordered_map<std::size_t, std::vector<std::string>> size_map;

    std::cout << "Searching for WAD files in directory: " << directory_ << "\n";

    // First pass: group WAD files by size
    for (const auto &entry : fs::recursive_directory_iterator(directory_)) {
      if (entry.is_regular_file() && entry.path().extension().string() ==
                                         ".wad") {  // Check for .wad extension
        auto size = entry.file_size();
        size_map[size].push_back(entry.path().string());
      }
    }

    std::size_t files_to_process = 0;  // Counter for total WAD files
    for (const auto &[size, files] : size_map) {
      if (files.size() > 1) {
        files_to_process += files.size();  // Count all potential duplicates
      } else if (!destination_.empty()) {
        files_to_process++;  // Count unique files to copy
      }
    }

    std::cout << "Total WAD files found: " << files_to_process << "\n";
    if (files_to_process == 0) {
      results.push_back("No WAD files found in the directory.");
      return results;
    }

    // Count total processed files for progress calculation
    std::size_t files_processed = 0;
    auto        updateProgress  = [&files_processed, files_to_process]() {
      float progress = (files_processed * 100.0f) / files_to_process;
      std::cout << "\rProgress: " << std::fixed << std::setprecision(1)
                << progress << "%" << std::flush;
    };

    // Second pass: calculate hashes and copy files
    for (const auto &[size, files] : size_map) {
      if (files.size() > 1) {  // Only process potential duplicates
        for (const auto &filepath : files) {
          auto hash = calculateFileHash(filepath);

          if (!hash.empty()) {
            FileInfo info{filepath, size};
            hash_map_[hash].push_back(info);
          }

          files_processed++;
          updateProgress();
        }
      } else if (!destination_.empty()) {  // Single file, copy to destination
        const auto &filepath = files[0];
        fs::path    destPath = makeDestinationPath(filepath);

        try {
          fs::copy_file(filepath, destPath,
                        fs::copy_options::overwrite_existing);
          // results.push_back("Copied unique WAD: " + filepath);
        } catch (const fs::filesystem_error &e) {
          results.push_back("Error copying " + filepath + ": " + e.what());
        }

        files_processed++;
        updateProgress();
      }
    }

    std::cout << "\nProcessing complete.\n\n";

    // Generate results for files with matching hashes
    for (const auto &[hash, files] : hash_map_) {
      if (files.size() > 1) {
        results.push_back("Found duplicate WAD files with hash " + hash + ":");
        for (const auto &file : files) {
          results.push_back("  " + file.path + " (" +
                            std::to_string(file.size) + " bytes)");
        }

        // If destination is set, copy the first instance
        if (!destination_.empty()) {
          const auto &firstFile = files[0];
          fs::path    destPath  = makeDestinationPath(firstFile.path);

          try {
            fs::copy_file(firstFile.path, destPath,
                          fs::copy_options::overwrite_existing);
            results.push_back("Copied first instance to destination: " +
                              destPath.string());
          } catch (const fs::filesystem_error &e) {
            results.push_back("Error copying to destination: " +
                              std::string(e.what()));
          }
        }

        results.push_back("");
      }
    }

    return results;
  }

  /**
   * @brief Generate a unique destination path for the copied file.
   * @param originalPath The original file path.
   * @return A unique destination path for the copied file.
   */
  std::string Finder::makeDestinationPath(const std::string &originalPath) {
    fs::path original = fs::path(originalPath);
    fs::path relative = fs::relative(original, directory_);

    // Convert path separators to underscores and remove the extension
    std::string path_part = relative.parent_path().string();
    std::replace(path_part.begin(), path_part.end(), '/', '_');

    std::stringstream ss;
    ss << std::setw(6) << std::setfill('0') << ++file_counter_ << "_";

    // Add path part if it exists
    if (!path_part.empty()) {
      ss << path_part << "_";
    }

    // Add original filename
    ss << relative.stem().string() << relative.extension().string();

    return (fs::path(destination_) / ss.str()).string();
  }

}  // namespace waddup

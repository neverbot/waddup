#include "./finder.hpp"
#include "./hash.hpp"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout << "Usage: waddup <source_directory> [destination_directory]\n";
    std::cout << "  source_directory: Directory to search for WAD files\n";
    std::cout << "  destination_directory: Directory to copy unique WADs to\n";
    return 1;
  }

  std::cout << "WAD DUPlicate detector:" << "\n";

  waddup::Finder finder(argv[1], argv[2]);
  auto           duplicates = finder.findDuplicates();

  if (duplicates.empty()) {
    std::cout << "No duplicate WAD files found." << "\n";
  } else {
    for (const auto &line : duplicates) {
      std::cout << line << "\n";
    }
  }

  return 0;
}

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>

#include "ZipArchive.h"
#include "ZipFile.h"
#include "ZipArchiveEntry.h"
#include "utils/stream_utils.h"

namespace fs = std::filesystem;

std::vector<std::string> find_in_archive(const fs::path& archivepath, const std::string& pattern) {
  std::vector<std::string> results;

  if (!fs::exists(archivepath)) return results;

  try {
    auto archive = ZipFile::Open(archivepath.string());

    for (size_t i = 0, size = archive->GetEntriesCount(); i < size; i++) {
      auto entry = archive->GetEntry(i);
      if (entry != nullptr && !entry->IsDirectory()) {
        auto name = entry->GetName();
        if (std::regex_match(name, std::regex{pattern.c_str()}))
          results.push_back(entry->GetFullName());
      }
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return results;
}

void test_find_in_archive() {
  auto results = find_in_archive("sample79.zip", R"(^.*\.jpg$)");
  for (const auto& name : results) {
    std::cout << name << std::endl;
  }
}

void compress(const std::filesystem::path& source, const std::filesystem::path& archive,
              std::function<void(std::string_view)> reporter) {
  if (std::filesystem::is_regular_file(source)) {
    if (reporter != nullptr) reporter("Compressing " + source.string());
    ZipFile::AddFile(archive.string(), source.string(), LzmaMethod::Create());
  } else {
    for (const auto& p : std::filesystem::recursive_directory_iterator(source)) {
      if (reporter != nullptr) reporter("Compressing " + p.path().string());

      if (std::filesystem::is_directory(p)) {
        auto zipArchive = ZipFile::Open(archive.string());
        auto entry = zipArchive->CreateEntry(p.path().string());
        entry->SetAttributes(ZipArchiveEntry::Attributes::Directory);
        ZipFile::SaveAndClose(zipArchive, archive.string());
      } else if (std::filesystem::is_regular_file(p)) {
        ZipFile::AddFile(archive.string(), p.path().string(), LzmaMethod::Create());
      }
    }
  }
}

void ensure_directory_exists(const fs::path& dir) {
  if (!fs::exists(dir)) {
    std::error_code err;
    fs::create_directories(dir, err);
  }
}

void decompress(const fs::path& destination, const fs::path& archive,
                std::function<void(std::string_view)> reporter) {
  ensure_directory_exists(destination);

  auto zipArchive = ZipFile::Open(archive.string());
  for (size_t i = 0; i < zipArchive->GetEntriesCount(); i++) {
    auto entry = zipArchive->GetEntry(i);
    if (entry == nullptr) continue;

    auto filepath = destination / fs::path{entry->GetFullName()}.relative_path();
    if (reporter != nullptr) reporter("Creating " + filepath.string());

    if (entry->IsDirectory()) {
      ensure_directory_exists(filepath);
    } else {
      ensure_directory_exists(filepath.parent_path());

      std::ofstream destFile;
      destFile.open(filepath.string().c_str(), std::ios::binary | std::ios::trunc);

      if (!destFile.is_open()) {
        if (reporter != nullptr)
          reporter("Cannot create destination file!");
      }

      auto dataStream = entry->GetDecompressionStream();
      if (dataStream != nullptr) {
        utils::stream::copy(*dataStream, destFile);
      }
    }
  }
}

int main() { test_find_in_archive(); }

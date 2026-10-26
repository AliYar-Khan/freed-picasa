#pragma once
// Folder scan + in-memory index. Recursively scans folders for images,
// reads EXIF + picasa.ini per-folder metadata into EditState per item.
#include "edit/ops.h"
#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace pic {

struct Photo {
  std::string path;      // absolute
  std::string name;      // display
  std::string folder;    // parent dir absolute
  int w = 0, h = 0;
  int orientation = 0;   // exif baked at scan
  uint64_t key = 0;      // stable id for caches
  double mtime = 0;
  uint64_t size = 0;
  std::string dt;        // exif datetime "YYYY:MM:DD HH:MM:SS"
  std::string keywords;  // picasa.ini "keywords=" comma/space separated
  bool starred = false;
  bool hidden = false;
  EditState edits;
};

class Scanner {
 public:
  void add_root(const std::string& dir);
  void clear_all();
  void rescan();                  // blocking; call on bg thread wrapper
  void rescan_async();            // std::thread-based
  std::string rescan_status();

  std::vector<Photo> snapshot();  // thread-safe copy
  size_t count();
  Photo* find(const std::string& path);

  // persists picasa.ini edit for one photo (Save semantics)
  bool persist_edits(const Photo& p);
  bool read_edits(Photo& p) const;

  std::vector<std::string> roots();
  std::vector<std::string> list_dirs(const std::string& parent);
  std::vector<uint32_t> indices_in_dir(const std::string& dir);

  std::mutex mu;                  // protects items
  std::vector<Photo> items;
  std::unordered_map<std::string, size_t> by_path;
  std::vector<std::string> roots_;
  std::atomic<bool> scanning{false};
  std::atomic<bool> rescan_requested{false};

 private:
  void scan_dir(const std::string& dir, std::vector<Photo>& out);
  static bool is_image(const std::string& name);
};
}  // namespace pic

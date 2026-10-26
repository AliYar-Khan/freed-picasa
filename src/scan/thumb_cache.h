#pragma once
// Disk + VRAM thumbnail cache. Decode worker produces RgbaImage; the main
// GL thread uploads textures (GL state must stay on the render thread).
#include "image/decode.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace pic {

struct GlTex {
  unsigned id = 0;
  int w = 0, h = 0;
};

class ThumbCache {
 public:
  ~ThumbCache() { stop(); }
  void init(const std::string& cache_dir, int max_side);
  void request(uint64_t key, const std::string& path, double mtime_size);
  // main thread: poll finished decodes -> upload GL. Returns count uploaded.
  int upload_pending();
  bool get(uint64_t key, GlTex& out) const;
  bool pending(uint64_t key) const;
  void evict(uint64_t key);
  void stop();

  static std::string cache_file(const std::string& cache_dir, const std::string& path,
                                double mtime_size);

 private:
  void worker();
  std::string dir_;
  int max_side_ = 320;
  mutable std::mutex qmu_;
  std::condition_variable cv_;
  std::deque<std::pair<uint64_t, std::string>> queue_;
  std::unordered_map<uint64_t, double> requested_;  // key -> mtime_size
  mutable std::mutex mu_;
  std::unordered_map<uint64_t, GlTex> tex_;
  std::unordered_map<uint64_t, Image> decoded_;
  std::atomic<bool> quit_{false};
  bool running_ = false;
  std::thread th_;
};

}  // namespace pic

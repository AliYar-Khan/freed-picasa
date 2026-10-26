#include "thumb_cache.h"
#include "image/image.h"
#include <GL/gl.h>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <sys/stat.h>
#include <sys/types.h>

namespace pic {

static uint64_t hash64(const std::string& s) {
  uint64_t h = 1469598103934665603ull;  // FNV-1a
  for (char c : s) {
    h ^= uint8_t(c);
    h *= 1099511628211ull;
  }
  return h;
}

std::string ThumbCache::cache_file(const std::string& cache_dir, const std::string& path,
                                   double mtime_size) {
  uint64_t ph = hash64(path);
  return cache_dir + "/" + std::to_string(ph) + "_" + std::to_string(uint64_t(mtime_size * 1000.0)) +
         ".jpg";
}

static bool load_disk_thumb(const std::string& file, Image& out) { return load_rgba8(file, out); }

void ThumbCache::init(const std::string& cache_dir, int max_side) {
  dir_ = cache_dir;
  max_side_ = max_side;
  mkdir(cache_dir.c_str(), 0755);
  if (running_ || th_.joinable()) return;
  running_ = true;
  quit_ = false;
  th_ = std::thread([this] { worker(); });
}

void ThumbCache::request(uint64_t key, const std::string& path, double mtime_size) {
  {
    std::lock_guard<std::mutex> lk(qmu_);
    if (requested_.count(key)) return;  // already queued
    requested_[key] = mtime_size;
    queue_.push_back({key, path});
  }
  cv_.notify_one();
}

void ThumbCache::worker() {
  for (;;) {
    std::pair<uint64_t, std::string> job;
    {
      std::unique_lock<std::mutex> lk(qmu_);
      cv_.wait(lk, [&] { return quit_ || !queue_.empty(); });
      if (quit_) return;
      job = queue_.front();
      queue_.pop_front();
    }
    uint64_t key = job.first;
    const std::string& path = job.second;
    Image im;
    std::string cf = cache_file(dir_, path, requested_[key]);
    bool ok = load_disk_thumb(cf, im);
    if (!ok) {
      ThumbImage ti;
      if (decode_thumb(path, max_side_, ti) && ti.im.ok()) {
        im = std::move(ti.im);
        save_jpeg(cf, im, 88);  // best-effort
      }
    }
    // if EXIF rotation is needed then decode_thumb already applied it;
    // a disk-cached thumb is already oriented.
    std::lock_guard<std::mutex> lk(mu_);
    if (ok || im.ok()) decoded_[key] = std::move(im);
    else decoded_[key].alloc(1, 1);  // placeholder to unstick UI
  }
}

int ThumbCache::upload_pending() {
  std::vector<std::pair<uint64_t, Image>> take;
  {
    std::lock_guard<std::mutex> lk(mu_);
    take.reserve(decoded_.size());
    for (auto& kv : decoded_) take.push_back({kv.first, std::move(kv.second)});
    decoded_.clear();
  }
  if (take.empty()) return 0;
  for (auto& kv : take) {
    Image& im = kv.second;
    unsigned tex = 0;
    if (!im.ok()) continue;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, im.w, im.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, im.row(0));
    std::lock_guard<std::mutex> lk(mu_);
    GlTex gt;
    gt.id = tex;
    gt.w = im.w;
    gt.h = im.h;
    tex_[kv.first] = gt;
  }
  return int(take.size());
}

bool ThumbCache::get(uint64_t key, GlTex& out) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = tex_.find(key);
  if (it == tex_.end()) return false;
  out = it->second;
  return true;
}

bool ThumbCache::pending(uint64_t key) const {
  std::lock_guard<std::mutex> lk(qmu_);
  return requested_.count(key) > 0;
}

void ThumbCache::evict(uint64_t key) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = tex_.find(key);
  if (it != tex_.end()) {
    if (it->second.id) glDeleteTextures(1, &it->second.id);
    tex_.erase(it);
  }
}

void ThumbCache::stop() {
  if (!running_) return;
  quit_ = true;
  cv_.notify_all();
  if (th_.joinable()) th_.join();
  running_ = false;
  std::lock_guard<std::mutex> lk(mu_);
  for (auto& kv : tex_) {
    if (kv.second.id) glDeleteTextures(1, &kv.second.id);
  }
  tex_.clear();
}

}  // namespace pic

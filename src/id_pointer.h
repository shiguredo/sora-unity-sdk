#ifndef SORA_ID_POINTER_H_INCLUDED
#define SORA_ID_POINTER_H_INCLUDED

#include <map>
#include <mutex>

#include "unity.h"

namespace sora {

// TextureUpdateCallback のユーザデータが 32bit 整数しか扱えないので、
// ID からポインタに変換する仕組みを用意する
class IdPointer {
  std::mutex mutex_;
  ptrid_t counter_ = 1;
  std::map<ptrid_t, void*> map_;

 public:
  static IdPointer& Instance() {
    static IdPointer ip;
    return ip;
  }
  ptrid_t Register(void* p) {
    std::lock_guard<std::mutex> guard(mutex_);
    map_[counter_] = p;
    return counter_++;
  }
  void Unregister(ptrid_t id) {
    std::lock_guard<std::mutex> guard(mutex_);
    map_.erase(id);
  }
  void* Lookup(ptrid_t id) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = map_.find(id);
    if (it == map_.end()) {
      return nullptr;
    }
    return it->second;
  }
};

}  // namespace sora

#endif  // SORA_ID_POINTER_H_INCLUDED
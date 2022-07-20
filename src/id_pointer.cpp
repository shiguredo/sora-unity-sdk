#include "id_pointer.h"

namespace sora_unity_sdk {

IdPointer& IdPointer::Instance() {
  static IdPointer ip;
  return ip;
}
ptrid_t IdPointer::Register(void* p) {
  std::lock_guard<std::mutex> guard(mutex_);
  map_[counter_] = p;
  return counter_++;
}
void IdPointer::Unregister(ptrid_t id) {
  std::lock_guard<std::mutex> guard(mutex_);
  map_.erase(id);
}
void* IdPointer::Lookup(ptrid_t id) {
  std::lock_guard<std::mutex> guard(mutex_);
  auto it = map_.find(id);
  if (it == map_.end()) {
    return nullptr;
  }
  return it->second;
}

}  // namespace sora_unity_sdk

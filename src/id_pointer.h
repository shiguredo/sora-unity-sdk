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
  static IdPointer& Instance();
  ptrid_t Register(void* p);
  void Unregister(ptrid_t id);
  void* Lookup(ptrid_t id);
};

}  // namespace sora

#endif  // SORA_ID_POINTER_H_INCLUDED

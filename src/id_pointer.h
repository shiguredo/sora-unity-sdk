#ifndef SORA_UNITY_SDK_ID_POINTER_H_INCLUDED
#define SORA_UNITY_SDK_ID_POINTER_H_INCLUDED

#include <map>
#include <mutex>

#include "unity.h"

namespace sora_unity_sdk {

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

}  // namespace sora_unity_sdk

#endif

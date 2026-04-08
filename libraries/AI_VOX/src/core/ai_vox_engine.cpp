#include "ai_vox_engine_impl.h"

namespace ai_vox {
Engine& Engine::GetInstance() {
  return EngineImpl::GetInstance();
}
}  // namespace ai_vox
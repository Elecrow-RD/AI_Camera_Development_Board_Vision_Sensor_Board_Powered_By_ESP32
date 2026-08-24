#pragma once

#ifndef _AI_VOX_IOT_MANAGER_H_
#define _AI_VOX_IOT_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "iot_entity.h"

namespace ai_vox::iot {

class Manager {
 public:
  Manager() = default;
  ~Manager() = default;

  void RegisterEntity(std::shared_ptr<Entity> entity);
  std::vector<std::string> DescriptionsJson() const;
  std::vector<std::string> UpdatedJson(const bool force);

 private:
  std::unordered_map<std::string, Value> UpdateStates(const std::string& name, std::unordered_map<std::string, Value> states, const bool force);

  std::vector<std::shared_ptr<iot::Entity>> entities_;
  std::unordered_map<std::string, std::unordered_map<std::string, Value>> last_states_;
};
}  // namespace ai_vox::iot

#endif
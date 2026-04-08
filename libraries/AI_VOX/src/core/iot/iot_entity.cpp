#include "iot_entity.h"

namespace ai_vox::iot {
Entity::Entity(std::string name, std::string description, std::vector<Property> properties, std::vector<Function> functions)
    : description_(std::move(description)), name_(std::move(name)) {
  for (const auto& propertie : properties) {
    properties_.insert({propertie.name, std::move(propertie)});
  }

  for (const auto& function : functions) {
    functions_.insert({function.name, std::move(function)});
  }
}

void Entity::UpdateState(const std::string& name, const Value& value) {
  std::lock_guard<std::mutex> lock(mutex_);

  const auto& it = properties_.find(name);
  if (it == properties_.end() || it->second.type != static_cast<ValueType>(value.index())) {
    abort();
  }

  states_.insert_or_assign(name, value);
}

}  // namespace ai_vox::iot
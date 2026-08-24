#include "iot_manager.h"

#include "cJSON.h"
#include "core/clogger/clogger.h"

namespace ai_vox::iot {
void Manager::RegisterEntity(std::shared_ptr<Entity> entity) {
  entities_.emplace_back(std::move(entity));
}

std::vector<std::string> Manager::DescriptionsJson() const {
  std::vector<std::string> result;

  for (auto &entity : entities_) {
    auto const root_json = cJSON_CreateObject();
    cJSON_AddStringToObject(root_json, "session_id", "");
    cJSON_AddStringToObject(root_json, "type", "iot");
    cJSON_AddBoolToObject(root_json, "update", true);

    auto const descriptors_json = cJSON_CreateArray();

    auto const entity_json = cJSON_CreateObject();
    cJSON_AddStringToObject(entity_json, "name", entity->name().c_str());
    cJSON_AddStringToObject(entity_json, "description", entity->description().c_str());
    auto const properties_json = cJSON_CreateObject();
    for (auto &[_, property] : entity->properties()) {
      auto const property_json = cJSON_CreateObject();
      cJSON_AddStringToObject(property_json, "description", property.description.c_str());
      cJSON_AddStringToObject(property_json,
                              "type",
                              property.type == iot::ValueType::kBool ? "boolean" : (property.type == iot::ValueType::kString ? "string" : "number"));
      cJSON_AddItemToObject(properties_json, property.name.c_str(), property_json);
    }
    cJSON_AddItemToObject(entity_json, "properties", properties_json);

    auto const methods_json = cJSON_CreateObject();
    for (auto [_, function] : entity->functions()) {
      auto const function_json = cJSON_CreateObject();
      cJSON_AddStringToObject(function_json, "description", function.description.c_str());
      auto const parameters_json = cJSON_CreateObject();
      for (auto &parameter : function.parameters) {
        auto const parameter_json = cJSON_CreateObject();
        cJSON_AddStringToObject(parameter_json, "description", parameter.description.c_str());
        cJSON_AddStringToObject(
            parameter_json,
            "type",
            parameter.type == iot::ValueType::kBool ? "boolean" : (parameter.type == iot::ValueType::kString ? "string" : "number"));
        cJSON_AddItemToObject(parameters_json, parameter.name.c_str(), parameter_json);
      }
      cJSON_AddItemToObject(function_json, "parameters", parameters_json);
      cJSON_AddItemToObject(methods_json, function.name.c_str(), function_json);
    }
    cJSON_AddItemToObject(entity_json, "methods", methods_json);

    cJSON_AddItemToArray(descriptors_json, entity_json);
    cJSON_AddItemToObject(root_json, "descriptors", descriptors_json);

    char *const message = cJSON_PrintUnformatted(root_json);
    result.emplace_back(message);
    cJSON_free(message);
    cJSON_Delete(root_json);
  }

  return result;
}

std::vector<std::string> Manager::UpdatedJson(const bool force) {
  CLOGD("force: %d", force);
  std::vector<std::string> result;
  for (auto &entity : entities_) {
    const auto diff = UpdateStates(entity->name(), entity->states(), force);
    if (diff.empty()) {
      continue;
    }

    auto const root_json = cJSON_CreateObject();
    cJSON_AddStringToObject(root_json, "session_id", "");
    cJSON_AddStringToObject(root_json, "type", "iot");
    cJSON_AddBoolToObject(root_json, "update", true);

    auto const states_json = cJSON_CreateArray();
    auto const state_item_json = cJSON_CreateObject();
    cJSON_AddStringToObject(state_item_json, "name", entity->name().c_str());
    auto const state_json = cJSON_CreateObject();
    for (const auto &[key, value] : diff) {
      if (std::holds_alternative<bool>(value)) {
        cJSON_AddBoolToObject(state_json, key.c_str(), std::get<bool>(value));
      } else if (std::holds_alternative<std::string>(value)) {
        cJSON_AddStringToObject(state_json, key.c_str(), std::get<std::string>(value).c_str());
      } else if (std::holds_alternative<int64_t>(value)) {
        cJSON_AddNumberToObject(state_json, key.c_str(), std::get<int64_t>(value));
      }
    }
    cJSON_AddItemToObject(state_item_json, "state", state_json);
    cJSON_AddItemToArray(states_json, state_item_json);
    cJSON_AddItemToObject(root_json, "states", states_json);
    char *const message = cJSON_PrintUnformatted(root_json);
    result.push_back(message);
    cJSON_free(message);
    cJSON_Delete(root_json);
  }

  return result;
}

std::unordered_map<std::string, Value> Manager::UpdateStates(const std::string &name,
                                                             std::unordered_map<std::string, Value> states,
                                                             const bool force) {
  if (force) {
    last_states_.insert_or_assign(name, states);
    return states;
  }

  const auto last_states_it = last_states_.find(name);
  if (last_states_it == last_states_.end()) {
    last_states_.emplace(name, states);
    return states;
  } else {
    std::unordered_map<std::string, Value> diff;
    for (const auto &[key, value] : states) {
      const auto &last_state_it = last_states_it->second.find(key);
      if (last_state_it == last_states_it->second.end() || last_state_it->second != value) {
        diff.emplace(key, value);
      }
    }
    last_states_it->second = states;
    return diff;
  }
}
}  // namespace ai_vox::iot
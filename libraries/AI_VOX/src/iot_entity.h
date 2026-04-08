#pragma once

#ifndef _AI_VOX_IOT_ENTITY_H_
#define _AI_VOX_IOT_ENTITY_H_

#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ai_vox::iot {

enum class ValueType : uint8_t {
  kBool,
  kString,
  kNumber,
};

using Value = std::variant<bool, std::string, int64_t>;

struct Parameter {
  std::string name;
  std::string description;
  ValueType type;
  bool required;
};

struct Function {
  std::string name;
  std::string description;
  std::vector<Parameter> parameters;
};

struct Property {
  std::string name;
  std::string description;
  ValueType type;
};

class Entity {
 public:
  Entity(std::string name, std::string description, std::vector<Property> properties, std::vector<Function> functions);

  virtual ~Entity() = default;

  inline const std::string& name() const {
    return name_;
  }

  inline const std::string& description() const {
    return description_;
  }

  void UpdateState(const std::string& name, const Value& value);

  inline const std::unordered_map<std::string, Property>& properties() const {
    return properties_;
  }

  inline const std::unordered_map<std::string, Function>& functions() const {
    return functions_;
  }

  inline const std::unordered_map<std::string, Value>& states() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return states_;
  }

 private:
  mutable std::mutex mutex_;
  const std::string description_;
  const std::string name_;
  std::unordered_map<std::string, Property> properties_;
  std::unordered_map<std::string, Function> functions_;
  std::unordered_map<std::string, Value> states_;
};

}  // namespace ai_vox::iot

#endif
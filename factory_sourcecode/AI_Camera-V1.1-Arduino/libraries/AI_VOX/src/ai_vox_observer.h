#pragma once

#ifndef _AI_VOX_OBSERVER_H_
#define _AI_VOX_OBSERVER_H_

#include <deque>
#include <mutex>
#include <string>
#include <variant>

#include "iot_entity.h"

namespace ai_vox {

enum class ChatState : uint8_t {
  kIdle,
  kIniting,
  kStandby,
  kConnecting,
  kListening,
  kSpeaking,
};

enum class ChatRole : uint8_t {
  kAssistant,
  kUser,
};

class Observer {
 public:
  static constexpr size_t kMaxQueueSize = 10;

  struct StateChangedEvent {
    ChatState old_state;
    ChatState new_state;
  };

  struct ChatMessageEvent {
    ChatRole role;
    std::string content;
  };

  struct ActivationEvent {
    std::string code;
    std::string message;
  };

  struct EmotionEvent {
    std::string emotion;
  };

  struct IotMessageEvent {
    std::string name;
    std::string function;
    std::map<std::string, iot::Value> parameters;
  };

  using Event = std::variant<StateChangedEvent, ActivationEvent, ChatMessageEvent, EmotionEvent, IotMessageEvent>;

  Observer() = default;
  virtual ~Observer() = default;

  virtual std::deque<Event> PopEvents() {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::move(event_queue_);
  }

  virtual void PushEvent(Event&& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (event_queue_.size() >= kMaxQueueSize) {
      event_queue_.pop_front();
    }
    event_queue_.emplace_back(std::move(event));
  }

 private:
  Observer(const Observer&) = delete;
  Observer& operator=(const Observer&) = delete;

  mutable std::mutex mutex_;
  std::deque<Event> event_queue_;
};

}  // namespace ai_vox

#endif
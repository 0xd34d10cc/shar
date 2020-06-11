#pragma once

#include <string>

// windows...
#ifdef MessageBox
#undef MessageBox
#endif

namespace shar::ui {

class MessageBox {
public:
  enum class Type { Error, Warning, Info };

  MessageBox(const MessageBox&) = delete;
  MessageBox(MessageBox&&) = default;
  MessageBox& operator=(const MessageBox&) = delete;
  MessageBox& operator=(MessageBox&&) = default;
  ~MessageBox() = default;

  static MessageBox error(std::string title, std::string error) {
    return MessageBox(Type::Error, std::move(title), std::move(error));
  }

  static MessageBox warning(std::string title, std::string error) {
    return MessageBox(Type::Warning, std::move(title), std::move(error));
  }

  static MessageBox info(std::string title, std::string error) {
    return MessageBox(Type::Info, std::move(title), std::move(error));
  }

  // returns true on success
  bool show();

private:
  MessageBox(Type type, std::string title, std::string message);

  Type m_type;
  std::string m_title;
  std::string m_message;
};

} // namespace shar::ui
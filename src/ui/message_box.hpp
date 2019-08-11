#pragma once

#include <string>

// windows...
#ifdef MessageBox
#undef MessageBox
#endif

namespace shar::ui {

class MessageBox {
public:
  enum class Type {
    Error,
    Warning,
    Info
  };

  MessageBox(Type type, std::string title, std::string message);
  MessageBox(const MessageBox&) = delete;
  MessageBox(MessageBox&&) = default;
  MessageBox& operator=(const MessageBox&) = delete;
  MessageBox& operator=(MessageBox&&) = default;
  ~MessageBox() = default;

  int show();
private:
  Type m_type;
  std::string m_title;
  std::string m_message;
};

}
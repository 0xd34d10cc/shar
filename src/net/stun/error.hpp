#pragma once

#include <system_error>


namespace shar::net::stun {

enum class Error {
  Success = 0,
  InvalidMessage,
  UnknownRequestId,
  RequestFailed,
  NoAddress
};

// Define a custom error code category derived from std::error_category
class ErrorCategory: public std::error_category
{
public:
  // Return a short descriptive name for the category
  virtual const char* name() const noexcept override final {
    return "stun::Error";
  }

  // Return what each enum means in text
  virtual std::string message(int c) const override final {
    switch (static_cast<Error>(c))
    {
    case Error::Success:
      return "success";
    case Error::InvalidMessage:
      return "invalid message";
    case Error::UnknownRequestId:
      return "unknown request id";
    case Error::RequestFailed:
      return "request failed";
    case Error::NoAddress:
      return "no ip address in response";
    default:
      return "unknown";
    }
  }
};

inline const ErrorCategory& error_category() {
  static ErrorCategory c;
  return c;
}

}

// Overload the global make_error_code() free function with our
// custom enum. It will be found via ADL by the compiler if needed.
inline std::error_code make_error_code(shar::net::stun::Error e) {
  return { static_cast<int>(e), shar::net::stun::error_category() };
}

namespace std {
// Tell the C++ 11 STL metaprogramming that enum ConversionErrc
// is registered with the standard error code system
template <> struct is_error_code_enum<shar::net::stun::Error> : true_type {};
}

#include <system_error>


namespace shar::net::rtsp {

enum class Error {
  success = 0,
  notEnoughData,
  invalidAddress,
  invalidProtocol,
  invalidHeader,
  invalidStatusCode,
  invalidType,
  excessHeaders,
  missingCRLF,
  unitializedField
};

class ErrorCategory : public std::error_category {
  virtual const char* name() const noexcept override final {
    return "net::rtsp";
  }

  virtual const char* message(int c) const override final {
    switch (static_cast<Error>(c)) {
    case Error::success:
      return "success";
    case Error::notEnoughData:
      return "need more data";
    case Error::invalidAddress:
      return "invalid address";
    case Error::invalidProtocol:
      return "invalid protocol";
    case Error::invalidHeader:
      return "invalid header";
    case Error::invalidStatusCode:
      return "invalid status code";
    case Error::invalidType:
      return "invalid request type";
    case Error::excessHeaders:
      return "too many headers";
    case Error::missingCRLF:
      return "no CRLF found";
    case Error::unitializedField:
      return "one of filed is not initialized";
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
  return { static_cast<int>(e), shar::net::rtsp::error_category() };
}

namespace std {
// Tell the C++ 11 STL metaprogramming that enum ConversionErrc
// is registered with the standard error code system
template <> struct is_error_code_enum<shar::net::rtsp::Error> : true_type {};
}
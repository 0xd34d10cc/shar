#include <system_error>


namespace shar::net::rtsp {

enum class Error {
  Success = 0,
  NotEnoughData,
  InvalidAddress,
  InvalidProtocol,
  InvalidHeader,
  InvalidStatusCode,
  InvalidType,
  ExcessHeaders,
  MissingCRLF
};

class ErrorCategory : public std::error_category {
  virtual const char* name() const noexcept override final {
    return "net::rtsp";
  }

  virtual std::string message(int c) const override final {
    switch (static_cast<Error>(c)) {
    case Error::Success:
      return "success";
    case Error::NotEnoughData:
      return "need more data";
    case Error::InvalidAddress:
      return "invalid address";
    case Error::InvalidProtocol:
      return "invalid protocol";
    case Error::InvalidHeader:
      return "invalid header";
    case Error::InvalidStatusCode:
      return "invalid status code";
    case Error::InvalidType:
      return "invalid request type";
    case Error::ExcessHeaders:
      return "too many headers";
    case Error::MissingCRLF:
      return "no CRLF found";
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
inline std::error_code make_error_code(shar::net::rtsp::Error e) {
  return { static_cast<int>(e), shar::net::rtsp::error_category() };
}

namespace std {
// Tell the C++ 11 STL metaprogramming that enum ConversionErrc
// is registered with the standard error code system
template <> struct is_error_code_enum<shar::net::rtsp::Error> : true_type {};
}
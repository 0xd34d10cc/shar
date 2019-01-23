#pragma once

#include "slice.hpp"


namespace shar::rtcp {

enum ItemType: std::uint8_t {
  END = 0,
  CNAME = 1,
  NAME = 2,
  EMAIL = 3,
  PHONE = 4,
  LOCATION = 5,
  TOOL = 6,
  NOTE = 7,
  PRIVATE = 8
};

// Source items structure:
//
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// chunk  |                          SSRC/CSRC_1                          |
//   1    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                           SDES items                          |
//        |                              ...                              |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// chunk  |                          SSRC/CSRC_2                          |
//   2    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                           SDES items                          |
//        |                              ...                              |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
class SourceItems: public Slice {
public:
  static const std::size_t NWORDS = 1;
  static const std::size_t MIN_SIZE = NWORDS * sizeof(std::uint32_t);

  SourceItems() noexcept = default;
  SourceItems(std::uint8_t* data, std::size_t size) noexcept;
  SourceItems(const SourceItems&) noexcept = default;
  SourceItems(SourceItems&&) noexcept = default;
  SourceItems& operator=(const SourceItems&) noexcept = default;
  SourceItems& operator=(SourceItems&&) noexcept = default;
  ~SourceItems() = default;

  bool valid() const noexcept;

  // SSRC: 32 bits
  //  The synchronization source identifier for the 
  //  originator of this packet.
  std::uint32_t stream_id() const noexcept;
  void set_stream_id(std::uint32_t stream_id) noexcept;

  // Item
  //
  //  0               1               2               3
  //  7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
  // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  // |    type       |     length    |     item-specific data        |
  // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  class Item: private Slice {
  public:
    Item() noexcept = default;
    Item(std::uint8_t* data, std::size_t size) noexcept;
    Item(const Item&) noexcept = default;
    Item(Item&&) noexcept = default;
    Item& operator=(const Item&) noexcept = default;
    Item& operator=(Item&&) noexcept = default;
    ~Item() = default;

    std::uint8_t type() const noexcept;
    void set_type(std::uint8_t type) noexcept;

    std::uint8_t length() const noexcept;
    void set_length(std::uint8_t len) noexcept;

    std::uint8_t* data() noexcept;
    const std::uint8_t* data() const noexcept;  
  };

  // move to next item
  // NOTE: should not be called if item is not initialized
  Item next() noexcept;

  // write Item with |type| and |len| at current position
  Item set(std::uint8_t type, std::uint8_t len) noexcept;

  // reset poistion to start of items list
  void reset() noexcept;
  std::size_t position() const noexcept;

protected:
  std::size_t m_position{MIN_SIZE};
};

}
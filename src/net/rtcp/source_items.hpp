#pragma once

#include "bytes_ref.hpp"


namespace shar::net::rtcp {

enum ItemType: u8 {
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
class SourceItems: public BytesRefMut {
public:
  static const usize NWORDS = 1;
  static const usize MIN_SIZE = NWORDS * sizeof(u32);

  SourceItems() noexcept = default;
  SourceItems(u8* data, usize size) noexcept;
  SourceItems(const SourceItems&) noexcept = default;
  SourceItems(SourceItems&&) noexcept = default;
  SourceItems& operator=(const SourceItems&) noexcept = default;
  SourceItems& operator=(SourceItems&&) noexcept = default;
  ~SourceItems() = default;

  bool valid() const noexcept;

  // SSRC: 32 bits
  //  The synchronization source identifier for the
  //  originator of this packet.
  u32 stream_id() const noexcept;
  void set_stream_id(u32 stream_id) noexcept;

  // Item
  //
  //  0               1               2               3
  //  7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
  // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  // |    type       |     length    |     item-specific data        |
  // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  class Item: private BytesRefMut {
  public:
    Item() noexcept = default;
    Item(u8* data, usize size) noexcept;
    Item(const Item&) noexcept = default;
    Item(Item&&) noexcept = default;
    Item& operator=(const Item&) noexcept = default;
    Item& operator=(Item&&) noexcept = default;
    ~Item() = default;

    u8 type() const noexcept;
    void set_type(u8 type) noexcept;

    u8 length() const noexcept;
    void set_length(u8 len) noexcept;

    u8* data() noexcept;
    const u8* data() const noexcept;
  };

  // move to next item
  // NOTE: should not be called if item is not initialized
  Item next() noexcept;

  // write Item with |type| and |len| at current position
  Item set(u8 type, u8 len) noexcept;

  // reset poistion to start of items list
  void reset() noexcept;
  usize position() const noexcept;

protected:
  usize m_position{MIN_SIZE};
};

}

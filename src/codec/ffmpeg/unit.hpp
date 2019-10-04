#pragma once

#include <memory>

#include "int.hpp"


// forward declarations from avcodec.h
extern "C" {
struct AVPacket;
void av_packet_free(AVPacket **pkt);
}

namespace shar::codec::ffmpeg {

// h264 nal unit
class Unit {
public:
  enum class Type {
    Unknown,
    IDR      // Instantaneous Decoder Refresh
  };

  Unit() = default;
  Unit(const Unit&) = delete;
  Unit(Unit&&) noexcept = default;
  Unit& operator=(Unit&&) noexcept = default;
  Unit& operator=(const Unit&) = delete;
  ~Unit() = default;

  // allocate empty unit
  static Unit allocate() noexcept;

  // create unit from data (NOTE: memcopy)
  static Unit from_data(const u8* data, usize size);

  bool empty() const noexcept;
  const u8* data() const noexcept;
  u8* data() noexcept;

  usize size() const noexcept;
  u32 timestamp() const noexcept;
  Type type() const noexcept;

  AVPacket* raw() noexcept;

private:
  Unit(AVPacket* packet);

  struct PacketDeleter {
    void operator()(AVPacket* packet) {
      av_packet_free(&packet);
    }
  };

  using PacketPtr = std::unique_ptr<AVPacket, PacketDeleter>;
  PacketPtr m_packet;
};

}
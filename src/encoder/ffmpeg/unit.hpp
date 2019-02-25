#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>


// forward declarations from avcodec.h
extern "C" {
struct AVPacket;
void av_packet_free(AVPacket **pkt);
}

namespace shar::encoder::ffmpeg {

// h264 nal unit
// TODO: make IUnit interface
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

  static Unit allocate() noexcept;

  bool empty() const noexcept;
  const std::uint8_t* data() const noexcept;
  std::uint8_t* data() noexcept;

  std::size_t size() const noexcept;
  std::uint32_t timestamp() const noexcept;
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
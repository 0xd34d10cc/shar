#include "codecs/ffmpeg/decoder.hpp"
#include "codecs/ffmpeg/encoder.hpp"


namespace ff = shar::codecs::ffmpeg;

int main() {
  auto logger = Logger("ffmpeg_test.log");
  ff::Encoder encoder {{1080, 1920}, logger, 11000 * 1024, 30};
  ff::Decoder decoder {logger};

  return EXIT_SUCCESS;
}
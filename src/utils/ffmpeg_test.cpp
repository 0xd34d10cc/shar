#include "codecs/ffmpeg/decoder.hpp"
#include "codecs/ffmpeg/encoder.hpp"


namespace ff = shar::codecs::ffmpeg;

int main() {
  ff::Encoder encoder {{1080, 1920}, 11000 * 1024, 30};
  ff::Decoder decoder;

  return EXIT_SUCCESS;
}
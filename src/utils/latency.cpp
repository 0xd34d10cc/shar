#include <thread>
#include <iostream>
#include <vector>
#include <cstddef>
#include <chrono>

#include "queues/frames_queue.hpp"
#include "queues/packets_queue.hpp"
#include "processors/h264encoder.hpp"
#include "processors/h264decoder.hpp"
#include "processors/frame_file_reader.hpp"


using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;

struct LatencyInfo {
  TimePoint start;
  TimePoint end;
};

int main() {
  auto logger = shar::Logger("latency_test.log");
  std::size_t total_frames = 120;
  std::size_t frame_rate = 60;

  shar::FramesQueue  input_frames;
  shar::FramesQueue  frames_after_timestamp;
  shar::PacketsQueue encoded_packets;
  shar::FramesQueue  output_frames;

  std::vector<LatencyInfo> entries;
  // to be sure that we will not have any reallocations
  entries.reserve(total_frames + 1);

  shar::Size            frame_size {1080, 1920};
  shar::FileParams      params {"example.bgra", frame_size, frame_rate };
  shar::FrameFileReader reader {std::move(params), logger, input_frames};

  auto config = shar::Config::make_default();
  shar::H264Encoder encoder {frame_size, frame_rate, config.get_subconfig("encoder"),
                             logger, frames_after_timestamp, encoded_packets};
  shar::H264Decoder decoder {encoded_packets, logger, output_frames};

  std::atomic<bool> reader_finished = false;
  std::thread       reader_thread {[&] {
    reader.run();
    reader_finished = true;
  }};

  std::thread start_timestamp_writer {[&] {
    std::size_t frame_number = 0;
    while (!reader_finished || !input_frames.empty()) {
      if (!input_frames.empty()) {
        do {
          auto* frame = input_frames.get_next();
          if (frame_number != total_frames - 1) {
            frame_number++;
            entries.push_back({Clock::now(), TimePoint()});
          }
          frames_after_timestamp.push(std::move(*frame));
          input_frames.consume(1);
        } while (!input_frames.empty());
      }
      input_frames.wait();
    }

    logger.info("Finished reading frames. Total: {0}", frame_number);
  }};

  std::thread encoder_thread {[&] {
    encoder.run();
  }};

  std::thread decoder_thread {[&] {
    decoder.run();
  }};

  std::size_t fps {0};
  shar::Timer timer {std::chrono::seconds(1)};
  // write end timestamp
  std::size_t frame_number          = 0;
  while (frame_number != total_frames - 1) {
    if (timer.expired()) {
      logger.info("fps: {}", fps);
      fps = 0;
      timer.restart();
    }

    if (!output_frames.empty()) {
      do {
        entries[frame_number].end = Clock::now();
        ++frame_number;
        ++fps;
        // frames are ignored here
        output_frames.consume(1);
      } while (!output_frames.empty());
    }

    output_frames.wait();
  }

  output_frames.set_consumer_state(shar::FramesQueue::State::Dead);

  if (entries.back().end == TimePoint()) {
    entries.back().end = Clock::now();
  }

  const auto to_ms = [](auto duration) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
  };

  frame_number = 0;
  for (const auto& entry: entries) {
    const auto latency = entry.end - entry.start;
    logger.info("{0}: {1}ms", frame_number, to_ms(latency).count());
    std::cout << frame_number << ": " << to_ms(latency).count() << "ms" << std::endl;
    frame_number++;
  }

  reader.stop();
  encoder.stop();
  decoder.stop();

  reader_thread.join();
  start_timestamp_writer.join();
  encoder_thread.join();
  decoder_thread.join();

  return 0;
}
#pragma once

#include "logger.hpp"

#include <cassert>
#include <filesystem>
#include <png.h>

namespace shar {

class PNGReader {
public:
  PNGReader(std::filesystem::path image_path, Logger& logger)
      : m_channels(0)
      , m_height(0)
      , m_width(0)
      , m_data(nullptr) {
    assert(!image_path.empty());
    if (FILE* fp = fopen(image_path.string().c_str(), "rb")) {
      png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                               nullptr,
                                               nullptr,
                                               nullptr);
      if (!png) {
        logger.error("Can not create png read struct");
        return;
      }
      png_infop info = png_create_info_struct(png);
      if (!info)
        logger.error("Can not create png info struct");

      if (setjmp(png_jmpbuf(png))) {
        logger.error("Internal error while parsing png file");
      }

      png_init_io(png, fp);

      png_read_info(png, info);

      m_width = png_get_image_width(png, info);
      m_height = png_get_image_height(png, info);

      auto color_type = png_get_color_type(png, info);
      auto bit_depth = png_get_bit_depth(png, info);

      // Read any color_type into 8bit depth, RGBA format.
      // See http://www.libpng.org/pub/png/libpng-manual.txt

      if (bit_depth == 16)
        png_set_strip_16(png);

      if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

      // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
      if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

      if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

      // These color_type don't have an alpha channel then fill it with 0xff.
      if (color_type == PNG_COLOR_TYPE_RGB ||
          color_type == PNG_COLOR_TYPE_GRAY ||
          color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xff, PNG_FILLER_AFTER);

      if (color_type == PNG_COLOR_TYPE_GRAY ||
          color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

      png_read_update_info(png, info);

      // After all our hacks with image we should have RGBA image
      assert(color_type == PNG_COLOR_TYPE_RGB_ALPHA);
      m_channels = 4;

      auto bytes_in_row = m_channels * m_width;
      auto row_pointers =
          reinterpret_cast<png_bytep*>(malloc(sizeof(png_bytep) * m_height));

      // flip RGBA to BGRA
      png_set_bgr(png);

      for (int y = 0; y < m_height; y++) {
        auto needed_bytes = png_get_rowbytes(png, info);
        assert(needed_bytes == bytes_in_row);
        row_pointers[y] = (png_byte*)malloc(needed_bytes);
      }
      png_read_image(png, row_pointers);

      // convert 2D data to 1D
      m_data = reinterpret_cast<std::uint8_t*>(
        malloc(static_cast<std::size_t>(m_width) * m_height * m_channels));

      for (auto i = 0; i < m_height; i++) {
        memcpy(m_data + bytes_in_row * i, row_pointers[i], m_width * m_channels);
      }

      logger.info("Picture successfully open with path: {}", image_path.string());
      free(row_pointers);
      fclose(fp);

      png_destroy_read_struct(&png, &info, NULL);

    } else {
      logger.error("Can't open file with path: {}", image_path.string());
    }
  }

  ~PNGReader() {
    free(m_data);
  }

  std::uint8_t* get_data() {
    return m_data;
  }

  usize get_width() {
    return m_width;
  }

  usize get_height() {
    return m_height;
  }

  usize get_channels() {
    return m_channels;
  }

private:
  usize m_channels;
  usize m_width;
  usize m_height;
  std::uint8_t* m_data;
};

} // namespace shar
#include "png_reader.hpp"


namespace shar {

PNGReader::PNGReader(std::filesystem::path image_path, Logger& logger)
  : m_channels(0)
  , m_height(0)
  , m_width(0)
  , m_data(nullptr) {
  assert(!image_path.empty());
  bool is_valid_path = std::filesystem::exists(image_path) && std::filesystem::is_regular_file(image_path);
  if (is_valid_path;  FILE * fp = fopen(image_path.string().c_str(), "rb")) {
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
      nullptr,
      nullptr,
      nullptr);
    if (!png) {
      logger.error("Can not create png read struct");
      return;
    }
    png_infop info = png_create_info_struct(png);
    if (!info) {
      logger.error("Can not create png info struct");
      return;
    }
    if (setjmp(png_jmpbuf(png))) {
      logger.error("Internal error while parsing png file");
      return;
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
    if (color_type != PNG_COLOR_TYPE_RGB_ALPHA)
    {
      logger.error("Can't convert png to RGBA image");
      return;
    }
    m_channels = 4;

    auto bytes_in_row = m_channels * m_width;
    m_data = std::move(std::make_unique<std::uint8_t[]>(bytes_in_row * m_height));
    std::unique_ptr<png_bytep[]> row_pointers =
      std::make_unique<png_bytep[]>(sizeof(png_bytep) * m_height);

    // flip RGBA to BGRA
    png_set_bgr(png);

    for (int y = 0; y < m_height; y++) {
      auto needed_bytes = png_get_rowbytes(png, info);
      // Might happen in case of corrupted png image. It's very rare case
      assert(needed_bytes == bytes_in_row);
      row_pointers[y] = m_data.get() + y * needed_bytes;
    }
    png_read_image(png, row_pointers.get());

    logger.info("Picture successfully open with path: {}", image_path.string());
    fclose(fp);

    png_destroy_read_struct(&png, &info, NULL);
  }
  else {
    logger.error("Can't open file with path: {}", image_path.string());
  }
}


std::unique_ptr<std::uint8_t[]> PNGReader::get_data() {
  return std::move(m_data);
}

usize PNGReader::get_width() {
  return m_width;
}

usize PNGReader::get_height() {
  return m_height;
}

usize PNGReader::get_channels() {
  return m_channels;
}


}
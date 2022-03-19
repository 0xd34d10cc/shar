#include "png_image.hpp"

// clang-format off
#include "disable_warnings_push.hpp"
#include <png.h>
#include "disable_warnings_pop.hpp"
// clang-format on

namespace shar {

PNGImage::PNGImage(std::filesystem::path path) {
  assert(!path.empty());

  using FilePtr = std::unique_ptr<FILE, decltype(&std::fclose)>;
  const auto fp = FilePtr(
      std::fopen(path.string().c_str(), "rb"),
      &std::fclose
  );
  if (!fp) {
    g_logger.error("Failed to open png file: {}", path.string());
    return;
  }

  const auto free_png = [](png_struct* p) {
    png_destroy_read_struct(&p, nullptr, nullptr);
  };
  using PngPtr = std::unique_ptr<png_struct, decltype(free_png)>;
  const auto png = PngPtr(
      png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr),
      free_png
  );

  if (!png) {
    g_logger.error("Cannot create png read struct");
    return;
  }

  const auto free_info = [png=png.get()](png_info* p) {
      png_destroy_info_struct(png, &p);
  };
  using PngInfoPtr = std::unique_ptr<png_info, decltype(free_info)>;
  const auto info = PngInfoPtr(png_create_info_struct(png.get()), free_info);

  if (!info) {
    g_logger.error("Can not create png info struct");
    return;
  }

  if (setjmp(png_jmpbuf(png.get()))) {
    g_logger.error("Internal error while parsing png file");
    // TODO: check if destructors are actually invoked in this case
    //       see C4611 msvc warning. Most likely all objects constructed
    //       below this if statement leak.
    return;
  }

  png_init_io(png.get(), fp.get());
  png_read_info(png.get(), info.get());

  const auto width = png_get_image_width(png.get(), info.get());
  const auto height = png_get_image_height(png.get(), info.get());
  m_size = Size{height, width};

  const auto color_type = png_get_color_type(png.get(), info.get());
  const auto bit_depth = png_get_bit_depth(png.get(), info.get());

  // Read any color_type into 8bit depth, RGBA format.
  // See http://www.libpng.org/pub/png/libpng-manual.txt

  if (bit_depth == 16)
    png_set_strip_16(png.get());

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png.get());

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png.get());

  if (png_get_valid(png.get(), info.get(), PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png.get());

  // These color_type don't have an alpha channel then fill it with 0xff.
  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png.get(), 0xff, PNG_FILLER_AFTER);

  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png.get());

  png_read_update_info(png.get(), info.get());

  // After all our hacks with image we should have RGBA image
  if (color_type != PNG_COLOR_TYPE_RGB_ALPHA) {
    g_logger.error("Can't convert png to RGBA image");
    return;
  }

  m_channels = 4;
  const usize bytes_in_row = m_channels * m_size.width();
  m_data = std::make_unique<std::uint8_t[]>(bytes_in_row * m_size.height());
  auto row_pointers = std::make_unique<png_bytep[]>(m_size.height());

  const auto actual_rowbytes = png_get_rowbytes(png.get(), info.get());
  if (bytes_in_row != actual_rowbytes) {
    g_logger.error("Unexpected number of bytes in row of png image"
                 " (expected {}, got {})",
                 bytes_in_row,
                 actual_rowbytes);
    return;
  }

  // flip RGBA to BGRA
  png_set_bgr(png.get());

  for (int y = 0; y < m_size.height(); y++) {
    row_pointers[y] = m_data.get() + y * bytes_in_row;
  }
  png_read_image(png.get(), row_pointers.get());

  m_valid = true;
}


std::unique_ptr<std::uint8_t[]> PNGImage::extract_data() noexcept {
  return std::move(m_data);
}

usize PNGImage::width() const noexcept {
  return m_size.width();
}

usize PNGImage::height() const noexcept {
  return m_size.height();
}

usize PNGImage::nchannels() const noexcept {
  return m_channels;
}

Size PNGImage::size() const noexcept {
  return m_size;
}

bool PNGImage::valid() const noexcept {
  return m_valid && m_data != nullptr;
}

}
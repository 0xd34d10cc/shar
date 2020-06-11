#pragma once

#include <filesystem>
#include <png.h>
#include <cassert>

class PNGReader {
public:
	PNGReader(std::filesystem::path image_path)  
		: m_row_pointers(nullptr)
		, m_data(nullptr)
	{
		assert(!image_path.empty());
		if (FILE* fp = fopen(image_path.string().c_str(), "rb")) {
			png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
																							 nullptr,
																							 nullptr,
																							 nullptr);
			if (!png) {
				throw std::runtime_error("can not create png read struct");
			}
			png_infop info = png_create_info_struct(png);
			if (!info)
				throw std::runtime_error("can not create png info struct");
			
			if (setjmp(png_jmpbuf(png)))
				abort();

			png_init_io(png, fp);

			png_read_info(png, info);

			m_width = png_get_image_width(png, info);
			m_height = png_get_image_height(png, info);
			m_color_type = png_get_color_type(png, info);
			m_bit_depth = png_get_bit_depth(png, info);

			// Read any color_type into 8bit depth, RGBA format.
			// See http://www.libpng.org/pub/png/libpng-manual.txt

			if (m_bit_depth == 16)
				png_set_strip_16(png);

			if (m_color_type == PNG_COLOR_TYPE_PALETTE)
				png_set_palette_to_rgb(png);

			// PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
			if (m_color_type == PNG_COLOR_TYPE_GRAY && m_bit_depth < 8)
				png_set_expand_gray_1_2_4_to_8(png);

			if (png_get_valid(png, info, PNG_INFO_tRNS))
				png_set_tRNS_to_alpha(png);

			// These color_type don't have an alpha channel then fill it with 0xff.
			if (m_color_type == PNG_COLOR_TYPE_RGB ||
					m_color_type == PNG_COLOR_TYPE_GRAY ||
					m_color_type == PNG_COLOR_TYPE_PALETTE)
				png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
			
			if (m_color_type == PNG_COLOR_TYPE_GRAY ||
					m_color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
				png_set_gray_to_rgb(png);

			png_read_update_info(png, info);
			if (m_row_pointers)
				throw std::runtime_error("can't update png info");
			assert(m_color_type == PNG_COLOR_TYPE_RGB_ALPHA);
			
			m_channels = 4;
			auto bytes_in_row = m_channels * m_width;
			m_row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * m_height);

			for (int y = 0; y < m_height; y++) {
				auto needed_bytes = png_get_rowbytes(png, info);
				assert(needed_bytes == bytes_in_row);
				m_row_pointers[y] = (png_byte*)malloc(needed_bytes);
			}
			png_read_image(png, m_row_pointers);

			fclose(fp);

			png_destroy_read_struct(&png, &info, NULL);
		} else {
			throw std::runtime_error("can't open file");
		}
	}

	std::uint8_t* get_data() {
		if (m_data == nullptr) {
			m_data = reinterpret_cast<std::uint8_t*>(malloc(static_cast<std::size_t>(m_width)* m_height* m_channels));
			for (auto i = 0; i < m_height; i++) {
				memcpy(m_data + static_cast<std::size_t>(m_width) * m_channels * i, m_row_pointers[i], static_cast<std::size_t>(m_width) * m_channels);
			}
		}
		return m_data;
	}

	int get_width() {
		return m_width;
	}

	int get_height() {
		return m_height;
	}

	int get_channels() {
		return m_channels;
	}

private:
  int m_channels;
	int m_width;
	std::uint8_t* m_data;
	int m_height;
	png_byte m_color_type;
	png_byte m_bit_depth;
	png_bytep* m_row_pointers;
};
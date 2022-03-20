from conans import ConanFile, CMake


class Shar(ConanFile):
    settings = "os",  "compiler",  "build_type"

    requires = (
        "asio/1.16.1",            # netcode
        "ffmpeg/4.2.1",           # encoder
        "sdl/2.0.20",             # window, input, OpenGL loader
        "spdlog/1.9.2",           # logs
        "gtest/1.8.1",            # UTs
        "jsonformoderncpp/3.7.0", # config deserialization
        "cli11/1.9.1",            # command line options
        "nuklear/4.06.1",         # gui
        "libpng/1.6.37",          # png
        "miniupnpc/2.2.2",        # port forwarding
        "protobuf/3.19.2"         # serialization
    )

    generators = "cmake"

    default_options = {
        'ffmpeg:shared': False,
        'ffmpeg:avdevice': False,
        'ffmpeg:avcodec': True,
        'ffmpeg:avformat': False,
        'ffmpeg:swresample': False,
        'ffmpeg:swscale': False,
        'ffmpeg:postproc': False,
        'ffmpeg:avfilter': False,
        'ffmpeg:with_zlib': False,
        'ffmpeg:with_bzip2': False,
        'ffmpeg:with_lzma': False,
        'ffmpeg:with_libiconv': False,
        'ffmpeg:with_freetype': False,
        'ffmpeg:with_openjpeg': False,
        'ffmpeg:with_openh264': False,
        'ffmpeg:with_opus': False,
        'ffmpeg:with_vorbis': False,
        'ffmpeg:with_zeromq': False,
        'ffmpeg:with_sdl': False,
        'ffmpeg:with_libx264': True,
        'ffmpeg:with_libx265': False,
        'ffmpeg:with_libvpx': False,
        'ffmpeg:with_libmp3lame': False,
        'ffmpeg:with_libfdk_aac': False,
        'ffmpeg:with_libwebp': False,
        'ffmpeg:with_ssl': False,
        'ffmpeg:with_programs': False
    }

    def requirements(self):
        if self.settings.os == "Windows":
            pass
            # self.options["ffmpeg"].qsv = False
        elif self.settings.os == "Linux":
            self.options["ffmpeg"].pulse = False
            self.options["ffmpeg"].vaapi = False
            self.options["ffmpeg"].vdpau = False
            self.options["ffmpeg"].alsa = False

            self.options["sdl2"].alsa = False
            self.options["sdl2"].jack = False
            self.options["sdl2"].pulse = False
            self.options["sdl2"].nas = False
            self.options["sdl2"].esd = False

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

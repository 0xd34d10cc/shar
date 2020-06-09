from conans import ConanFile, CMake


class Shar(ConanFile):
    settings        = "os",  "compiler",  "build_type"

    requires        = (
        "asio/1.16.1",                                 # network
        "ScreenCaptureLite/16.1.0@0xd34d10cc/testing", # capture
        "ffmpeg/4.2.1@bincrafters/stable",             # encoder
        "sdl2/2.0.9@bincrafters/stable",               # window, input, OpenGL
        "spdlog/0.17.0@bincrafters/stable",            # logs
        "gtest/1.8.1@bincrafters/stable",              # UTs
        "jsonformoderncpp/3.5.0@vthiery/stable",       # config
        "CLI11/1.7.1@cliutils/stable",                 # command line options
        "nuklear/1.33.0@0xd34d10cc/testing"            # ui
    )

    generators      = "cmake"

    default_options = {
        "ffmpeg:iconv": False,
        "ffmpeg:x264": True,
        "ffmpeg:mp3lame": False,
        "ffmpeg:vpx": False,
        "ffmpeg:fdk_aac": False,
        "ffmpeg:freetype": False,
        "ffmpeg:opus": False,
        "ffmpeg:vorbis": False,
        "ffmpeg:x265": False,
        "ffmpeg:openh264": False,
        "ffmpeg:openjpeg": False,
        "ffmpeg:zlib": False,
        "ffmpeg:bzlib": False,
        "ffmpeg:lzma": False,
        "ffmpeg:openssl": False,
        "ffmpeg:webp": False
    }

    def requirements(self):
        if self.settings.os == "Windows":
            self.options["ffmpeg"].qsv = False
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

from conans import ConanFile, CMake


class Shar(ConanFile):
    settings        = "os",  "compiler",  "build_type"

    requires        = ("asio/1.12.0@bincrafters/stable ",
                       "spdlog/0.17.0@bincrafters/stable",
                       "gtest/1.8.1@bincrafters/stable",
                       "ScreenCaptureLite/16.1.0@0xd34d10cc/testing",
                       "ffmpeg/4.0@0xd34d10cc/testing",
                       # TODO: move to 0xd34d10cc
                       "prometheus-cpp/0.6.0@d34dpkgs/testing")

    generators      = "cmake"

    default_options = ("ffmpeg:fPIC     = True",
                       "ffmpeg:iconv    = False",
                       "ffmpeg:x264     = True",
                       "ffmpeg:mp3lame  = False",
                       "ffmpeg:vpx      = False",
                       "ffmpeg:fdk_aac  = False",
                       "ffmpeg:opus     = False",
                       "ffmpeg:vorbis   = False",
                       "ffmpeg:x265     = False",
                       "ffmpeg:openh264 = False",
                       "ffmpeg:openjpeg = False",
                       "ffmpeg:zlib     = False",
                       "ffmpeg:bzlib    = False",
                       "ffmpeg:lzma     = False")

    def requirements(self):
        if self.settings.os == "Windows":
            self.options["ffmpeg"].qsv = False
            self.options["ffmpeg"].amf = True
        elif self.settings.os == "Linux":
            self.options["ffmpeg"].pulse = False
            self.options["ffmpeg"].vaapi = False
            self.options["ffmpeg"].vdpau = False
            self.options["ffmpeg"].alsa = False

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
from conans import ConanFile, CMake


class Shar(ConanFile):
    settings        = "os",  "compiler",  "build_type"

    requires        = ("asio/1.12.0@bincrafters/stable",              # network
                       "ScreenCaptureLite/16.1.0@0xd34d10cc/testing", # capture
                       "ffmpeg/4.0@0xd34d10cc/testing",               # encoder
                       "sdl2/2.0.9@bincrafters/stable",               # window, input, OpenGL
                       "spdlog/0.17.0@bincrafters/stable",            # logs
                       "gtest/1.8.1@bincrafters/stable",              # UTs
                       # TODO: move to 0xd34d10cc
                       "jsonformoderncpp/3.5.0@vthiery/stable",       # config
                       "CLI11/1.7.1@cliutils/stable",                 # command line options
                       # TODO: make separate package at d34dpkgs with version > 4.0
                       "nuklear/1.33.0@shearer12345/testing")         # ui

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

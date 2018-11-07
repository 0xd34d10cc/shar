from conans import ConanFile, CMake

class Shar(ConanFile):
    settings        = "os",  "compiler",  "build_type"

    requires        = ("ScreenCaptureLite/16.1.0@0xd34d10cc/testing",
                       "glfw/3.2.1.20180327@bincrafters/stable",
                       "boost/1.67.0@conan/stable",
                       "ffmpeg/4.0@pl0q1n/testing",
                       "spdlog/0.17.0@bincrafters/stable",
                       "miniupnpc/2.1@0xd34d10cc/testing")

    generators      = "cmake",  "gcc",  "txt"

    default_options = ("boost:without_math           = True",
                       "boost:without_wave           = True",
                       "boost:without_container      = True",
                       "boost:without_exception      = True",
                       "boost:without_graph          = True",
                       "boost:without_iostreams      = True",
                       "boost:without_locale         = True",
                       "boost:without_log            = True",
                       "boost:without_random         = True",
                       "boost:without_regex          = True",
                       "boost:without_mpi            = True",
                       "boost:without_serialization  = True",
                       "boost:without_signals        = True",
                       "boost:without_coroutine      = True",
                       "boost:without_fiber          = True",
                       "boost:without_context        = True",
                       "boost:without_timer          = True",
                       "boost:without_thread         = True",
                       "boost:without_chrono         = True",
                       "boost:without_date_time      = True",
                       "boost:without_atomic         = True",
                       "boost:without_filesystem     = True",
                       "boost:without_graph_parallel = True",
                       "boost:without_python         = True",
                       "boost:without_stacktrace     = True",
                       "boost:without_test           = True",
                       "boost:without_type_erasure   = True",
                       "boost:shared                 = False",
                       "ffmpeg:fPIC                  = True",
                       "ffmpeg:iconv                 = False",
                       "ffmpeg:x264                  = True",
                       "ffmpeg:mp3lame               = False",
                       "ffmpeg:vpx                   = False",
                       "ffmpeg:fdk_aac               = False",
                       "ffmpeg:opus                  = False",
                       "ffmpeg:vorbis                = False",
                       "ffmpeg:x265                  = False",
                       "ffmpeg:openh264              = False",
                       "ffmpeg:openjpeg              = False",
                       "ffmpeg:zlib                  = False",
                       "ffmpeg:bzlib                 = False",
                       "ffmpeg:lzma                  = False")

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

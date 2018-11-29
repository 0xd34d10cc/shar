from conans import ConanFile, CMake


BOOST_LIBS = [
    'math', 
    'wave', 
    'container', 
    'contract', 
    'exception', 
    'graph', 
    'iostreams', 
    'locale', 
    'log',
    'program_options', 
    'random', 
    'regex', 
    'mpi', 
    'serialization', 
    'signals',
    'coroutine', 
    'fiber', 
    'context', 
    'timer', 
    'thread', 
    'chrono', 
    'date_time',
    'atomic', 
    'filesystem', 
    'system', 
    'graph_parallel', 
    'python',
    'stacktrace', 
    'test', 
    'type_erasure'
]

ENABLED_BOOST_LIBS = [
    "system" # required for asio
]

BOOST_OPTIONS = tuple(['boost:without_{}=True'.format(lib) 
                       for lib in BOOST_LIBS 
                       if lib not in ENABLED_BOOST_LIBS])


class Shar(ConanFile):
    settings        = "os",  "compiler",  "build_type"

    requires        = ("ScreenCaptureLite/16.1.0@0xd34d10cc/testing",
                       "boost/1.67.0@conan/stable",
                       "ffmpeg/4.0@0xd34d10cc/testing",
                       "spdlog/0.17.0@bincrafters/stable",
                       "prometheus-cpp/0.6.0@d34dpkgs/testing")

    generators      = "cmake",  "gcc",  "txt"

    default_options = BOOST_OPTIONS +\
                      ("boost:shared    = False",
                       "ffmpeg:fPIC     = True",
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

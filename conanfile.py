from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain


class ReVCConan(ConanFile):
    name = "revc"
    version = "master"
    license = "???"  # FIXME: https://github.com/GTAmodding/re3/issues/794
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "audio": ["openal", "miles"],
        "with_libsndfile": [True, False],
        "with_opus": [True, False],
    }
    default_options = {
        "audio": "openal",
        "with_libsndfile": False,
        "with_opus": False,
    }
    exports_sources = "CMakeLists.txt", "cmake/*", "src/*", "gamefiles/*"

    def configure(self):
        if self.options.audio != "openal":
            self.options.with_libsndfile = False

    def requirements(self):
        self.requires(f"librw/{self.version}")
        self.requires("mpg123/1.31.2")
        if self.options.audio == "openal":
            self.requires("openal-soft/1.24.3")
        elif self.options.audio == "miles":
            self.requires(f"miles-sdk/{self.version}")
        if self.options.with_libsndfile:
            self.requires("libsndfile/1.2.2")
        if self.options.with_opus:
            self.requires("opusfile/0.12")

    def validate(self):
        librw_opts = self.dependencies["librw"].options
        if librw_opts.platform == "gl3" and librw_opts.get_safe("gl3_gfxlib") != "glfw":
            raise ConanInvalidConfiguration("Only `glfw` is supported as gl3_gfxlib.")

    @property
    def _reVC_audio(self):
        return {
            "miles": "MSS",
            "openal": "OAL",
        }[str(self.options.audio)]

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["REVC_AUDIO"] = self._reVC_audio
        tc.variables["REVC_WITH_OPUS"] = bool(self.options.with_opus)
        tc.variables["REVC_WITH_LIBSNDFILE"] = bool(self.options.with_libsndfile)
        tc.variables["REVC_INSTALL"] = True
        tc.variables["REVC_VENDORED_LIBRW"] = False
        tc.cache_variables["CMAKE_POLICY_VERSION_MINIMUM"] = "3.5"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

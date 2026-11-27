from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
import os


class LibrwConan(ConanFile):
    name = "librw"
    version = "master"
    license = "MIT"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "platform": ["null", "gl3", "d3d9", "ps2"],
        "gl3_gfxlib": ["glfw", "sdl2"],
    }
    default_options = {
        "platform": "gl3",
        "gl3_gfxlib": "glfw",
    }
    exports_sources = (
        "cmake/*",
        "skeleton/*",
        "src/*",
        "tools/*",
        "args.h",
        "rw.h",
        "CMakeLists.txt",
        "LICENSE",
    )

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.platform = "d3d9"

    def configure(self):
        if self.options.platform != "gl3":
            self.options.rm_safe("gl3_gfxlib")

    def validate(self):
        if self.options.platform == "d3d9" and self.settings.os != "Windows":
            raise ConanInvalidConfiguration("platform=d3d9 can only be built for os=Windows")

    def requirements(self):
        if self.options.platform == "gl3":
            if self.options.gl3_gfxlib == "glfw":
                self.requires("glfw/3.3.8")
            elif self.options.gl3_gfxlib == "sdl2":
                self.requires("sdl/2.32.10")

    def layout(self):
        cmake_layout(self)

    @property
    def _librw_platform(self):
        return {
            "null": "NULL",
            "gl3": "GL3",
            "d3d9": "D3D9",
            "ps2": "PS2",
        }[str(self.options.platform)]

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["LIBRW_PLATFORM"] = self._librw_platform
        tc.variables["LIBRW_INSTALL"] = True
        tc.variables["LIBRW_TOOLS"] = False
        tc.cache_variables["CMAKE_POLICY_VERSION_MINIMUM"] = "3.5"
        if self.options.platform == "gl3":
            tc.variables["LIBRW_GL3_GFXLIB"] = str(self.options.gl3_gfxlib).upper()
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "none")
        self.cpp_info.builddirs.append(os.path.join("lib", "cmake", "librw"))

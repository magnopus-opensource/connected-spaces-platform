from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, CMake, cmake_layout

class CSP(ConanFile):
    name = "connected-spaces-platform"
    settings = "os", "compiler", "build_type", "arch"

    exports_sources = "CMakeLists.txt", "Library/*", "Tests/*", "cmake/*"

    def layout(self):
        # Explicitly set the provided generator
        # This fixes a bug with the xcode generator where conan doesn't configure correctly and points to the incorrect directory.
        cmake_layout(
            self,
            generator=self.conf.get("tools.cmake.cmaketoolchain:generator", default=None)
        )

    def requirements(self):
        self.requires("rapidjson/cci.20250205")
        self.requires("fmt/12.1.0")
        self.requires("asyncplusplus/1.2")
        self.requires("msgpack-cxx/7.0.0")
        self.requires("tinyspline/0.6.0")
        self.requires("glm/1.0.1")
        self.requires("gtest/1.11.0")
        self.requires("sole/1.0.4")

        # We use the Emscripten WebSockets API for Emscripten
        if self.settings.os != "Emscripten":
            self.requires("poco/1.14.2")

    def configure(self):
        # Disable unnecessary Poco modules as this is a big library with dependecies.
        # Even with these removed, Poco increases our install step time by ~5 minutes.
        if self.settings.os != "Emscripten":
            self.options["poco"].enable_crypto = True
            self.options["poco"].enable_netssl = True
            self.options["poco"].enable_xml = False
            self.options["poco"].enable_json = False
            self.options["poco"].enable_util = True
            self.options["poco"].enable_pdf = False
            self.options["poco"].enable_zip = False
            self.options["poco"].enable_data = False
            self.options["poco"].enable_data_mysql = False
            self.options["poco"].enable_data_postgresql = False
            self.options["poco"].enable_data_sqlite = False
            self.options["poco"].enable_mongodb = False
            self.options["poco"].enable_redis = False
            self.options["poco"].enable_prometheus = False
            self.options["poco"].enable_jwt = False
            self.options["poco"].enable_activerecord = False

        # We need to turn of the c++ interface for tinyspline, as this is linked to its javscript (embind) interface
        # which doesn't play well with our configuration for 2 reasons:
        # - We use incompatible emscripten link flags with tinyspline
        # - We use an incompatible emsdk, which causes compile errors with tinyspline
        self.options["tinyspline"].cxx = False


        # Enabling boost adds a lot of time to our conan install step
        # + emscripten builds fail to compile with it. Disable it.
        self.options["msgpack-cxx"].use_boost = False

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)

        # Generate conan presets file
        tc.user_presets_path = 'ConanPresets.json'

        tc.generate()
        
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

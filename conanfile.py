from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, CMake, cmake_layout

class CSP(ConanFile):
    name = "connected-spaces-platform"
    version = "0.1"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    exports_sources = "CMakeLists.txt", "Library/*", "Tests/*", "cmake/*"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("rapidjson/cci.20250205")
        self.requires("fmt/12.1.0")
        self.requires("asyncplusplus/1.2")
        self.requires("poco/1.14.2")
        self.requires("msgpack-cxx/7.0.0")
        self.requires("tinyspline/0.6.0")
        self.requires("glm/1.0.1")
        self.requires("gtest/1.11.0")
        self.requires("sole/1.0.4")

    def configure(self):
        # Disable unnecessary Ppoco modules as this is a big library with dependecies.
        # Even with these removed, Poco increases our install step time by ~5 minutes.
        self.options["poco"].enable_crypto = True
        self.options["poco"].enable_netssl = True
        self.options["poco"].enable_xml = True
        self.options["poco"].enable_json = True
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

        # Enabling boost adds a lot of time to our conan install step
        # + emscripten builds fail to compile with it. Disable it.
        self.options["msgpack-cxx"].use_boost = False

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        # We want to be able to pass shared to Conan to build csp as a shared library
        # This prevent conan configuring everything to be built as shared.
        tc.blocks.remove("shared")
        # This allows us to set shared libs through the conan install step and pass to cmake
        tc.cache_variables["CSP_BUILD_SHARED"] = self.options.shared

        tc.generate()
        
    #def build_requirements(self):
        #self.tool_requires("cmake/3.31.6")
        
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

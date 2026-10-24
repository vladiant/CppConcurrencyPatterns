from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout


class CppCon2023Conan(ConanFile):
    name = "cppcon2023"
    version = "0.1"
    package_type = "application"

    settings = "os", "compiler", "build_type", "arch"

    requires = (
        "benchmark/1.9.4",
        "boost/1.86.0",
        "gtest/1.15.0",
    )

    default_options = {
        "boost/*:header_only": True,
        "benchmark/*:shared": False,
        "gtest/*:build_gmock": False,
        "gtest/*:shared": False,
    }

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        toolchain = CMakeToolchain(self)
        toolchain.generate()

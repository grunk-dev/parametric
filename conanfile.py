from conans import ConanFile, CMake


class ParametricConan(ConanFile):
    name = "parametric"
    version = "0.1"
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of Parametric here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    exports_sources = "parametric/*"
    no_copy_source = True

    def package(self):
        self.copy("*.hpp", dst="include/", excludes=("optional.hpp", "core_impl.hpp"))
        self.copy("optional.hpp", dst="include/impl/optional.hpp")
        self.copy("core_impl.hpp", dst="include/impl/core_impl.hpp")

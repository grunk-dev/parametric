from conans import ConanFile, CMake
from conans.tools import load
import re

def get_version():
    try:
        content = load("CMakeLists.txt")
        version = re.search("project\(parametric VERSION (.*)\)", content).group(1)
        return version.strip()
    except Exception as e:
        return None

class ParametricConan(ConanFile):
    name = "parametric"
    version = get_version()
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

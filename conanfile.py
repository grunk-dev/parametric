# SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
# SPDX-FileCopyrightText: 2026 Martin Siggel <martin.siggel@dlr.de>
#
# SPDX-License-Identifier: Apache-2.0

from os.path import join
from conan import ConanFile
from conan.tools.files import load, copy
import re

def get_version(conanfile=None):
    try:
        content = load(conanfile, "CMakeLists.txt")
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

    def package_id(self):
         self.info.clear()

    def package(self):
        copy(self, "*.hpp", self.source_folder, join(self.package_folder, "include/"), excludes=("optional.hpp", "core_impl.hpp"))
        copy(self, "optional.hpp", self.source_folder, join(self.package_folder, "include/impl/optional.hpp"))
        copy(self, "core_impl.hpp", self.source_folder, join(self.package_folder, "include/impl/core_impl.hpp"))

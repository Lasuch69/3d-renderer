# Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md).
# Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# file: https://github.com/godotengine/godot/blob/master/glsl_builders.py

import os.path
from typing import Optional, Iterable


def generate_inline_code(
    input_lines: Iterable[str], insert_newline: bool = True
):
    """Take header data and generate inline code

    :param: input_lines: values for shared inline code
    :return: str - generated inline value
    """
    output = []
    for line in input_lines:
        if line:
            output.append(",".join(str(ord(c)) for c in line))
        if insert_newline:
            output.append("%s" % ord("\n"))
    output.append("0")
    return ",".join(output)


class HeaderStruct:
    def __init__(self):
        self.vertex_lines = []
        self.fragment_lines = []
        self.compute_lines = []

        self.vertex_included_files = []
        self.fragment_included_files = []
        self.compute_included_files = []

        self.reading = ""
        self.line_offset = 0
        self.vertex_offset = 0
        self.fragment_offset = 0
        self.compute_offset = 0


def include_file_in_header(
    filename: str, header_data: HeaderStruct, depth: int
) -> HeaderStruct:
    fs = open(filename, "r")
    line = fs.readline()

    while line:
        index = line.find("//")
        if index != -1:
            line = line[:index]

        if line.find("#[VERTEX]") != -1:
            header_data.reading = "vertex"
            line = fs.readline()
            header_data.line_offset += 1
            header_data.vertex_offset = header_data.line_offset
            continue

        if line.find("#[FRAGMENT]") != -1:
            header_data.reading = "fragment"
            line = fs.readline()
            header_data.line_offset += 1
            header_data.fragment_offset = header_data.line_offset
            continue

        if line.find("#[COMPUTE]") != -1:
            header_data.reading = "compute"
            line = fs.readline()
            header_data.line_offset += 1
            header_data.compute_offset = header_data.line_offset
            continue

        while line.find("#include ") != -1:
            includeline = line.replace("#include ", "").strip()[1:-1]

            if includeline.startswith("thirdparty/"):
                included_file = os.path.relpath(includeline)

            else:
                included_file = os.path.relpath(
                    os.path.dirname(filename) + "/" + includeline)

            if not included_file in header_data.vertex_included_files and header_data.reading == "vertex":
                header_data.vertex_included_files += [included_file]
                if include_file_in_header(included_file, header_data, depth + 1) is None:
                    print("Error in file '" + filename + "': #include " +
                          includeline + "could not be found!")
            elif not included_file in header_data.fragment_included_files and header_data.reading == "fragment":
                header_data.fragment_included_files += [included_file]
                if include_file_in_header(included_file, header_data, depth + 1) is None:
                    print("Error in file '" + filename + "': #include " +
                          includeline + "could not be found!")
            elif not included_file in header_data.compute_included_files and header_data.reading == "compute":
                header_data.compute_included_files += [included_file]
                if include_file_in_header(included_file, header_data, depth + 1) is None:
                    print("Error in file '" + filename + "': #include " +
                          includeline + "could not be found!")

            line = fs.readline()

        line = line.replace("\r", "").replace("\n", "")

        if header_data.reading == "vertex":
            header_data.vertex_lines += [line]
        if header_data.reading == "fragment":
            header_data.fragment_lines += [line]
        if header_data.reading == "compute":
            header_data.compute_lines += [line]

        line = fs.readline()
        header_data.line_offset += 1

    fs.close()

    return header_data


def build_header(
    filename: str, header_data: Optional[HeaderStruct] = None
) -> None:
    header_data = header_data or HeaderStruct()
    include_file_in_header(filename, header_data, 0)

    out_file = filename + ".gen.h"

    out_file_base = out_file
    out_file_base = out_file_base[out_file_base.rfind("/") + 1:]
    out_file_base = out_file_base[out_file_base.rfind("\\") + 1:]
    out_file_ifdef = out_file_base.replace(".", "_").upper() + "_RD"
    out_file_class = out_file_base.replace(".glsl.gen.h", "").title().replace(
        "_", "").replace(".", "") + "ShaderRD"

    if header_data.compute_lines:
        body_parts = [
            "static const char _computeCode[] = {\n%s\n\t\t};" %
            generate_inline_code(header_data.compute_lines),
            f'setup(nullptr, nullptr, _computeCode, "{out_file_class}");',
        ]
    else:
        body_parts = [
            "static const char _vertexCode[] = {\n%s\n\t\t};" %
            generate_inline_code(header_data.vertex_lines),
            "static const char _fragmentCode[] = {\n%s\n\t\t};" %
            generate_inline_code(header_data.fragment_lines),
            f'setup(_vertexCode, _fragmentCode, nullptr, "{out_file_class}");',
        ]

    body_content = "\n\t\t".join(body_parts)

    # Intended curly brackets are doubled so f-string doesn't eat them up.
    shader_template = f"""/* WARNING, THIS FILE WAS GENERATED, DO NOT EDIT */
#ifndef {out_file_ifdef}
#define {out_file_ifdef}

#include "shader_rd.h"

class {out_file_class} : public ShaderRD {{
public:
    {out_file_class}() {{
        {body_content}
    }}
}};

#endif
"""

    with open(out_file, "w") as fd:
        fd.write(shader_template)


def build_headers(source):
    for x in source:
        build_header(filename=str(x))


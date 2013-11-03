import os
import shutil
import sys

BINARY = {
  "cards.png": "MAIN_TEXTURE",
}

STRINGS = {
  "default.fragmentshader": "DEFAULT_FRAG_SHADER",
  "default.vertexshader": "DEFAULT_VERTEX_SHADER",
}

MASTER_HEADER = "resources_gen.h"
SIZE_SUFFIX = "_SIZE"

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
RES_DIR = os.path.join(ROOT, "res")
GEN_DIR = os.path.join(ROOT, "src", "generated")

def clean_up():
  for entry in os.listdir(GEN_DIR):
    file_path = os.path.join(GEN_DIR, entry)
    try:
      if os.path.isfile(file_path):
        print "Removing %s..." % file_path
        os.unlink(file_path)
    except Exception, e:
      print e
      sys.exit("Sorry, some exception!");

def generate_master_header(sizes):
  content = ["#pragma once\n", "// Generated file, do not modify!\n", "\n"]

  for v in STRINGS.values():
    content += ["extern \"C\" const char %s[];\n" % v]

  content += "\n"

  for k, v in sizes.items():
    content += ["extern \"C\" const unsigned char %s[];\n" % k]
    content += ["static const unsigned int  %s = %d;\n" % (k + SIZE_SUFFIX, v)]

  file_path = os.path.join(GEN_DIR, MASTER_HEADER)
  print "Generating %s..." % file_path;
  with open(file_path, "w") as f:
    f.writelines(content)

def generate_embedded_resources():
  sizes = {}    
  for k, v in BINARY.items() + STRINGS.items():
    file_path = os.path.join(RES_DIR, k)
    print "Generating %s..." % file_path;

    if not os.path.isfile(file_path):
      sys.exit("Sorry, %s is not a file!" % file_path)
    bin_content = open(file_path, "rb").read()
    if k in BINARY:
      sizes[v] = len(bin_content)
    else:
      bin_content += '\0'

    content = []
    content += ["// Generated file, do not modify!\n", "\n", "extern const unsigned char %s[] = {\n" % v]
    line = "    "
    for i, b in enumerate(bin_content):
      if i > 0 and i % 16 == 0:
        content += [line + "\n"]
        line = "    "
      line += "0x%02X, " % ord(b)
    content += [line+"\n", "};\n"]

    res_file_path = os.path.join(GEN_DIR, k + ".c")
    with open(res_file_path, "w") as f:
      f.writelines(content)

  generate_master_header(sizes)

if __name__ == "__main__":
  if not os.path.exists(GEN_DIR):
    os.mkdir(GEN_DIR)
  clean_up()
  generate_embedded_resources()

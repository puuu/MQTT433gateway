#!/usr/bin/env python

"""MQTT433gateway web interface builder

Project home: https://github.com/puuu/MQTT433gateway/
"""

import os
import platform
from shutil import copyfile
from subprocess import check_output, CalledProcessError


def is_tool(name):
    cmd = "where" if platform.system() == "Windows" else "which"
    try:
        check_output([cmd, name])
        return True
    except CalledProcessError:
        return False


def build_web():
    if not is_tool("npm"):
        print("WARNING: npm is not avaiable. web interface will not be build.")
        return
    os.chdir("web")
    print("Attempting to build webpage...")
    print(check_output(["npm", "install"]))
    print(check_output(["node_modules/.bin/gulp"]))
    copyfile("build/index.html.gz.h", "../dist/index.html.gz.h")
    os.chdir("..")


build_web()

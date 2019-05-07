#!/usr/bin/env python

"""MQTT433gateway web interface builder

Project home: https://github.com/puuu/MQTT433gateway/
"""

import platform
from subprocess import check_output, check_call, CalledProcessError
from xxd_i import dump


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
    print("Attempting to build webpage...")
    check_call(["npm", "install", "--prefix", "web"])
    check_call(["npm", "run", "--prefix", "web", "build"])
    dump("web/dist/index.html.gz", "dist/index.html.gz.h", "index_html_gz")


build_web()

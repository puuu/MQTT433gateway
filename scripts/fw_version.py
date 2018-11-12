"""MQTT433gateway git firmware version extractor

Project home: https://github.com/puuu/MQTT433gateway/
"""
import subprocess
import json
import platformio
from platformio.managers.lib import LibraryManager
from platformio.managers.platform import PlatformFactory
Import("env")

CHARS_TO_ESCAPE = ("'", "{", "}")

def get_fw_version():
    return subprocess.check_output(['git', 'describe', '--abbrev=8', '--dirty',
                                    '--always', '--tags']).strip()

def get_dependencies():
    lm = LibraryManager(platformio.util.get_projectlibdeps_dir())
    dependencies = {library['name']: library['version']
                    for library in lm.get_installed()}
    platform = env['PIOPLATFORM']
    dependencies[platform] = PlatformFactory.newPlatform(platform).version
    dependencies['PlatformIO'] = platformio.__version__
    return dependencies

def get_dependencies_json():
    return json.dumps(get_dependencies(), separators=(',', ':'), sort_keys=True)

def escape_string(text, escape_chars=CHARS_TO_ESCAPE):
    for char in escape_chars:
        text = text.replace(char, '\\'+char)
    return text

def gen_build_flag_macro(macro, value):
    return "-D{name}='{definition}'".format(name=macro,
                                            definition=escape_string(value))

env.Append(
    BUILD_FLAGS=[
        gen_build_flag_macro('FIRMWARE_VERSION', get_fw_version()),
        gen_build_flag_macro('FW_BUILD_WITH', get_dependencies_json())
    ]
)

"""MQTT433gateway git firmware version extractor

Project home: https://github.com/puuu/MQTT433gateway/
"""
import subprocess
Import("env")

def get_fw_version():
    return subprocess.check_output(['git', 'describe', '--abbrev=8', '--dirty',
                                    '--always', '--tags']).strip()

def gen_build_flag_macro(macro, value):
    return "-D{name}='{definition}'".format(name=macro, definition=value)

env.Append(
    BUILD_FLAGS=[
        gen_build_flag_macro('FIRMWARE_VERSION', get_fw_version())
    ]
)

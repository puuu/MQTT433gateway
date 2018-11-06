"""MQTT433gateway git firmware extractor

Project home: https://github.com/puuu/MQTT433gateway/
"""
import subprocess
Import("env")

framework_version = '-DFIRMWARE_VERSION="' + subprocess.check_output(['git', 'describe', '--abbrev=8', '--dirty', '--always', '--tags']).strip() + '"'

env.Append(
    BUILD_FLAGS=[
        framework_version
    ]
)

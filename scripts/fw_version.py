"""MQTT433gateway git firmware extractor

Project home: https://github.com/puuu/MQTT433gateway/
"""
import subprocess
Import("env")

framework_version = str("-DFRAMEWORK_VERSION=\\\"") + subprocess.check_output(['git', 'describe', '--abbrev=8', '--dirty', '--always', '--tags']).strip() + str("\\\"")

env.Append(
  BUILD_FLAGS=[
      framework_version
  ]
)

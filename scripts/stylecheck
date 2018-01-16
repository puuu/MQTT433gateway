#!/bin/sh
# Project home: https://github.com/puuu/MQTT433gateway/
# Script to check styling of C/C++ files
# Copyright (c) 2018 Puuu
# Permission to copy and modify is granted under the MIT license

DIFF_PRAMETER=""
UPDATE_FILES=false
QUIET=false
while [ "$1" != "" ]; do
  case $1 in
    -c|--color)
      DIFF_PRAMETER="$DIFF_PRAMETER --color"
      ;;
    -u|--update)
      UPDATE_FILES=true
      ;;
    -q|--quiet)
      QUIET=true
      ;;
    -h|--help|*)
      echo "Usage: $0 [OPTIONS]"
      echo "Checks style of all source files and show necessary changes. Returns exit code"
      echo "of 1 if styling is incorrect."
      echo ""
      echo "Options:"
      echo "-c, --color     colorize diff output"
      echo "-q, --quiet     suppress diff output"
      echo "-u, --update    update files in place"
      exit
      ;;
  esac
  shift
done

RESULT=0
for file in $(find lib src \( -name "*.cpp" -o -name ".c" -o -name "*.h" \))
do
  [ $QUIET != true ] && clang-format -style=google "$file" | \
    diff $DIFF_PRAMETER -u "$file" - || RESULT=$?
  [ $UPDATE_FILES = true ] && clang-format -style=google "$file" -i
done
exit $RESULT

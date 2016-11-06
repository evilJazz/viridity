#!/bin/bash
SCRIPT_FILENAME="`cd \`dirname \"$0\"\`; pwd`/`basename \"$0\"`"
SCRIPT_ROOT=$(dirname "$SCRIPT_FILENAME")
cd "$SCRIPT_ROOT"

set -e

if [ ! -f 3rdparty/kcl/kcl.pri ]; then
   git clone https://git.katastrophos.net/kcl.git 3rdparty/kcl
else
   cd 3rdparty/kcl
   git pull
fi

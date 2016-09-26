#!/bin/bash
SCRIPT_FILENAME="`cd \`dirname \"$0\"\`; pwd`/`basename \"$0\"`"
SCRIPT_ROOT=$(dirname "$SCRIPT_FILENAME")
cd "$SCRIPT_ROOT"

if [ ! -d 3rdparty/kcl ]; then
   git clone https://git.katastrophos.net/kcl.git 3rdparty/kcl
else
   cd 3rdparty/kcl
   git pull
fi


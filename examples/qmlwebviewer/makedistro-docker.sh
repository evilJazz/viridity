#!/bin/bash
SCRIPT_FILENAME="`cd \`dirname \"$0\"\`; pwd`/`basename \"$0\"`"
DISTRO_ROOT=$(dirname "$SCRIPT_FILENAME")

function usage()
{
    echo $0 [--bundle-libs] [TARGETROOT] [TARGETNAME] {filename of docker image}
    echo
    echo "  --bundle-libs    Bundle Qt libraries with tarball"
    echo "  TARGETROOT = the install path defined in the .pro/Makefile, e.g. /opt/qmlwebviewer"
    echo "  TARGETNAME = the name of the binary, e.g. qmlwebviewer"
    echo
    echo "NOTE: Required the QTDIR environment variable to be defined."
    echo
}

BUNDLE_LIBS_PARAM=
if [ "$1" == "--bundle-libs" ]; then
    BUNDLE_LIBS_PARAM=--bundle-libs
    shift 1
fi

TARGETROOT="$1"
TARGETNAME="$2"
OUTPUTNAME="$3"

[ -z $TARGETROOT ] && usage && exit 1
[ -z $TARGETNAME ] && usage && exit 1

set -e

cd "$DISTRO_ROOT"
rm -Rf docker

mkdir docker
cd docker

mkdir files

OUTPUTTARBALL="$DISTRO_ROOT/docker/files/output.tar.gz"

$DISTRO_ROOT/makedistro-tarball.sh $BUNDLE_LIBS_PARAM "$TARGETROOT" "$OUTPUTTARBALL"

echo "Assembling Docker image..."

cat > files/run.sh << RUNSH
#!/bin/sh
cd "$TARGETROOT"/bin

export LANG="de_DE.UTF-8"
export LANGUAGE="de_DE"

QT_PLUGIN_PATH="$TARGETROOT/plugins" xvfb-run -a -s "-screen 0 640x480x24" "./$TARGETNAME" \$@
RUNSH

chmod +x files/run.sh

cat > Dockerfile << DOCKER
FROM ubuntu:18.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get -y install \
    fontconfig libjpeg8 xvfb libxrender1 libxcomposite1 libegl1-mesa libxslt1.1 libxi6 libfontconfig1 libglib2.0-0 \
    libdbus-1-3 libicu60 libasound2 libpulse-mainloop-glib0 libpulse0 libssl1.0.0 libssl1.0-dev ca-certificates
DOCKER

if [ -z "$BUNDLE_LIBS_PARAM" ]; then
    cat >> Dockerfile << DOCKER
RUN apt-get -y install qt5-default qml libqt5qml5 libqt5quick5 libjpeg
DOCKER
fi

cat >> Dockerfile << DOCKER
RUN mkdir /data
RUN mkdir /config
RUN chown www-data.www-data config

VOLUME /data
VOLUME /config

EXPOSE 8080

ADD "files/output.tar.gz" /
ADD "files/run.sh" "$TARGETROOT"

USER www-data
ENTRYPOINT ["$TARGETROOT/run.sh"]
DOCKER

if [ ! -z "$OUTPUTNAME" ]; then
    echo "Building Docker image..."
    docker build -t "$TARGETNAME" .
    echo "Exporting Docker image..."
    docker save "$TARGETNAME" | gzip -9 > "$OUTPUTNAME"
fi

echo "Done."

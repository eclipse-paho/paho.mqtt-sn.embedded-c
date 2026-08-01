#!/bin/bash

WORK_DIR=$(realpath $(dirname "$0")/..)
BDIR=build.gateway
ODIR=bin/

build () {
    echo "Start building MQTT-SN Gateway  $1"
    
    pushd "$WORK_DIR"
    if [ ! -d ./$BDIR ]; then
        mkdir $BDIR
    fi
    cd $BDIR
    cmake .. -DSENSORNET=$1 -DDEFS="${2} ${3}"
    make MQTTSNPacket
    make MQTT-SNGateway
    make MQTT-SNLogmonitor
    popd
    cp *.conf ./$ODIR
}

if [ $1 == "udp" ] ; then
    build $1 $2 $3
elif [ $1 == "udp6" ] ; then 
    build $1 $2 $3
elif [ $1 == "xbee" ] ; then
    build $1 $2 $3
elif [ $1 == "loralink" ]; then
    build $1 $2 $3
elif [ $1 == "rfcomm" ] ; then 
    build $1 $2 $3
elif [ $1 == "dtls" ] ; then
    build $1 $2 $3
elif [ $1 == "dtls6" ] ; then
    build dtls "${2} ${3} -DDTLS6"
elif [ $1 == "clean" ] ; then
    pushd "$WORK_DIR"
    rm -rf ./$BDIR
    popd
    rm -rf ./$ODIR
else
    echo "Usage: build.sh  [ udp | udp6 | xbee | loralink | rfcomm | dtls | dtls6 | clean]"
fi



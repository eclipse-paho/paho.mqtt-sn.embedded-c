#!/bin/bash

DEF1="DEF1=${2}"
DEF2="DEF2=${3}"

if [ $1 == "udp" ] ; then
    make SN=UDP $DEF1 $DEF2
elif [ $1 == "udp6" ] ; then 
    make SN=UDP6 $DEF1 $DEF2
elif [ $1 == "rfcomm" ] ; then 
	export LDADDBLT=-lbluetooth
    make SN=RFCOMM $DEF1 $DEF2
elif [ $1 == "dtls" ] ; then
    make SN=DTLS $DEF1 $DEF2
elif [ $1 == "dtls6" ] ; then
    make SN=DTLS6 $DEF1 $DEF2
elif [ $1 == "clean" ] ; then
    make clean
else
    echo "Usage: build.sh  [ udp | udp6 | rfcomm | dtls | dtls6 | clean]"
fi



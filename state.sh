#!/bin/bash
case $1 in
0)
    STATE="UNKNOWN"
    ;;
1)
    #ready
    STATE="RE"
    ;;
2)
    #armed stay
    STATE="AR_STAY"
    ;;
3)
    #armed away
    STATE="AR_AWAY"
    ;;
4)
    #armed instant
    STATE="AR_INSTANT"
    ;;
5)
    #armed max
    STATE="AR_MAX"
    ;;
6)
    #alarm
    STATE="AL"
    ;;
7)
    #not ready
    STATE="NR"
    ;;
*)
    STATE="Unknown"
    exit 15
    ;;
esac

exit 0

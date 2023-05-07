#!/bin/bash
case $1 in
0)
    STATE="Battery power"
    echo "Alarm on battery power"
    ;;
1)
    STATE="AC power"
    ;;
*)
    STATE="Unknown"
    exit 15
    ;;
esac

exit 0

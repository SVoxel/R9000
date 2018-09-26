#!/bin/sh

nice=`config get plex_nice`
/bin/nice -n $nice ./plexmediaserver.sh start

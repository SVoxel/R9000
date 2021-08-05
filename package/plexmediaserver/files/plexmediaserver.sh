#!/bin/sh

SCRIPT=$(readlink -f $0)
SCRIPTPATH=$(dirname ${SCRIPT})
cd $SCRIPTPATH

if [ "$(pwd)" != "$SCRIPTPATH" ]; then
  echo "Something went wrong, not able to switch to $SCRIPTPATH"
  exit 1
fi

# Set identification variables
export PLEX_MEDIA_SERVER_INFO_VENDOR=Netgear
export PLEX_MEDIA_SERVER_INFO_DEVICE="$(artmtd -r board_model_id|cut -d':' -f2)"
export PLEX_MEDIA_SERVER_INFO_MODEL="$(uname -m)"
export PLEX_MEDIA_SERVER_INFO_PLATFORM_VERSION="$(cat /firmware_version)"

export GCONV_PATH="/tmp/plexmediaserver/extra_libs/gconv"
export PLEX_MEDIA_SERVER_HOME="${SCRIPTPATH}"
export PLEX_MEDIA_SERVER_MAX_PLUGIN_PROCS=6
export PLEX_BROWSER_ROOT="/tmp/mnt"
export PLEX_MEDIA_SERVER_APPLICATION_SUPPORT_DIR="$(config get plex_file_path)/Library/Application Support"
export PLEX_MEDIA_SERVER_DISABLE_AUTOUPDATES=1
export PLEX_MEDIA_SERVER_DEFAULT_PREFERENCES="ScannerLowPriority=true&DlnaEnabled=false&TranscoderVideoResolutionLimit=704x480&TranscoderH264Preset=ultrafast"
export LC_ALL="C"
export LANG="C"
export TMPDIR="$(config get plex_file_path)"
export LD_LIBRARY_PATH="${SCRIPTPATH}/lib:/tmp/plexmediaserver/extra_libs"

ulimit -s 3000


startme() {
  echo "Starting Plex Media Server"
  if [ -d "${SCRIPTPATH}/extra_libs"  ]; then
    echo "Installing required libs into /tmp/plexmediaserver/extra_libs"
    rm -rf "/tmp/plexmediaserver/extra_libs"
    mv "${SCRIPTPATH}/extra_libs" "/tmp/plexmediaserver/extra_libs"
  fi
  ./Plex\ Media\ Server &
}

stopme() {
  echo "Stopping Plex Media Server"
  if [[ -f "${PLEX_MEDIA_SERVER_APPLICATION_SUPPORT_DIR}/Plex Media Server/plexmediaserver.pid" ]]; then
    kill -3 $(cat "${PLEX_MEDIA_SERVER_APPLICATION_SUPPORT_DIR}/Plex Media Server/plexmediaserver.pid")
    echo "Quit sent to server. Waiting 3 seconds and force killing if not dead"
    sleep 3
  fi
  if [[ "$(ps|egrep -e 'Plex Media Server|Plex DLNA Server'|grep -v grep|wc -l)" != "0" ]]; then
    echo "Force killing leftover procs"
    ps|egrep -e "Plex Media Server|Plex DLNA Server"|awk '{print $2}'|xargs kill -9
  else
    echo "Plex Media Server shutdown cleanly"
  fi
}
case $1 in
  start)
    startme
  ;;

  stop)
    stopme
  ;;

  restart)
    stopme; startme
  ;;

  *)
    echo "plexmediaserver.sh needs one of the folling options (start|stop|restart)"
esac

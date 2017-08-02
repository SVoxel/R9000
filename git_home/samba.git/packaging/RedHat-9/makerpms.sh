#!/bin/sh
# Copyright (C) John H Terpstra 1998-2002
# Updated for RPM 3 by Jochen Wiedmann, joe@ispsoft.de
# Changed for a generic tar file rebuild by abartlet@pcug.org.au
# Changed by John H Terpstra to build on RH7.2 - should also work for earlier versions jht@samba.org

# The following allows environment variables to override the target directories
#   the alternative is to have a file in your home directory calles .rpmmacros
#   containing the following:
#   %_topdir  /home/mylogin/redhat
#
# Note: Under this directory rpm expects to find the same directories that are under the
#   /usr/src/redhat directory
#

SPECDIR=`rpm --eval %_specdir`
SRCDIR=`rpm --eval %_sourcedir`

# At this point the SPECDIR and SRCDIR vaiables must have a value!

USERID=`id -u`
GRPID=`id -g`
VERSION='3.0.24'
REVISION=''
SPECFILE="samba3.spec"
RPMVER=`rpm --version | awk '{print $3}'`
RPM="rpm"
echo The RPM Version on this machine is: $RPMVER

##
## fix the mandir macro
##
case $RPMVER in
    [23]*)
       sed -e "s/MANDIR_MACRO/\%\{prefix\}\/man/g" < samba.spec > $SPECFILE
       ;;
    4*)
       sed -e "s/MANDIR_MACRO/\%\{_mandir\}/g" < samba.spec > $SPECFILE
       ;;
    *)
       echo "Unknown RPM version: `rpm --version`"
       exit 1
       ;;
esac

##
## now catch the right command to build an RPM (defaults ro 'rpm'
##
case $RPMVER in
    4.[123]*)
       RPM="rpmbuild"
       ;;
esac

echo "RPM build command is \"$RPM\""

pushd .
cd ../../source
if [ -f Makefile ]; then
        make distclean
fi
popd

pushd .
cd ../../../
chown -R ${USERID}.${GRPID} samba-${VERSION}${REVISION}
if [ ! -d samba-${VERSION} ]; then
        ln -s samba-${VERSION}${REVISION} samba-${VERSION} || exit 1
fi
echo -n "Creating samba-${VERSION}.tar.bz2 ... "
tar --exclude=.svn -cf - samba-${VERSION}/. | bzip2 > ${SRCDIR}/samba-${VERSION}.tar.bz2
echo "Done."
if [ $? -ne 0 ]; then
        echo "Build failed!"
        exit 1
fi

popd


/bin/cp -p filter-requires-samba_rh8.sh ${SRCDIR}
/bin/cp -p filter-requires-samba_rh9.sh ${SRCDIR}
chmod 755 ${SRCDIR}/filter-requires-samba_rh?.sh
/bin/cp -av $SPECFILE ${SPECDIR}

echo Getting Ready to build release package
cd ${SPECDIR}
${RPM} -ba --clean --rmsource $SPECFILE

echo Done.


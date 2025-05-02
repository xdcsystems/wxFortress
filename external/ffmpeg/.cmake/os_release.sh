#!/bin/sh

os_release_text=`cat /etc/os-release 2>/dev/null`

os_name=`echo "${os_release_text}" | \
    grep -x -E 'ID=.+' | \
    sed s/ID=// | \
    sed -r s/\"\(.+\)\"/\1/`

os_version=`echo "${os_release_text}" | \
    grep -x -E VERSION_ID=.+ | \
    sed s/VERSION_ID=// | \
    sed -r "s/\"(.+)\"/\1/" | \
    sed -r "s/(.+\..+)\..+/\1/"`

echo ${os_name}-${os_version}


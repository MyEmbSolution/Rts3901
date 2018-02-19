#!/bin/sh

destdir=/media/sdcard
scheduledir=/media/sdcard/schedule
mddir=/media/sdcard/md

do_umount()
{
        if grep -qs "^/dev/$1 " /proc/mounts ; then
                umount "$destdir";
        fi

        [ -d "${destdir}" ] && rmdir "${destdir}"
}

do_mount()
{
        mkdir -p "${destdir}" || exit 1

        if ! mount -t auto "/dev/$1" "${destdir}"; then
                # failed to mount, clean up mountpoint
                rmdir "${destdir}"
                exit 1
        fi

        mkdir -p "${scheduledir}" || exit 1
        mkdir -p "${mddir}" || exit 1

}

case "${ACTION}" in
add|"")
        do_umount ${MDEV}
        do_mount ${MDEV}
        ;;
remove)
        do_umount ${MDEV}
        ;;
esac

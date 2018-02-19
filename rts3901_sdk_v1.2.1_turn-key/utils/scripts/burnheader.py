#!/usr/bin/env python
# -*- coding:utf-8 -*-

import sys
import os
import platform
import getpass
import datetime
import binascii
import struct
import merge


relpath_dict = {
    "merge_ini": "utils/scripts/merge.ini",
    "target": "target",
    "sdkver": "config/VERSION",
    "uboot_r": "target/uboot/rlxboard.h",
    "uboot": "bootloader/uboot/Makefile",
}


def pathj(dname, fname):
    return os.path.abspath(os.path.join(dname, fname))


class Header_info:
    def __init__(self, root):
        self.magic = "7265616c"
        self.root_dir = root
        merge_ini = pathj(self.root_dir, relpath_dict["merge_ini"])
        self.pairs = merge.file_pairs(self.root_dir, merge_ini)
        self.f_target = self.init_fname("target")
        self.f_sdkver = self.init_fname("sdkver")
        self.f_uboot = self.init_fname("uboot")
        self.f_uboot_r = self.init_fname("uboot_r")
        self.f_mcu_fw = self.find_mcu_fw()

        self.lines = []
        print "-------------------- header begin --------------------"
        self.add_line(self.magic, "magic")
        self.add_line(self.bitmap_ic(), "mapic")
        self.add_line(self.mcu_pid_vid(), "pidvid")
        self.add_line(self.kernel_version()+"00", "kernel")
        self.add_line(self.sdk_version(), "sdkver")
        self.add_line(self.mcu_trunk(), "trunk")
        self.add_line(self.mcu_customer(), "customer")
        self.add_line(self.uboot_version(), "uboot")
        self.add_line(self.uboot_revision()+"0000", "uboot_r")
        self.add_line(self.signature(), "signature", False)
        self.add_line(self.build_date(), "date")
        self.add_line(self.build_time()+"00", "time")
        self.add_line(self.align_zero_bytes(256))
        print "-------------------- header end   --------------------"

    def find_mcu_fw(self):
        mcu_fw = [f for key, f in self.pairs if key == "mcu_fw"]
        if len(mcu_fw) != 1:
            return
        return mcu_fw[0]

    def add_line(self, line, info="", tobin=True):
        if info:
            print "{:10}{}".format(info, line)
        if tobin:
            self.lines.append(binascii.a2b_hex(line))
        else:
            self.lines.append(line)

    def bitmap_ic(self):
        bitmap = 0
        for key, f in self.pairs:
            if key == "u-boot":
                bitmap += 1
            if key == "mcu_fw":
                bitmap += 2
            if key == "hconf":
                bitmap += 4
            if key == "jffs2":
                bitmap += 8
            if key == "kernel":
                bitmap += 16
            if key == "rootfs":
                bitmap += 32
            if key == "ldc":
                bitmap += 64
        ic = os.readlink(self.f_target)[-4:]
        hic, lic = int(ic[:2]), int(ic[2:])
        return "{:02x}{:02x}{:04x}".format(hic, lic, bitmap)

    def signature(self, size=32):
        user = getpass.getuser()
        host = platform.node()
        line = "{}@{}".format(user, host)
        line += size * "\0"
        return line[:size]

    def build_date(self):
        line = datetime.datetime.now().strftime("%Y-%m-%d")
        y, m, d = [int(x) for x in line.split("-")]
        return "{:04x}{:02x}{:02x}".format(y, m, d)

    def build_time(self):
        line = datetime.datetime.now().strftime("%H-%M-%S")
        h, m, s = [int(x) for x in line.split("-")]
        return "{:02x}{:02x}{:02x}".format(h, m, s)

    def init_fname(self, key):
        return pathj(self.root_dir, relpath_dict[key])

    def kernel_version(self):
        return "{:02x}{:02x}{:02x}".format(3, 10, 27)

    def sdk_version(self):
        with open(self.f_sdkver) as f:
            linum = 0
            for line in f:
                linum += 1
                old_line = line
                line = line.strip()
                if not line:
                    continue
                rc = 0
                if "rc" in line:
                    line, rc = line.split("-rc")
                V = []
                for x in line[1:].split("."):
                    if x.isdigit():
                        V.append(int(x))
                    else:
                        V.append(0)
                while len(V) < 3:
                    V.append(0)
                v1, v2, v3= V[:3]
                rc = int(rc)
                return "{:02}{:02}{:02}{:02}".format(v1, v2, v3, rc)

    def uboot_revision(self):
        with open(self.f_uboot_r) as f:
            for line in f:
                line = line.strip()
                if "TAG_STRING" in line:
                    row = line.split('"')[1][1:].split(".")
                    row = "{:02x}{:02x}".format(int(row[0]), int(row[1]))
                    return row

    def uboot_version(self):
        ver, plevel, extra = 2014, 1, 2
        if not os.path.exists(self.f_uboot):
            print "WARNING: uboot makefile not exist"
            return "{:04x}{:02x}{:02x}".format(ver, plevel, extra)

        with open(self.f_uboot) as f:
            for line in f:
                if "VERSION" in line[:len("VERSION")]:
                    ver = int(line.split("=")[-1].strip())
                if "PATCHLEVEL" in line:
                    plevel = int(line.split("=")[-1].strip())
                if "EXTRAVERSION" in line:
                    extra = int(line.split("-rc")[-1])
                    return "{:04x}{:02x}{:02x}".format(ver, plevel, extra)
        print "ERROR: uboot makefile parse failed"
        return "{:04x}{:02x}{:02x}".format(0, 0, 0)

    def mcu_pid_vid(self):
        offset = 0x1008
        if not self.f_mcu_fw:
            return "00" * 4
        with open(self.f_mcu_fw, "rb") as f:
            f.seek(offset)
            vid, pid = struct.unpack(">HH", f.read(4))
            row = "{:04x}{:04x}".format(vid, pid)
            return row

    def mcu_trunk(self):
        offset = 0x100c
        if not self.f_mcu_fw:
            return "00" * 4
        with open(self.f_mcu_fw, "rb") as f:
            f.seek(offset)
            trunk = struct.unpack(">I", f.read(4))[0]
            return "{:08x}".format(trunk)

    def mcu_customer(self):
        offset = 0x1010
        if not self.f_mcu_fw:
            return "00" * 4
        with open(self.f_mcu_fw, "rb") as f:
            f.seek(offset)
            customer = struct.unpack(">I", f.read(4))[0]
            return "{:08x}".format(customer)

    def align_zero_bytes(self, size):
        for line in self.lines:
            size -= len(line)
        return "00" * size

    def write_header(self, fd):
        for line in self.lines:
            fd.write(line)

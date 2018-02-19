#!/usr/bin/env python
# -*- coding:utf-8 -*-

import os
import struct
import binascii
import burnheader
import mtdpart as mp

def file_size(fname):
    return os.stat(fname).st_size


def file_checksum(fname):
    align = 0x100000000
    fd = open(fname, "rb")
    checksum = 0
    while True:
        c = fd.read(1)
        if not c:
            break
        checksum += struct.unpack("B", c)[0]
    return (align - checksum % align) & 0xFFFFFFFF


def config_val(line, base=10):
    return int(line.split("=")[1], base)


class configure:
    def init_header(self, fname):
        self.ps = mp.Parts(fname)
        self.flash_size = self.ps.flash_size
        self.eb_size = self.ps.sector_size

    def init_map(self):
        self.config_map = {}
        for item in self.ps.parts:
            burnaddr1 = item.name + "_addr"
            maxcnt1 = item.name + "_cnt"
            magic1 = "ffffffff"
            self.burnaddr1, self.maxcnt1 = self.ps.get_burn_header(item.name)
            self.config_map[item.name] = [magic1, self.burnaddr1, self.maxcnt1]
        self.config_map["---FEOF---"] = ["46454f46", -1, 0]

    def __init__(self, config):
        self.kernel_cnt = 0
        self.init_header(config)
        self.init_map()

    def magic(self, key):
        return self.config_map[key][0]

    def burnaddr(self, key):
        return self.config_map[key][1]

    def max_cnt(self, key):
        return self.config_map[key][2]

    def max_size(self, key):
        return self.config_map[key][2] * self.eb_size

    def dump(self):
        print "eraseblock size: 0x%x" % (self.eb_size)
        print "filename\t magic\t\t burnaddr(hex)\t maxcnt(dec)"
        for key in self.config_map:
            align = "\t" if len(key) < 8 else ""
            item = self.config_map[key]
            print "%s\t %s\t %8x\t %4d" % (key + align,
                                           item[0], item[1], item[2])

    def write_feof(self, fd):
        fd.write(binascii.a2b_hex(self.config_map["---FEOF---"][0]))

class head:
    def __init__(self, config, key, fname):
        fsize = file_size(fname)
        self.magic = config.magic(key)
        self.reserved = (8 + 16) * "0"
        self.addr = "%016x" % config.burnaddr(key)
        self.size = "%016x" % (fsize)
        self.content = self.magic + self.reserved + self.addr + self.size

        max_size = config.max_size(key)
        if max_size < fsize:
            print "ERROR: %s size 0x%x > 0x%x" % (fname, fsize, max_size)
            exit(1)

    def dump(self):
        print self.magic, self.reserved
        print self.addr, self.size

    def write(self, fd):
        fd.write(binascii.a2b_hex(self.content))


class tail:
    def __init__(self, fname):
        self.checksum = "%08x" % (file_checksum(fname))
        self.content = self.checksum

    def dump(self):
        print self.checksum

    def write(self, fd):
        fd.write(binascii.a2b_hex(self.content))


def file_align(fname, bytes=4):
    size = os.stat(fname).st_size
    print fname, size
    if size % bytes:
        falign = fname + ".align"
        with open(fname, "r") as src:
            with open(falign, "w") as dst:
                dst.write(src.read())
                dst.write(binascii.a2b_hex('ff' * (bytes - size % bytes)))
        return falign
    return fname


def file_append(fd, config, key, fname):
    fname = file_align(fname)
    print "append:", os.path.realpath(fname)
    h = head(config, key, fname)
    t = tail(fname)
    h.dump()
    print "[%s]" % (os.path.basename(fname))
    t.dump()
    h.write(fd)
    fd.write(open(fname).read())
    t.write(fd)

def file_raw_fill(fd, config, key, fname):
    print "raw append:", os.path.realpath(fname)
    addr = config.burnaddr(key)

    fd.seek(addr, 0)
    fd.write(open(fname).read())

def file_raw_fullfill(fd, config):
    pad_len = config.flash_size
    fd.write(binascii.a2b_hex('ff' * pad_len))

def file_pairs(root, fini):
    pairs = []
    with open(fini) as fd:
        for line in fd:
            line = line.strip()
            if not line or line[:1] == "#":
                continue
            key, path = line.split("=")
            key = key.strip()
            path = path.strip()
            if key == "root":
                root = path
                continue
            if path[0] != "/":
                path = os.path.join(root, path)
            if key == "jffs2":
                os.system("./mkjffs2.py")
            if not os.path.exists(path):
                if fini == "raw.ini":
                    return []
                print "WARNING: %s not exist, so skip it." % (os.path.abspath(path))
                continue
            pairs.append((key, path))

    return pairs

if __name__ == "__main__":
    root_directory = "../../"
    bsp_directory = root_directory + "target/bsp/"
    default_config = bsp_directory + "partition.ini"
    target_bin = root_directory + "image/linux.bin"
    header = root_directory + "image/header"
    raw_bin = root_directory + "image/raw.bin"

    images_ini = file_pairs(root_directory, "merge.ini")
    config = configure(default_config)
    config.dump()

    fd = open(target_bin, "wb")
    burnheader.Header_info(root_directory).write_header(fd)
    for key, fname in images_ini:
        file_append(fd, config, key, fname)
    config.write_feof(fd)

    raw_ini = file_pairs(root_directory, "raw.ini")
    if raw_ini != []:
        find_uboot = False
        for key, fname in raw_ini:
            if key == "boot":
                find_uboot = True
        if find_uboot == True:
            rawfd = open(raw_bin, "wb")
            file_raw_fullfill(rawfd, config)
            for key, fname in raw_ini:
                file_raw_fill(rawfd, config, key, fname)

#!/usr/bin/env python
# -*- coding:utf-8 -*-

import os
import sys

MTDPART_OFS_NXTBLK = -2

def line_reformat(line):
    line = line.strip()
    if line[:1] == "#" or not line:
        return None
    return line

def line_head(line):
    return line[:1] == "[" and line[-1:] == "]"

def line_global(line):
    return line == "[global]"

def line_int_value(line, base=10):
    return int(line.split("=")[1].strip(), base)

def line_str_value(line):
    return line.split("=")[1].strip()

class Part():
    def __init__(self, sec_size, config):
        self.macro_dict = {
            "boot": ["RTS_MTD_BOOT_OFFSET", "RTS_MTD_BOOT_SIZE"],
            "mcu": ["RTS_MTD_MCU_FW_OFFSET", "RTS_MTD_MCU_FW_SIZE"],
            "hconf": ["RTS_MTD_HCONF_OFFSET", "RTS_MTD_HCONF_SIZE"],
            "userdata": ["RTS_MTD_USERDATA_OFFSET", "RTS_MTD_USERDATA_SIZE"],
            "kernel": ["RTS_MTD_KERNEL_OFFSET", "RTS_MTD_KERNEL_SIZE"],
            "rootfs": ["RTS_MTD_ROOTFS_OFFSET", "RTS_MTD_ROOTFS_SIZE"],
            "ldc": ["RTS_MTD_LDC_OFFSET", "RTS_MTD_LDC_SIZE"],
        }
        self.sector_size = sec_size
        self.name = config[0][1:-1]
        self.offset = MTDPART_OFS_NXTBLK
        self.sectors = 0
        self.readonly = 0
        self.flags = ""
        for line in config:
            if "offset" in line:
                self.offset = line_int_value(line)
            if "sectors" in line:
                self.sectors = line_int_value(line)
            if "readonly" in line:
                self.readonly = line_int_value(line)
            if "flags" in line:
                self.flags = line.split("=")[1].strip()

    def dump(self):
        print self.name, self.offset, self.sectors, self.readonly

    def offset_bytes(self):
        return self.offset * self.sector_size
    def size_count(self):
        return self.sectors

    def macro_def(self):
        if self.name in self.macro_dict:
            line = "#define %s %s\n" % (self.macro_dict[self.name][0],
                                        self.offset_str(1))
            line += "#define %s %s\n" % (self.macro_dict[self.name][1],
                                         self.size_str(1))
            return line
        return ""

    def offset_str(self, hex_only=False):
        if not hex_only and self.name in self.macro_dict:
            return self.macro_dict[self.name][0]
        return "0x%x" % (self.offset * self.sector_size)

    def size_str(self, hex_only=False):
        if not hex_only and self.name in self.macro_dict:
            return self.macro_dict[self.name][1]
        return "0x%x" % (self.sectors * self.sector_size)

    def write_define(self, fd):
        fd.write(self.macro_def())

    def write_code(self, fd):
        code = """\t{
\t\t.name = "%s",
\t\t.offset = %s,
\t\t.size = %s,
"""
        fd.write(code % (self.name, self.offset_str(), self.size_str()))
        flags = "MTD_CAP_ROM" if self.readonly else ""
        if self.flags:
            flags = flags + " | " + self.flags if flags else self.flags
        if flags:
            fd.write("\t\t.mask_flags = {0},\n".format(flags))
        fd.write("\t},\n")

    def write_mapping_config(self, fd):
        if self.name == "boot":
            fd.write("#define CONFIG_SOC_BOOT_BLK_CNT %d\n" % self.sectors)
        elif self.name == "mcu":
            fd.write("#define CONFIG_SOC_MCU_BLK_CNT %d\n" % self.sectors)
        elif self.name == "hconf":
            fd.write("#define CONFIG_SOC_HCONF_BLK_CNT %d\n" % self.sectors)
        elif self.name == "userdata":
            fd.write("#define CONFIG_SOC_USERDATA_BLK_CNT %d\n" % self.sectors)
        elif self.name == "kernel":
            fd.write("#define CONFIG_SOC_KERNEL_BLK_CNT %d\n" % self.sectors)
        elif self.name == "rootfs":
            fd.write("#define CONFIG_SOC_ROOTFS_BLK_CNT %d\n" % self.sectors)
        elif self.name == "ldc":
            fd.write("#define CONFIG_SOC_LDC_BLK_CNT %d\n" % self.sectors)

    def injection_error(self):
        self.sectors = -1

class Parts():
    def __init__(self, fname):
        self.flash_size = 0x1000000
        self.sector_size = 0x10000
        self.mtd_id = "(ERROR! mtd_id NOT SET!)"
        self.flash_sectors = self.flash_size / self.sector_size
        self.rootfs_idx = 0
        self.hconf_idx = 0
        self.userdata_idx = 0
        self.parts = []
        self.parse_parts(fname)
        if self.update() < 0:
            self.injection_error()
        self.dump()

    def injection_error(self):
        for p in self.parts:
            p.injection_error()

    def parse_parts(self, fname):
        with open(fname) as f:
            config = []
            i = 0
            for line in f:
                line = line_reformat(line)
                if not line:
                    continue
                if line_head(line) and config:
                    if line == "[rootfs]":
                        self.rootfs_idx = i
                    if line == "[hconf]":
                        self.hconf_idx = i
                    if line == "[userdata]":
                        self.userdata_idx = i
                    i = i + 1
                    self.get_config(config)
                    config = [line]
                else:
                    config.append(line)

            self.get_config(config)
            print "rootfs_idx: ", self.rootfs_idx
            print "hconf_idx: ", self.hconf_idx
            print "userdata_idx: ", self.userdata_idx

    def get_config(self, config):
        if len(config) < 1:
            print "ERROR: config is empty"
        head = config[0]
        if line_global(head):
            self.init_global(config)
        else:
            self.init_part(config)

    def init_global(self, config):
        for line in config:
            if "flash_size" in line:
                self.flash_size = line_int_value(line, 16)
            if "sector_size" in line:
                self.sector_size = line_int_value(line, 16)
            if "mtd_id" in line:
                self.mtd_id = line_str_value(line)

        self.flash_sectors = self.flash_size / self.sector_size

    def init_part(self, config):
        self.parts.append(Part(self.sector_size, config))

    def dump(self):
        print "global", hex(self.flash_size), hex(self.sector_size)
        for p in self.parts:
            p.dump()

    def update(self):
        if self.flash_size % self.sector_size:
            print "ERROR: size not match", self.flash_size, self.sector_size
            return -1
        if self.flash_sectors != self.flash_size / self.sector_size:
            print "ERROR: caculation error"
            return -1

        offset = 0
        for p in self.parts:
            if p.offset == MTDPART_OFS_NXTBLK:
                p.offset = offset
            if p.offset < offset:
                print "ERROR: offset cross", p.dump()
                return -1
            if p.offset + p.sectors > self.flash_sectors:
                print "ERROR: size overflow", p.dump()
                return -1
            offset = p.offset + p.sectors
        return 0

    def write_partition_code(self, part_def, part_struct, rcs):
        with open(part_def, "w") as f:
            f.write("""/* generated by mtdpart.py */
#ifndef MTD_PARTITION_DEF_H
#define MTD_PARTITION_DEF_H
""")
            f.write("#define RTS_MTD_ROOTFS_IDX %d\n" % (self.rootfs_idx))
            f.write("#define RTS_MTD_HCONF_IDX %d\n" % (self.hconf_idx))
            for p in self.parts:
                p.write_define(f)
            f.write("""#endif /* MTD_PARTITION_DEF_H */
""")

        with open(part_struct, "w") as f:
            f.write("""/* generated by mtdpart.py */
#ifndef MTD_PARTITION_H
#define MTD_PARTITION_H
#include <linux/mtd/partitions.h>
#include "%s"

static struct mtd_partition rts_spiflash_part[] = {
""" % part_def.split("/")[-1])
            for p in self.parts:
                p.write_code(f)
            f.write("""};
#endif /* MTD_PARTITION_H */
""")

        with open(rcs, "w") as f:
            f.write("# generated by mtdpart.py\n\n")
            f.write("mount -t jffs2 /dev/mtdblock%d $1\n" % (self.userdata_idx))

    def write_mapping_config(self, fname):
        with open(fname, "w") as f:
            f.write("""/* generated by mtdpart.py */
#ifndef MTD_MAPPING_CONFIG
#define MTD_MAPPING_CONFIG

#define RTS_MTD_FLASH_SECTOR_SIZE 0x%x
""" % self.sector_size)
            for p in self.parts:
                p.write_define(f)

            f.write("\n")

            f.write("#define RTS_CMDLINE\t\t\"console=ttyS1,57600 \"\t\t\\\n")
            f.write("\t\t\t\t\"root=/dev/mtdblock%d \"\t\\\n" % (self.rootfs_idx))
            f.write("\t\t\t\t\"rts_hconf.hconf_mtd_idx=%d \"\t\\\n" % (self.hconf_idx))
            f.write("\t\t\t\t\"mtdparts=%s:" % (self.mtd_id))
            first_element = 1
            usedsize = 0
            freesize = 0
            for p in self.parts:
                if (first_element):
                    first_element = 0
                else:
                    f.write(",");
                f.write("%dk(%s)" % (p.sectors * self.sector_size / 1024, p.name))

                usedsize = usedsize + p.sectors * self.sector_size
            freesize = self.flash_size - usedsize
            if (freesize > 0):
                f.write(",")
                f.write("%dk(%s)" % (freesize / 1024, "freespace"))
            f.write("\"\n")

            f.write("#endif /* MTD_MAPPING_CONFIG */\n")

    def get_burn_header(self, name):
        for p in self.parts:
            if p.name == name:
                return (p.offset_bytes(), p.size_count())
        return (0, 0)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "Usage mtdpart.py rootdir"
    root_directory = os.path.normpath(sys.argv[1])

    bsp_directory = root_directory + "/target/bsp/"
    partition_ini = bsp_directory + "/partition.ini"

    linux_dir = root_directory + "/linux-3.10"
    mtd_partition_def = linux_dir + "/include/generated/mtd_partition_def.h"
    mtd_partition = linux_dir + "/include/generated/mtd_partition.h"
    rcS = root_directory + "/target/rootfs/etc/init.d/mount_jffs2"

    ps = Parts(partition_ini)
    ps.write_partition_code(mtd_partition_def, mtd_partition, rcS)
    os.chmod(rcS, 0755)
    print "Generated:", os.path.realpath(mtd_partition_def)
    print "Generated:", os.path.realpath(mtd_partition)
    print "Generated:", os.path.realpath(rcS)

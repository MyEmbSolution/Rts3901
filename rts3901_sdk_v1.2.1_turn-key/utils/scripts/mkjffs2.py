#!/usr/bin/env python
# -*- coding:utf-8 -*-

import os
import mtdpart as mp

def send_cmd(cmd):
    print cmd
    os.system(cmd)

def pathj(dname, bname):
    return os.path.join(dname, bname)

def create_jffs2(dname, size):
    cmd = "mkfs.jffs2 -r %s -e 0x10000 --pad=0x%x -l -o jffs2.img" % (dname, size)
    send_cmd(cmd)

def move_jffs2(dname):
    send_cmd("cp jffs2.img %s" % (dname))


if __name__ == "__main__":
    root = "../../"
    default_config = root + "target/bsp/partition.ini"

    ps = mp.Parts(default_config)
    eb_size = ps.sector_size
    data_addr, data_cnt = ps.get_burn_header("userdata")

    dname = pathj(root, "jffs2")
    size = data_cnt * eb_size
    if not os.path.exists(dname):
        print "JFFS directory not exist"
        exit(0)

    create_jffs2(dname, size)
    image = pathj(root, "image/jffs2.bin")
    move_jffs2(image)

#!/usr/bin/env python
# -*- coding:utf-8 -*-

import sys
import os

import mtdpart as mp

#top_config_file = root_dir + "/.config"
top_config_file =  "./.config"
top_config = open(top_config_file)

for line in top_config:
    if "CONFIG_PARTITION_IN_BSP" in line:
        if line[0] != "#":
            if len(sys.argv) != 2:
                print "Usage genconf.py rootdir"
                sys.exit(-1)
            root_dir = os.path.normpath(sys.argv[1])
            sys.path.append(root_dir + "/utils/scripts")
            partition = root_dir + "/target/bsp/partition.ini"
    elif "CONFIG_PARTITION_IN_UBOOT" in line:
        if line[0] != "#":
            partition = "./partition.ini"
top_config.close()

if __name__ == "__main__":
#    partition = root_dir + "/target/bsp/partition.ini"
    mapping_config = "include/generated/mtd_mapping_config.h"
    ps = mp.Parts(partition)
    ps.write_mapping_config(mapping_config)

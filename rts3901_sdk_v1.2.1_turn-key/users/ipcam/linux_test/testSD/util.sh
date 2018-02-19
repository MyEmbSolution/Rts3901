#!/bin/sh
#Code by shanjian_fei 2015.9.15

#check if the card has been inserted
check_card()
{
	if [ ! -e "$dev_node" ]; then
		echo "please insert card!"
		while true
		do
			if [ -e "$dev_node" ]; then
			break
			fi
		done
	fi
}

#format card mkfs.vfat
format_card()
{
	mkfs.vfat $dev_node
}

#erase the partiton table
erase_partition_table()
{
	dd if=/dev/zero of=$dev_node bs=1M count=2
}

#create partition
create_partition()
{
	(echo n; echo p; echo 1; echo ; echo; echo p; echo w) | fdisk $dev_node
}

wait_until_unplug()
{
	while true
	do
		if [ ! -e "$dev_node" ]; then
			break
		fi
	done
}

wait_until_plug()
{
	while true
	do
		if [ -e "$dev_node" ]; then
			break
		fi
	done
}






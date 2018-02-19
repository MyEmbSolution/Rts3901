#!/bin/sh
#Code by shanjian_fei 2015.8.6

sdcard_addr="/media/sdcard"
dev_node="/dev/mmcblk0"
partition_addr="/dev/mmcblk0p1"

source /bin/util.sh

print_help()
{
	echo ""
	echo ""
	echo "burntest.sh"
	echo "burntest.sh [n]  -----n is integer----- burntest.sh 2"
	echo "burntest.sh time [n]  -----n is integer----- burntest.sh time 1000"
	echo "burntest.sh size [n] filenumber [n]  -----n is integer----- burntest.sh size 1g filenumber 16"
}

test_exfat()
{
	while true
	do
		echo ""
		echo ""
		echo "Please remove card and format the card for exfat in windows,then insert it"
		wait_until_unplug
		wait_until_plug
		sleep 2
		df | grep mmcblk0p1
		if [ "$?" = "" ]; then
			if [ ! -e "$sdcard_addr" ]; then
				mkdir $sdcard_addr
				mount $partition_addr $sdcard_addr
			else
				mount $partition_addr $sdcard_addr
			fi
		else
			mount | grep exfat
			if [ "$?" = "" ]; then
				echo ""
				echo ""
				echo "The card's filesystem is not exfat"
			else
				break
			fi
		fi
	done
}

init()
{
	check_card

	sleep 2

	for directory in `ls /media`
	do
		umount "/media/"$directory
	done

	erase_partition_table
	create_partition
	#format and mount partition /dev/mmcblk0p1
	#sleep 1
	#mkfs.vfat $partition_addr

	echo ""
	echo ""
	echo "Please remove card!"

	while true
	do
		if [ ! -e "$dev_node" ]; then
			break
		fi
	done

	echo ""
	echo ""
	echo "Please insert card!"

	while true
	do
		if [ -e "$dev_node" ]; then
			break
		fi
	done

	sleep 1

	dir_num=0
	for directory in `ls /media`
	do
		dir_num=`expr $dir_num + 1`
	done

	#check, if there is no /media/sdcard,create it
	if [ $dir_num -eq 0 ]; then
		mkdir $sdcard_addr
		if [ "$1" = "ext2" ]; then
			echo ""
			echo ""
			echo "Now test ext2"
			mkfs.ext2 $partition_addr
			mount $partition_addr $sdcard_addr
		else
			echo ""
			echo ""
			echo "Now test fat"
			mkfs.vfat $partition_addr
			mount $partition_addr $sdcard_addr
		fi
	elif [ $dir_num -eq 1 ]; then
		sdcard_addr="/media/"`ls /media`
		umount $sdcard_addr
		if [ "$1" = "ext2" ]; then
			echo ""
			echo ""
			echo "Now test ext2"
			mkfs.ext2 $partition_addr
			mount $partition_addr $sdcard_addr
		else
			echo ""
			echo ""
			echo "Now test fat"
			mkfs.vfat $partition_addr
			mount $partition_addr $sdcard_addr
		fi
	else
		echo ""
		echo ""
		echo "The mount point is more than 1,please check it!"
		echo "Test failed!"
		exit
	fi
}

defaultnumber=1

if [ "$#" -eq 4 ]; then
	if [ "$1" = "size" ] && [ "$3" = "filenumber" ] && [ "$4" -ge 1 ]; then
		for i in `seq 2`
		do
			if [ $i = 1 ]; then
				init
				TESTSIZE=$2 FILENUMBER=$4 fio /etc/sdtest/write_size.job
			elif [ $i = 2 ]; then
				init ext2
				TESTSIZE=$2 FILENUMBER=$4 fio /etc/sdtest/write_size.job
			fi
		done
	else
		print_help
	fi
fi

if [ "$#" -eq 3 ] || [ "$#" -ge 5 ]; then
	print_help
fi

if [ "$#" -eq 2 ]; then
	if [ "$1" = "time" ] && [ "$2" -ge 1 ]; then
		for i in `seq 2`
		do
			if [ $i = 1 ]; then
				init
				TESTSIZE=$2 fio /etc/sdtest/write_runtime.job
			elif [ $i = 2 ]; then
				init ext2
				TESTSIZE=$2 fio /etc/sdtest/write_runtime.job
			fi
		done
	else
		print_help
	fi
fi

if [ "$#" -eq 1 ]; then
	if [ "$1" -ge 1 ]; then
		for i in `seq 2`
		do
			if [ $i = 1 ]; then
				init
				LOOPNUMBER=$1 fio /etc/sdtest/write_full_device.job
			elif [ $i = 2 ]; then
				init ext2
				LOOPNUMBER=$1 fio /etc/sdtest/write_full_device.job
			fi
		done
	else
		print_help
	fi
fi

if [ "$#" -eq 0 ]; then
	for i in `seq 2`
	do
		if [ $i = 1 ]; then
			init
			LOOPNUMBER=$defaultnumber fio /etc/sdtest/write_full_device.job
		elif [ $i = 2 ]; then
			init ext2
			LOOPNUMBER=$defaultnumber fio /etc/sdtest/write_full_device.job
		fi
	done
fi




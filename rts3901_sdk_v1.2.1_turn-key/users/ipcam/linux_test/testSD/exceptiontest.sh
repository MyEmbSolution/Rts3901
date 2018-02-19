#!/bin/sh
#Code by shanjian_fei 2015.8.6
dev_node="/dev/mmcblk0"
partition_addr="/dev/mmcblk0p1"
sdcard_addr="/media/sdcard"

source /bin/util.sh

#test help info
test_help_info()
{
	while true
	do
		number=$1
		if [ "$number" -ge 1 ]; then
			break
		else
			echo ""
			echo ""
			echo "please exec ----  execption.sh [n]  ---- to test"
			exit
		fi
	done
}

test_fat_and_ext2()
{
	check_card
	dir_num=0
	for directory in `ls /media`
	do
		dir_num=`expr $dir_num + 1`
	done
	if [ $dir_num -eq 0 ]; then
		mkdir $sdcard_addr
		if [ "$1" = "ext2" ]; then
			echo ""
			echo ""
			echo "-------------------------------------------------------"
			echo "Now test ext2"
			echo "-------------------------------------------------------"
			echo ""
			echo ""
			mkfs.ext2 $partition_addr
			mount $partition_addr $sdcard_addr
		else
			echo ""
			echo ""
			echo "-------------------------------------------------------"
			echo "Now test fat"
			echo "-------------------------------------------------------"
			echo ""
			echo ""
			mkfs.vfat $partition_addr
			mount $partition_addr $sdcard_addr
		fi
	elif [ $dir_num -eq 1 ]; then
		sdcard_addr="/media/"`ls /media`
		umount $sdcard_addr
		if [ "$1" = "ext2" ]; then
			echo ""
			echo ""
			echo "-------------------------------------------------------"
			echo "Now test ext2"
			echo "-------------------------------------------------------"
			echo ""
			echo ""
			mkfs.ext2 $partition_addr
			mount $partition_addr $sdcard_addr
		else
			echo ""
			echo ""
			echo "-------------------------------------------------------"
			echo "Now test fat"
			echo "-------------------------------------------------------"
			echo ""
			echo ""
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

test_exfat()
{
	while true
	do
		echo ""
		echo ""
		echo "--------------------------------------------------------------------------"
		echo "Please remove card and format the card for exfat in windows,then insert it"
		echo "--------------------------------------------------------------------------"
		echo ""
		echo ""
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

write_data()
{
	file_name="/test_file.dat"
	if [ -e "${sdcard_addr}${file_name}" ]; then
		rm -r ${sdcard_addr}${file_name}
	fi
	touch ${sdcard_addr}${file_name}
	echo ""
	echo ""
	echo "veridisk -d -w -f ${sdcard_addr}${file_name} -m 10000 -e 10000 -q"
	echo "Now, writing data to card,it need few minutes!Please wait!"
	veridisk -d -w -f ${sdcard_addr}${file_name} -m 2000 -e 1000 -q &
	echo ""
	echo "======================"
	echo "|Please remove card! |"
	echo "======================"
	sleep 1
	while true
	do
		if [ ! -e "$dev_node" ]; then
			if [ -e "$sdcard_addr" ]; then
				umount $sdcard_addr
				sleep 1
				if [ -e "$sdcard_addr" ]; then
					echo "umount $sdcard_addr failed"
					echo "test failed"
					exit
				fi
			fi
			break
		fi
	done
}

test_help_info $1

echo ""
echo ""
echo "Test $1 times"

for i in `seq $1`
do
	check_card

	sleep 2

	for directory in `ls /media`
	do
		umount "/media/"$directory
	done
	erase_partition_table
	create_partition
	echo "======================================================"
	echo "Start Exception Test! NO.$i"
	echo "======================================================"
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

	sleep 2
	test_fat_and_ext2 fat
	sleep 1
	write_data
	sleep 1
	test_fat_and_ext2 ext2
	sleep 1
	write_data
	#sleep 1
	#test_exfat
	#sleep 1
	#write_data
	sleep 1
	echo ""
	echo ""
	echo "Test NO.$i end!(Test $1 times)"

done

#!/bin/sh
#Code by shanjian_fei 20150728

dev_node="/dev/mmcblk0"
sdcard_addr="/media/sdcard"

source /bin/util.sh

check_card
sleep 2
if [ -e "$sdcard_addr" ]; then
	umount "$sdcard_addr"
fi
#format_card
erase_partition_table
create_partition


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


#check, if there is no /media/sdcard,create it
if [ ! -e "$sdcard_addr" ]; then
	mkdir $sdcard_addr
fi

#format and mount partition /dev/mmcblk0p1
partition_addr="/dev/mmcblk0p1"
mkfs.vfat $partition_addr
mount $partition $sdcard_addr

#check /dev/mmcblk0 removed ."1" means existence,"0" removed.
check_dev_node()
{
	if [ -e "$dev_node" ]; then
		return 1
	else
		return 0
	fi
}

#get card info
get_card_info()
{
	addr=/sys/bus/mmc/devices/
	card_info=`ls $addr`
	echo $card_info
}

#get manfid,cid,csd,date detail info
get_card_detail_info()
{
	addr=$1
	result=`cat $addr`
	echo $result
}

#check card information,judeg whether with a card
#if same return 1,different 0
compare_card_info_before_and_after()
{
	echo ""
	echo ""
	echo "The card info is: "
	echo $1
	echo $2
	if [ "$1" = "$2" ]; then
		return 1
	else
		return 0
	fi
}


echo "========================================================"
echo "Start Card Test!"
echo "========================================================"


#global variable
dev_info_dir="/sys/bus/mmc/devices/"

#delete random.dat and random.dat.md5
random_data="/media/sdcard/random.dat"
if [ -e "$random_data" ]; then
	rm -r $random_data
fi

random_data_md5_1="/usr/conf/random1.dat.md5"
random_data_md5_2="/usr/conf/random2.dat.md5"
if [ -e "$random_data_md5_1" ]; then
	rm -r $random_dat_md5_1
fi
if [ -e "$random_data_md5_2" ]; then
	rm -r $random_dat_md5_2
fi

echo ""
echo ""
echo "Create file named random.dat"


if [ -e "$random_data" ]; then
	rm -r $random_data
fi

#touch /media/sdcard/random.dat
touch "$random_data"
dd if=/dev/urandom of=/media/sdcard/random.dat bs=1024b count=1

echo ""
echo ""
echo "Create $random_data md5 file"
md5sum /media/sdcard/random.dat > /usr/conf/random1.dat.md5
sync
echo ""
echo ""
echo "get card info dir before removing"
echo ""
echo ""
card_info_before="$(get_card_info)"
echo "card_info_before: $card_info_before"

#get manfid info
manfid_addr=${dev_info_dir}${card_info_before}"/manfid"
manfid_info_before="$(get_card_detail_info $manfid_addr)"
echo "manfid_info_before: $manfid_info_before"

#get cid info
cid_addr=${dev_info_dir}${card_info_before}"/cid"
cid_info_before="$(get_card_detail_info $cid_addr)"
echo "cid_info_before: $cid_info_before"

#get csd info
csd_addr=${dev_info_dir}${card_info_before}"/csd"
csd_info_before="$(get_card_detail_info $csd_addr)"
echo "csd_info_before: $csd_info_before"

#get date info
date_addr=${dev_info_dir}${card_info_before}"/date"
date_info_before="$(get_card_detail_info $date_addr)"
echo "date_info_before: $date_info_before"

echo ""
echo ""
echo "Please remove the card"

while true
do
	if [ ! -e "$dev_node" ]; then
		break
	fi

done

sleep 2

#check /media/sdcard, if exists,remove
if [ -e "$sdcard_addr" ]; then
	umount "$sdcard_addr"
	if [ -e "$sdcard_addr" ]; then
		echo "$sdcard_addr exists and umount failed!"
		exit
	fi
fi

echo ""
echo ""
echo "========================================================"
echo "Please insert card!"
echo "========================================================"

while true
do
	if [ -e "$dev_node" ]; then
		break
	fi
done

sleep 2
echo ""
echo ""
echo "The card has been inserted!"

echo ""
echo ""
echo "========================================================"
echo "check card info"
echo "========================================================"
#get card info after inserting
card_info_after="$(get_card_info)"
#check card info ,equal or not
compare_card_info_before_and_after $card_info_before $card_info_after
compare_card_info_result=$?
if [ $compare_card_info_result = 1 ]; then
	echo ""
	echo "The card's info dir is the same after removing and inserting card"
elif [ $compare_card_info_result = 0 ]; then
	echo ""
	echo "The card's info dir was changed after removing and inserting card"
	echo "test failed"
	exit
fi

#compare manfid,cid,csd,date info
#info_str=("/manfid" "/cid" "/csd" "/date")
#manfid
manfid_str="/manfid"
manfid_addr=${dev_info_dir}${card_info_before}${manfid_str}
manfid_info_after="$(get_card_detail_info $manfid_addr)"
echo "manfid_info_after: $manfid_info_after"
compare_card_info_before_and_after $manfid_info_before $manfid_info_after
compare_manfid_result=$?
if [ $compare_manfid_result = 1 ]; then
	echo ""
	echo "The card's manfid is the same after removing and inserting card"
elif [ $compare_manfid_result = 0 ]; then
	echo ""
	echo "The card's manfid was changed after removing and inserting card"
	echo "test failed"
	exit
fi

#cid
cid_str="/cid"
cid_addr=${dev_info_dir}${card_info_before}${cid_str}
cid_info_after="$(get_card_detail_info $cid_addr)"
compare_card_info_before_and_after $cid_info_before $cid_info_after
compare_cid_result=$?
if [ $compare_cid_result = 1 ]; then
	echo ""
	echo "The card's cid is the same after removing and inserting card"
elif [ $compare_cid_result = 0 ]; then
	echo ""
	echo "The card's cid was changed after removing and inserting card"
	echo "test failed"
	exit
fi

#csd
csd_str="/csd"
csd_addr=${dev_info_dir}${card_info_before}${csd_str}
csd_info_after="$(get_card_detail_info $csd_addr)"
compare_card_info_before_and_after $csd_info_before $csd_info_after
compare_csd_result=$?
if [ $compare_csd_result = 1 ]; then
	echo ""
	echo "The card's csd is the same after removing and inserting card"
elif [ $compare_csd_result = 0 ]; then
	echo ""
	echo "The card's csd was changed after removing and inserting card"
	echo "test failed"
	exit
fi

#date
date_str="/date"
date_addr=${dev_info_dir}${card_info_before}${date_str}
date_info_after="$(get_card_detail_info $date_addr)"
compare_card_info_before_and_after $date_info_before $date_info_after
compare_date_result=$?
if [ $compare_date_result = 1 ]; then
	echo ""
	echo "The card's date is the same after removing and inserting card"
elif [ $compare_date_result = 0 ]; then
	echo ""
	echo "The card's date was changed after removing and inserting card"
	echo "test failed"
	exit
fi

#diff $file1 $file2
md5sum /media/sdcard/random.dat > /usr/conf/random2.dat.md5
#compare md5
md5_1="/usr/conf/random1.dat.md5"
md5_2="/usr/conf/random2.dat.md5"

echo ""
echo ""
echo "========================================================"
echo "Check if the file has been modified!"
echo "========================================================"

result=`diff $md5_1 $md5_2 > /dev/null`
if [ ! -n "$result" ]; then
	echo "The file is the same after removing and inserting card"
else
	echo "The file was changed after removing and inserting card "
	echo "test failed"
	exit
fi

echo ""
echo ""
echo "========================================================"
echo "Test result: PASS!"
echo "========================================================"

#!/bin/bash
if [ "$#" -ne 4 ];then
echo "Wrong Input"
fi
#take input arguments
dir_name=$1
echo $dir_name

num_of_files=$2
echo "Num of files=$num_of_files"
if [ "$num_of_files" -lt 0 ];then
echo "Invalid num of files"
fi

num_of_dirs=$3
echo "Num of dirs=$num_of_dirs"
if [ "$num_of_dirs" -lt 0 ];then
echo "Invalid num of dirs"
fi

levels=$4
echo "levels=$levels"
if [ "$levels" -lt 0 ];then
echo "Invalid num of levels"
fi

#check if directory exists
if [ ! -d "$dir_name" ];then
echo "Directory doesn't exist."
mkdir ./$dir_name
echo "Directory Created"
fi


#make random names for directories
array=("$num_of_dirs")

set="abcdefghijklmonpqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
counter=0
while [ "$counter" -lt "$num_of_dirs" ]; do
n=8
rand=""
for i in `seq 1 $n`; do
    char=${set:$RANDOM % ${#set}:1}
    rand+=$char
done
array[$counter]=$rand
let counter=counter+1
done
#echo "${array[@]}"

#create directory hierarchy
let "up=$levels - 1"
created=0
while [ "$created" -lt "$num_of_dirs" ]; do
nstr="$dir_name/${array[$created]}"
let created=created+1
mkdir $nstr
base=$nstr
for i in `seq 1 $up`; do
if [ "$created" -lt "$num_of_dirs" ];then
nstr="$base/${array[$created]}"
mkdir $nstr
let created=created+1
base=$nstr
fi
done
let counter=counter+1
done

#make random names for files
echo "-------------"
array2=("$num_of_files")
set="0123456789"
f="file"
counter=0
while [ "$counter" -lt "$num_of_files" ]; do
n=4
rand=""
for i in `seq 1 $n`; do
    num=${set:$RANDOM % ${#set}:1}
    rand+=$num
done
string="$f$rand"
array2[$counter]=$string
let counter=counter+1
done
#echo "${array2[@]}"

echo "-------------"

#split files in the directories - round robin
counter=0
while [ "$counter" -lt "$num_of_files" ]; do
for dir in $(find "$dir_name" -type d); do
if [ "$counter" -lt "$num_of_files" ];then
    echo > $dir/${array2[$counter]}
let counter=counter+1 
fi
done
done


#print output
for entry in $(find "$dir_name"); do
if [ -d "$entry" ];then
echo "$entry"
fi
if [ -f "$entry" ];then
echo "    $entry"
fi
done

#write in files
set="abcdefghijklmonpqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
counter=0
while [ "$counter" -lt "$num_of_files" ]; do
n=6
rand=""
for i in `seq 1 $n`; do
    char=${set:$RANDOM % ${#set}:1}
    rand+=$char
done
array2[$counter]=$rand
let counter=counter+1
done
#echo "${array2[@]}"

counter=0
for entry in $(find "$dir_name"); do
if [ -f "$entry" ];then
echo ${array2[counter]} >> $entry
let counter=counter+1 
fi
done


##############################################################
# File Name:  	  problem3-2-2.sh
# Version:	  V1.0
# Author:      	  Zhangmin
# Created Time :  2022-04-25 
# Description: 	  猜数字游戏
##############################################################

#! /bin/bash


:<<EOF
函数名：    rand
函数参数：  $1，随机数的下限
	    $2，随机数的上限
功能：	    产生一个指定范围内的随机数
*******************************************
思路：
    通过时间获得随机数：
    date +%s%N   #生成19位数字
    $num%$max+$min   #产生的随机数对max取余，得到随机数，
                     #再加上下限得到指定范围内的随机数
EOF
function rand(){  
    min=$1  
    max=$(($2-$min+1))  
    num=$(date +%s%N)  
    echo $(($num%$max+$min))  
}  


# 1.生成随机数
randNum=$(rand 1 60)
echo "$randNum"
# 将最小值与最大值存储
min=1
max=60
# 游戏次数
count=0

# 2.游戏开始
echo "猜数字游戏，范围：1～60"
read -p "请输入一个1～60范围内的数字" input
while :
do
    if [ $input -eq $randNum ];    #相等
    then
	count=$[$count+1]
	echo "恭喜猜对！一共猜了$count次"
	exit  # 退出
    elif [ $input -gt $randNum ];   #偏大
    then
	count=$[$count+1]
	max=$input
	read -p "猜大了，请输入一个$min~$max之间的数" input
    else                         #偏小
	count=$[$count+1]
	min=$input
	read -p "猜小了，请输入一个$min~$max之间的数" input
    fi
done






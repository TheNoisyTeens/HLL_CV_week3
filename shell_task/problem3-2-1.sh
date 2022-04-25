#! /bin/bash

##############################################################
# File Name:  	  problem3-2-1.sh
# Version:	  V1.0
# Author:      	  Zhangmin
# Created Time :  2017-04-25 
# Description: 	  减法操作
##############################################################

# -p命令 开启提示 
read -p "请输入被减数：" num1
read -p "请输入减数：" num2

:<<EOF
 判断输入变量是否是数字：
 使用sed替换后判断是否为空，为空则输入的是数字
EOF
if [ -z "$(echo $num1 | sed 's#[0-9]##g')" -a -z "$(echo $num2 | sed 's#[0-9]##g')" ];
then
	# 用变量result存储结果
	result=$[$num1-$num2]
	echo "$num1-$num2=$result"
else
	echo "输入数据无法进行减法操作"
fi

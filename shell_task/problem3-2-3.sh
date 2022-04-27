##############################################################
# File Name:  	  problem3-2-2.sh
# Version:	  V1.0
# Author:      	  Zhangmin
# Created Time :  2022-04-25 
# Description: 	  利用bash for循环打印下面这句话中字母数不大于6的单词
##############################################################

#!/bin/bash
words="I am oldboy teacher welcome to oldboy training class."

# 遍历这个字符串
for i in $word_6
do
 if [ ${#i} -lt 7 ]; #字符数小于7
 then
   echo $i  # 输出
 fi
done


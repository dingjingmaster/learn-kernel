#!/bin/bash

my_arm_home='/data/environment/gcc-arm'
my_path=$PATH
my_path=${my_arm_home}/bin:${my_path}
export PATH=$(echo ${my_path} | sed 's/:/\n/g' | sort | uniq | tr -s '\n' ':' | sed 's/:$//g')
echo $PATH

#!/bin/bash

if [ $# == 0 ]
then
    rm -rf /Redis/*.rdb /Redis/*.log
else
    ports=($*)
    for port in ${ports[*]}
    do
        rm -rf /Redis/${port}.rdb 
        rm -rf /Redis/${port}.log
        echo "remove ${port} rdb and log files."
    done
fi


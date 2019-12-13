#!/bin/bash

if [ $# == 0 ]
then
    ports=(6379 6380 6381 6382 6383)
else
    ports=($*)
fi

for port in ${ports[*]}
do
    ../../redis-4.0.8/src/redis-cli -h 127.0.0.1 -p ${port} SHUTDOWN
    echo "server ${port} shutdown."
done

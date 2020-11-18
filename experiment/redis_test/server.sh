#!/bin/bash

if [ $# == 0 ]
then
    ports=(6379 6380 6381 6382 6383)
else
    ports=($*)
fi

for port in ${ports[*]}
do
    /Redis/RWTree/redis-4.0.8/src/redis-server ./${port}.conf
    echo "server ${port} started."
done

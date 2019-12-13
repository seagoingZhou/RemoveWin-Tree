#!/bin/bash

REDIS_PORT=$1

../redis-4.0.8/src/redis-cli -h 127.0.0.1 -p ${REDIS_PORT}

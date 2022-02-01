#!/bin/sh
redis-server --protected-mode no --logfile /tmp/redis.txt --loadmodule /src/module.so
redis-cli acl setuser default on '>'$1
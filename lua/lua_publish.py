#!/usr/bin/python3
import os
import redis
import sys

# argv[1] = redis host 
# argv[2] = redis port
# argv[3] = lua scripts absolute path

def get_redis_client():
    return redis.Redis(host=sys.argv[1], port=sys.argv[2], db=0, decode_responses=True)

def main():
  # Load lua code
    dir = os.getcwd()
    filename = sys.argv[3]
    file = open(os.getcwd() + "\\" + filename, "r")
    all_of_it = file.read()
    file.close()
    r = get_redis_client()
    ret = r.execute_command("schema.register_query_lua",filename,all_of_it)
    print(ret + " - " + filename)       

if __name__ == "__main__":
    main()

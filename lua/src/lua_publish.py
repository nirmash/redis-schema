#!/usr/bin/python3
import os
import redis
import sys

# argv[1] = redis host 
# argv[2] = redis port
# argv[3] = lua scripts absolute path

def get_redis_client():
    return redis.Redis(host='127.0.0.1', port=6379, db=0, decode_responses=True)

def main():
  # Load lua code
    dir = os.getcwd()
    filename = sys.argv[1]
    filePath = ""

    if (os.name == "nt"):
      filePath = os.getcwd() + "\\" + filename
    else:
      filePath = os.getcwd() + "/" + filename
    
    file = open(filePath,"r")

    all_of_it = file.read()
    file.close()
    r = get_redis_client()
    ret = r.execute_command("schema.register_query_lua",filename,all_of_it)
    print(ret + " - " + filename)       

if __name__ == "__main__":
    main()

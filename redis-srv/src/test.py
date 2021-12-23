#!/usr/bin/python3
import time
import os
import redis
import sys

# argv[1] = redis host 
# argv[2] = redis port
# argv[3] = lua scripts absolute path
# argv[4] = shared file location no trailing slash

#1. schema.string_rule - enforces a string length
#2. schema.regex_rule - enforces a RegEx
#3. schema.number_rule - checks for min max in a number value
#4. schema.enum_rule - takes a list of values for an enum type validation
#5. schema.list_rule - takes a name of a Redis list with values
#6. schema.del_rule - deletes a rule, also used internally when a rule is updated
#7. schema.upsert - call a command so data can be validated

#define FIELD_PREFIX "__*_schema_field_"
#define FIELD_LISTS "__*_schema_field_sorted_set_*__"

def get_redis_client():
    return redis.Redis(host=sys.argv[1], port=sys.argv[2], db=0, decode_responses=True)

def main():
  # Load lua code
    print('1/ Create string rule')
    r = get_redis_client()
    ret = r.execute_command("schema.string_rule","StringRule1",100)
    print('created string rule ret-val ' + ret)    
    sprocs = r.zrange('__*_schema_field_sorted_set_*__',0,-1)
    print ("rules: %s".format(),sprocs)
    if len(sprocs) > 0:
      print ("test 1 passed")
    else:
      print ("test 1 failed")
      return
    
    # Tests with key change and timer
    print('2/ Test delete rule')
    res = r.execute_command("schema.del_rule","StringRule1")
    sprocs = r.zrange('__*_schema_field_sorted_set_*__',0,-1)
    items = r.hgetall('__*_schema_field_StringRule1')
    if len(sprocs) == 0 and len(items) == 0:
      print ('test 2 passed')
    else: 
      print ('test 2 failed')
      return

if __name__ == "__main__":
    main()

# How to make Redis play nice with your data 
I love using Redis in my projects, it is a super-fast and scalable no-SQL data layer with a bunch of useful in-memory data structures. In some cases, however, it would be great to have schema support for the data in my Redis database. Imagine a case in which we have 2 different Microservices reading and writing customer profile data into Redis. It would save time and reduce my bug count if, for example, I don't have to check that a customer age is a positive number in several different components. 

## Redis Modules to the rescue
One of the best 'hidden gems' of Redis is the [Module APIs](https://redis.io/topics/modules-intro) which allows adding new commands and behaviors to Redis. Redis comes with a [C API](https://redis.io/topics/modules-api-ref) which is really easy-to-use (it even helps you avoid [memory leaks](https://redis.io/topics/modules-api-ref#section-automatic-memory-management-for-modules)) so even a part-time developer like myself is able to create one. In this article we will look at a module that can add data validation rules to Redis as well as implements a simple Table data type. The source code for this article is located at [https://github.com/nirmash/redis-schema](https://github.com/nirmash/redis-schema).

## The `schema` Redis module 
The Redis module in this article is comprised of three components:
1. **Rules** - The module allows developers to associate data validation rules with Redis Key name patterns. It supports string length, number range, RegEx, Enum and Picklist validation rules. Field patters are evaluated left to right (e.g. a string rule that has the `name_` pattern and defines max string length at 10 will apply to a Redis key called `name_1` but not `name1`).
2. **Tables** - The module can be used to create a table rule. A table rule defines a table name and a list of individual rules that act as columns. In a table, field names are strictly evaluated (e.g. a table that has a `firstName` field will not accept data for a field called `firstName1`). 
3. **Lua scripts** - Redis supports running server-side [Lua Scripts](https://redis.io/commands/eval. The module adds the ability to use Lua scripts as a way to implement queries or further validation on data. 

All metadata (e.g. the data validation rules or scripts) is saved in Redis keys. Redis commands were added to register rules and "upsert" values into the database. The [GitHub repo](https://github.com/nirmash/redis-schema) includes the source code for the module and the files needed to load it into a Docker environment. 

The rest of the article walks through building and launching the module in a Docker container, loading some test records and adding a Lua script to query them. 

## Pre-requisites
Using this sample requires the following.
1. Install [Git](https://git-scm.com/downloads)
2. Install [Docker](https://docs.docker.com/get-docker/)
3. Install [Docker Compose](https://docs.docker.com/compose/install/)

## Getting the environment up and running
We will start by cloning the Git repo. In a terminal window, type:
```
$ git clone https://github.com/nirmash/redis-schema.git
Cloning into 'redis-schema'...
remote: Enumerating objects: 174, done.
remote: Counting objects: 100% (174/174), done.
remote: Compressing objects: 100% (138/138), done.
remote: Total 174 (delta 36), reused 174 (delta 36), pack-reused 0
Receiving objects: 100% (174/174), 267.90 KiB | 1.05 MiB/s, done.
Resolving deltas: 100% (36/36), done.
```
Once done, the files will be under the `redis-schema` folder.

To start up the environment, type the following into the command window.
```
cd redis-schema/
```
To initiate the containers, use `launch.bat` (Windows CMD) or `./launch.sh` (MacOS \ Linux). The script pulls and builds 2 containers, one running a Redis server with the schema module loaded (`redis-schema_redis_1`) and another container with Lua installed and some test scripts to help demonstrate the `schema` module features.

```
./launch.sh
```
```
...
...
NAME                          COMMAND                  SERVICE             STATUS              PORTS
redis-schema_lua_debugger_1   "lua"                    lua_debugger        running
redis-schema_redis_1          "redis-server '--pro…"   redis               running             0.0.0.0:6379->6379/tcp
```
**Note:** The `docker-compose.yaml` manifest maps the Redis port (6379) from the Redis container to your local host. If you have the Redis client (`redis-cli`) installed on your local host you can connect to the Redis instance running on the `redis-schema_redis_1` container. The rest of this article assumes Redis client is not installed on your local host and leverages the `redis-schema_lua_debugger_1` to showcase the module capabilities.

## Connect to the `redis-schema_lua_debugger_1` container
We will use the `redis-schema_lua_debugger_1` as our environment. To get started, open a terminal window and type:
```
docker exec -it redis-schema_lua_debugger_1 bash
```
The container terminal should now show up.
```
root@f7f16310026d:/#
```
We can now connect to Redis and check that the `schema` module is up and running.
```
root@f7f16310026d:/# redis-cli -h 172.28.1.4 -p 6379
172.28.1.4:6379> 
```
**Note:** Since we are using docker compose, the Redis server host is exposed on `172.28.1.4` IP address listening on port `6379`.

Verify that the `schema` module is loaded by typing `schema.help`:
```
172.28.1.4:6379> schema.help
Module to add validation rules to Redis data
...
...
```

## Create some data validation rules for Redis keys

We will create a couple of schema rules for Redis keys and Redis lists. Let's imaging that we want to store a list of Vehicle Identification Numbers (VIN) in our Redis database. VIN numbers follow a strict format (upper case characters except for I, O, or Q with the last 6 characters being numeric and overall length of 17 characters). We will validate that a string meets the VIN format by using a [RegEx](https://en.wikipedia.org/wiki/Regular_expression) rule.

```
schema.regex_rule vin_ \b[(A-H|J-N|P|R-Z|0-9)]{17}\b
```
To execute validation rules on incoming data, the module includes a `key_upsert` command. This command takes a Redis command as its first parameter (e.g. `set`, `sadd`) a field name and the value(s). 

Inserting a valid VIN into a [Redis set](https://redis.io/commands/sadd) will look like:
```
schema.upsert_key sadd vin_list1 SALVA2AE4EH877002
```
```
172.28.1.4:6379> schema.upsert_key sadd vin_list1 SALVA2AE4EH877002
(integer) 1
172.28.1.4:6379> smembers vin_list1
1) "SALVA2AE4EH877002"
172.28.1.4:6379>
```
Trying an invalid VIN will look like: 
```
172.28.1.4:6379> schema.upsert_key sadd vin_list1 SALVA2AE4EH877002BBBDF
(error) No match for SALVA2AE4EH877002BBBDF on \b[(A-H|J-N|P|R-Z|0-9)]\{17\}\b expression
172.28.1.4:6379>
```
A simpler case would be storing model years for the cars in the list. We would want to make sure the value is reasonable (let's say 1940 - 2023). To do that we would:
```
schema.number_rule model_year_ 1940 2023
```
Setting the value for a `model_year_` (valid and invalid) key looks like:
```
172.28.1.4:6379> schema.upsert_key set model_year_1 1200
(error) 1200 is out is out of range (min: 1940 max: 2023)
172.28.1.4:6379> schema.upsert_key set model_year_1 1941
OK
```
## Create the "contacts" table
In some cases, we would want to store our data in a tabular form. Let's imagine we want to store a list of contacts in Redis and be able to filter and retrieve the data. We will start by creating the fields in our table and associate them with a new table we name `contacts`.
```
schema.string_rule firstName 20
schema.string_rule lastName 20
schema.number_rule age 0 150
schema.table_rule contacts firstName lastName age
```
Since Redis doesn't have a notion of tables we are storing the table data in [Redis hashsets](https://redis.io/commands/hset). The module is adding an `Id` key to table rows and increments the number of records in a separate counter key. Let's add some records to the `contacts` table and see how they are stored in the Redis database.

The module includes the `schema.upsert_row` command. This command takes a row index as the first parameter (-1 for a new row) followed by the table name and then a list of key value pairs (field name and value). Values are checked against the field rules (e.g. a contact age can't be larger that a 150).
```
schema.upsert_row -1 contacts firstName john lastName doe age 25
```
`schema.upsert_row` then creates a hashset called `contacts_1` (as it is the first record in the table) and a key called `__*_schema_table_pk_contacts` that stores the latest row index. If, for example, we want to update the age value in the record we just added we can use `schema.upsert_row` with 1 as the first parameter.
```
schema.upsert_row 1 contacts age 35
```
Let's insert some more data into our table so we can query it later.
```
schema.upsert_row -1 contacts firstName john lastName doe age 25

schema.upsert_row -1 contacts firstName jane lastName doe age 30

schema.upsert_row -1 contacts firstName alexander lastName hamilton age 45

schema.upsert_row -1 contacts firstName george lastName washington age 50

schema.upsert_row -1 contacts firstName ben lastName smith age 11
```
## Create and debug a Lua query script
Now that we have a table with some data, we need a way to query it. We may also want to run some more business logic that is specific to our data model. To do that, the module includes a way to register and execute [Lua Scripts](https://redis.io/commands/eval). Let's look at a script that returns all the data in the `contacts` table. 
```lua 
if KEYS[1] == nil then return "missing table name" end

local results = {}
local tableScanItems = {}
local i = 1
tableScanItems = redis.call("keys",KEYS[1] .. "_*")
for _, tableScanItem in next, tableScanItems do
    results[i] = redis.call("hgetall",tableScanItem)
    i = i + 1
end 
return results
```
Scripts are registered by using the `schema.register_query_lua` command. This command takes a script name and lua source code. The `redis-schema_lua_debugger` container includes a simple tool to load lua scripts from files. 

To use the tool, open a new terminal window and navigate to the `src` folder.
```
docker exec -it redis-schema_lua_debugger_1 bash
cd src
```
```
root@01b73af63926:/src#
```
Now let's load the select all script for our table. The `lua_publish.py` script takes the name of the Lua file we want to load. 
```
python3 lua_publish.py select_all.lua
```
```
Script Loaded - select_all.lua
root@01b73af63926:/src#
```
To use the Lua script we need to use the `schema.execute_query_lua` command. To select all the data in the `contacts` table we would:
```
schema.execute_query_lua select_all.lua contacts
```
```
1) 1) "Id"
   2) "3"
   3) "firstName"
   4) "alexander"
   5) "lastName"
   6) "hamilton"
   7) "age"
   8) "45"
....
5) 1) "Id"
   2) "4"
   3) "firstName"
   4) "george"
   5) "lastName"
   6) "washington"
   7) "age"
   8) "50"
```
**Note:** The `lua_publish.py` uses the name of the lua file as the name of the script that we would later call with the `schema.execute_query_lua` command.

To make developing the Lua scripts easier, we will use the [Lua debugger](https://redis.io/topics/ldb) that is built into Redis. We will debug `select_all.lua` by using the below command: 
```
redis-cli -h 172.28.1.4 -p 6379 --ldb --eval select_all.lua contacts
```
```
root@9272b3492e54:/src# redis-cli -h 172.28.1.4 -p 6379 --ldb --eval select_all.lua contacts
Lua debugging session started, please use:
quit    -- End the session.
restart -- Restart the script in debug mode again.
help    -- Show Lua script debugging commands.

* Stopped at 1, stop reason = step over
-> 1   if KEYS[1] == nil then return "missing table name" end
lua debugger> n
* Stopped at 3, stop reason = step over
-> 3   local results = {}
lua debugger> n
* Stopped at 4, stop reason = step over
-> 4   local tableScanItems = {}
lua debugger> n
* Stopped at 5, stop reason = step over
-> 5   local i = 1
lua debugger> n
* Stopped at 6, stop reason = step over
-> 6   tableScanItems = redis.call("keys",KEYS[1] .. "_*")
lua debugger> n
<redis> keys contacts_*
<reply> ["contacts_3","contacts_1","contacts_5","contacts_2","contacts_4"]
* Stopped at 7, stop reason = step over
-> 7   for _, tableScanItem in next, tableScanItems do
lua debugger>
```
**Note:** The lua debugger expect the actual lua file with the source code, make sure you execute the debugger from the folder that includes the lua files. 

## Conclusion
This article introduced the idea of using Redis Modules to create schema validation rules and leverages the built-in Lua scripting support in Redis to add server-side logic to your Redis backend. Check out the code on [GitHub repo](https://github.com/nirmash/redis-schema).

# How to make Redis play nice with your data 
I love using Redis in my projects, it is a super fast and scalable no-SQL data layer with a bunch of useful in-memory data structures. In some cases, however, it would be great to have somekind of schema support for the data in my Redis database. Imagine a case in which I have 2 different Microservices using the reading and writing customer profile data into Redis. It would save time and reduce my bug count if, for example, I don't have to check that a customer age is a positive number in several different components. 

## Redis Modules to the rescue
One of the best 'hidden gems' of Redis is the [Module APIs](https://redis.io/topics/modules-intro) which allows adding new commands and behaviors type to Redis. Redis comes with a [C API](https://redis.io/topics/modules-api-ref) which is really easy-to-use (it even helps you avoid [memory leaks](https://redis.io/topics/modules-api-ref#section-automatic-memory-management-for-modules)) so even a part-time developer like myself is able to create one. In this article I will show how to create a module that data validation rules to Redis as well as implements a simple Table data type. The code for this article is located [https://github.com/nirmash/redis-schema](https://github.com/nirmash/redis-schema).

## The `schema` Redis module 
The Redis module in this article is comprised of three components:
1. **Rules** - The module allows developers to associate data validation rules with Redis Key name patterns. The module supports string length, number range, RegEx, Enum and Picklist validation rules. Field patters are evaluated left to right (e.g. a string rule that has the `name_` pattern and defines max string length at 10 will apply to a Redis key called `name_1` but not `name1`).
2. **Tables** - The moduled can be used to create a table rule. A table rule defines a table name and a list of individual rules that are part of it. In a table, field names are strictly evaluated. 
3. **Lua scripts** - Redis supports running server-side [Lua Scripts](https://redis.io/commands/eval). The module adds the ability to use Lua scripts as a way to implement queries or further validation on data. 

All metadata (e.g. the data validation rules or scripts) is saved as Redis keys. Redis commands are added to register rules and "upsert" values into the database. The [GitHub repo](https://github.com/nirmash/redis-schema) includes the source code for the module and the files needed to load it into a Docker environment. 

This article walks through building and launching the module in a Docker container, loading some test data and adding a Lua script to query the data. 

## Pre-requisits
Using the samples requires the following.
1. Install [Git](https://git-scm.com/downloads)
2. Install [Docker](https://docs.docker.com/get-docker/)
3. Install [Docker Compose](https://docs.docker.com/compose/install/)

## Getting the environment up and running



redis-cli -h 172.28.1.4 -p 6379 --ldb --eval select_all.lua contacts

# An Example Redis Module
1. What problem was I trying to solve - managing Lua scripts. 
    The way Redis works today you have to bring your own script and there is no way to edit manage
2. Thinking about IDEs I went with VS Code, it has an awesome ecosystem of add-ons (Redis add-ons suck BTW)
3. Way I decided to solve it is with a module that stores and runs Lua scripts, I then added a simple add-on that publishes the Lua scripts
    redis-server --loadmodule Projects/redis_modules/lua_sprocs/src/module.so
4. I will show a few things:
    I will load the module, this module listens for incoming events redis-server --loadmodule Projects/redis_modules/lua_sprocs/src/module.so
5. Redis allow you to load a script and get a hash back. I keep a list of scripts and each script has a hash-set with all the script attributes. 
    Let's try 'hello world' - sc1.lua, run luasprocs.execute sc1.lua and show the key change
    Note the format we use to configure the metadata, the module parses it and keeps it in the hashset 
6. Event, we can call this script when a key changes and pass it the key name and value
    set dream something
    get sc2 
7. Another execution pattern that is very helpful is to run a CRON job 
    go to sc3.lua, show the CRON job, it runs every 15 seconds using a TTL EXPIRE
    go back to the terminal running the Redis and show the expiration every 5 seconds
    note that it pushes data into a list
    call lrange ttlLst 0 10
8. Finally, we may want to run a script from an async thread and call things like the file system 
    go to sc4.lua (it has runasync=yes) and call it by changing a key name. It takes the name and value of my key and puts it in a file.
9. Now let's look at the data model 
    lrange __*_Sproc_List_*__ 0 5
    hgetall __*_Sproc_Name_sc2.lua 
What is left to do: 
1. support more key types (just does key set, get now)
2. figure out how to load more Lua libraries for the parallel execution model or call a new process 
3. Add nicer UX for the sprocs in VS Code

redis-server --loadmodule Projects/redis_modules/lua_sprocs/src/module.so --logfile redis.log



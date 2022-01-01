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
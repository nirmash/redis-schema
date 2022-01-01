if KEYS[1] == nil then return "missing table name" end
if KEYS[2] == nil then return "missing field name" end
if KEYS[3] == nil then return "missing field value" end
local results = {}
local tableScanItems = {}
local tableScanItemValue = {}
local i = 1
tableScanItems = redis.call("keys",KEYS[1] .. "_*")
for _, tableScanItem in next, tableScanItems do
    tableScanItemValue = redis.call("hget",tableScanItem,KEYS[2])
    if tableScanItemValue == KEYS[3] then
        results[i] = redis.call("hgetall",tableScanItem)
        i = i + 1
    end 
end 
return results
--on_key_changed_name=test;on_key_changed_event=set;notification_key_name=ret_key
--"redis.call('set','test_result_sc1','pass') return redis.call('get','test_result_sc1')"
"return KEYS [1] .. KEYS[2]"
"return redis.call('KEYS' .. \n .. KEYS[1] .. '_')"

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

schema.string_rule firstName 20
schema.string_rule lastName 20
schema.string_rule email 20
schema.number_rule age 0 150
schema.table_rule contacts firstName lastName email age

schema.upsert_row -1 contacts firstName nir lastName mashkowski email nir@a.com age 51
schema.upsert_row -1 contacts firstName oren lastName mashkowski email oren@a.com age 47
schema.upsert_row -1 contacts firstName dafna lastName teva email daf@a.com age 45
schema.upsert_row -1 contacts firstName joshua lastName mashkowski email josh@a.com age 19
schema.upsert_row -1 contacts firstName ella lastName mashkowski email ella@a.com age 11

schema.register_query_lua test1 "return KEYS"
schema.register_query_lua test2 'local arr = {} arr[1] ="one" arr[2] = "two" return arr'
schema.execute_query_lua test1 me too
schema.execute_query_lua test2

schema.register_query_lua select_where 'if KEYS[1] == nil then return "missing table name" end if KEYS[2] == nil then return "missing field name" end if KEYS[3] == nil then return "missing field value" end local results = {} local tableScanItems = {} local tableScanItemValue = {} local i = 1 tableScanItems = redis.call("keys",KEYS[1] .. "_*") for _, tableScanItem in next, tableScanItems do tableScanItemValue = redis.call("hget",tableScanItem,KEYS[2]) print("tableScanItemValue: " .. tableScanItemValue) if tableScanItemValue == KEYS[3] then results[i] = redis.call("hgetall",tableScanItem) i = i + 1 end end return results'
schema.execute_query_lua select_where contacts lastName mashkowski
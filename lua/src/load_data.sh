redis-cli -h 172.28.1.4 -p 6379 schema.string_rule firstName 20
redis-cli -h 172.28.1.4 -p 6379 schema.string_rule lastName 20
redis-cli -h 172.28.1.4 -p 6379 schema.string_rule email 20
redis-cli -h 172.28.1.4 -p 6379 schema.number_rule age 0 150
redis-cli -h 172.28.1.4 -p 6379 schema.table_rule contacts firstName lastName email age

redis-cli -h 172.28.1.4 -p 6379 schema.upsert_row -1 contacts firstName john lastName doe email john@doe.com age 25
redis-cli -h 172.28.1.4 -p 6379 schema.upsert_row -1 contacts firstName jane lastName doe email jane@doe.com age 30
redis-cli -h 172.28.1.4 -p 6379 schema.upsert_row -1 contacts firstName alexander lastName hamilton email alex@hamilton.com age 45
redis-cli -h 172.28.1.4 -p 6379 schema.upsert_row -1 contacts firstName george lastName washington email george@washington.com age 50
redis-cli -h 172.28.1.4 -p 6379 schema.upsert_row -1 contacts firstName ben lastName smith email ben@smith.com age 11
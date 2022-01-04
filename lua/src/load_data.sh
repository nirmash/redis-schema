redis-cli -h 172.28.1.4 -p 6379 schema.string_rule firstName 20
redis-cli -h 172.28.1.4 -p 6379 schema.string_rule lastName 20
redis-cli -h 172.28.1.4 -p 6379 schema.number_rule age 0 150
redis-cli -h 172.28.1.4 -p 6379 schema.table_rule contacts firstName lastName email age

redis-cli -h 172.28.1.4 -p 6379 schema.upsert_row -1 contacts firstName john lastName doe age 25
redis-cli -h 172.28.1.4 -p 6379 schema.upsert_row -1 contacts firstName jane lastName doe age 30
redis-cli -h 172.28.1.4 -p 6379 schema.upsert_row -1 contacts firstName alexander lastName hamilton age 45
redis-cli -h 172.28.1.4 -p 6379 schema.upsert_row -1 contacts firstName george lastName washington age 50
redis-cli -h 172.28.1.4 -p 6379 schema.upsert_row -1 contacts firstName ben lastName smith age 11
#define REDISMODULE_EXPERIMENTAL_API

#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <unistd.h> 
#include <pthread.h>
#include <regex.h>

#include "../redismodule.h"
#include "../rmutil/ccronexpr.h"
#include "../rmutil/util.h"
#include "../rmutil/strings.h"
#include "../rmutil/test_util.h"
#include "../rmutil/lauxlib.h"
#include "../rmutil/lua.h"
#include "../rmutil/lualib.h"

#define FIELD_PREFIX "__*_schema_field_"
#define FIELD_LISTS "__*_schema_field_sorted_set_*__"
#define TABLE_KEY_PREFIX "__*_schema_table_pk_"
#define TABLE_PREFIX "__*_schema_table_"
#define TABLES_LIST "__*_schema_table_list_*__"

#define SPROC_INDEX_LIST "__*_Sproc_List_*__"
#define SPROC_NAME_PREFIX "__*_Sproc_Name_"
#define SPROC_HASH_KEY_NAME "hash"
#define SPROC_CODE_KEY_NAME "sproc_source"

#define FIELD_TYPE_STR    "string"
#define FIELD_TYPE_REGEX  "regex"
#define FIELD_TYPE_NUMBER "number"
#define FIELD_TYPE_ENUM   "enum"
#define FIELD_TYPE_PICKLIST  "picklist"

int is_number(char const* arg)
{
   int n = strlen(arg);
   for(int ii=0;ii<n;ii++){
     if(!isdigit(arg[ii]))
      return 0;
   }
   return 1;
}

int starts_with(const char * key_name, const char * rule_name)
{
    while(*rule_name)
    {
        if(*rule_name++ != *key_name++)
            return 0;
    }
    return 1;
}

int delete_rule (RedisModuleCtx *ctx, RedisModuleString* rule_name){
  RedisModuleString *keyName = RedisModule_CreateStringPrintf(ctx,"%s%s",FIELD_PREFIX, RedisModule_StringPtrLen(rule_name,NULL));
  RedisModule_Call(ctx,"DEL","s",keyName);
  return (int) RedisModule_CallReplyInteger(RedisModule_Call(ctx,"ZREM","cs",FIELD_LISTS,rule_name));
}

int add_rule_2_index(RedisModuleCtx *ctx, RedisModuleString* rule_name){
  
  size_t len = strlen(RedisModule_StringPtrLen(rule_name,NULL));
  if(len == 0)
    return RedisModule_ReplyWithSimpleString(ctx,"Negative rule name length");    
  
  RedisModule_Call(ctx,"ZADD","cls",FIELD_LISTS,(long long) len,rule_name);
  return len;
}

RedisModuleString* get_redis_array_element_value(RedisModuleCtx *ctx, RedisModuleCallReply *reply, size_t idx){
    RedisModuleCallReply *ele = RedisModule_CallReplyArrayElement(reply,idx);
    return RedisModule_CreateStringFromCallReply(ele);
}

RedisModuleString* get_key_rule(RedisModuleCtx *ctx, RedisModuleString *key_name){
  RedisModule_AutoMemory(ctx);

  RedisModuleCallReply *reply = NULL;

  reply = RedisModule_Call(ctx,"ZRANGE","cllc",FIELD_LISTS,0,-1,"REV");
  if(reply==NULL)
    return NULL;

  size_t items = RedisModule_CallReplyLength(reply);
  if(items<=0)
    return NULL;

  for (size_t j = 0; j < items; j++) {
    RedisModuleString *rName = get_redis_array_element_value(ctx,reply,j);
    if(starts_with(RedisModule_StringPtrLen(key_name,NULL),RedisModule_StringPtrLen(rName,NULL)) == 1)
      return rName;
  }
  return NULL;
}

RedisModuleString* execute_schema_rule (RedisModuleCtx *ctx, RedisModuleString *rule_name, RedisModuleString **argv, int argc){
  RedisModule_AutoMemory(ctx);

  RedisModuleString* hSetName = RedisModule_CreateStringPrintf(ctx,"%s%s",FIELD_PREFIX,RedisModule_StringPtrLen(rule_name,NULL));
  RedisModuleString* rule_type_name = NULL;  

  RedisModuleCallReply* iHashReply = RedisModule_Call(ctx,"hget","sc",hSetName,"key_pattern");
  RedisModuleString* rule_pattern = NULL;
  rule_pattern = RedisModule_CreateStringFromCallReply(iHashReply);
  if(rule_pattern == NULL){
    RedisModule_Log(ctx, "notice","NULL on finding the pattern");
    return NULL;
  }

  RedisModuleString* rule_type = NULL; 
  iHashReply = RedisModule_Call(ctx,"hget","sc",hSetName,"type");
  rule_type = RedisModule_CreateStringFromCallReply(iHashReply);
  //execute string rule for key or list set command
  rule_type_name = RedisModule_CreateStringPrintf(ctx,"%s",FIELD_TYPE_STR);
  if(RedisModule_StringCompare(rule_type,rule_type_name) == 0){
    //get the length 
    iHashReply = RedisModule_Call(ctx,"hget","sc",hSetName,"len");
    RedisModuleString* str_len_limit_str = RedisModule_CreateStringFromCallReply(iHashReply);
    int str_len_limit = atoi(RedisModule_StringPtrLen(str_len_limit_str,NULL));
    if (str_len_limit <=0)
      return NULL;
    //truncate if longer
    int str_len = 0;
    for(int ii=3;ii<argc;ii++){
      str_len = strlen(RedisModule_StringPtrLen(argv[ii],NULL));
      if(str_len > str_len_limit){
        RedisModuleString* ret_val = RedisModule_CreateStringPrintf(ctx,"String %s length (%d) violates limit (%d)",RedisModule_StringPtrLen(argv[ii],NULL),str_len,str_len_limit);
        return ret_val;
      }
    }
    return NULL;
  } 
  //execute regex rule for key or list set command
  rule_type_name = RedisModule_CreateStringPrintf(ctx,"%s",FIELD_TYPE_REGEX);
  if(RedisModule_StringCompare(rule_type,rule_type_name) == 0){
    iHashReply = RedisModule_Call(ctx,"hget","sc",hSetName,"regex");
    RedisModuleString* regex_exp = RedisModule_CreateStringFromCallReply(iHashReply);
    regex_t regex;
    RedisModuleString* regex_reply = NULL;
    regcomp(&regex,RedisModule_StringPtrLen(regex_exp,NULL),0);   
    for(int ii=3;ii<argc;ii++){
      int regex_res = regexec(&regex,RedisModule_StringPtrLen(argv[ii],NULL),0,NULL,0);
      if(regex_res == 0)
        continue;
      if (regex_res == REG_NOMATCH){
        regex_reply = RedisModule_CreateStringPrintf(ctx, "No match for %s on %s expression",RedisModule_StringPtrLen(argv[ii],NULL),RedisModule_StringPtrLen(regex_exp,NULL));
        return regex_reply;
      }else{
        regex_reply = RedisModule_CreateStringPrintf(ctx, "Error evaluating %s for %s expression",RedisModule_StringPtrLen(argv[ii],NULL),RedisModule_StringPtrLen(regex_exp,NULL));
        return regex_reply;
      }
    }
  }
  //execute number rule for key or list set command
  rule_type_name = RedisModule_CreateStringPrintf(ctx,"%s",FIELD_TYPE_NUMBER);
  if(RedisModule_StringCompare(rule_type,rule_type_name) == 0){
    iHashReply = RedisModule_Call(ctx,"hget","sc",hSetName,"min");
    int i_min = atoi(RedisModule_StringPtrLen(RedisModule_CreateStringFromCallReply(iHashReply),NULL));
    iHashReply = RedisModule_Call(ctx,"hget","sc",hSetName,"max");
    int i_max = atoi(RedisModule_StringPtrLen(RedisModule_CreateStringFromCallReply(iHashReply),NULL));
    for(int ii=3;ii<argc;ii++){
      RedisModuleString* num_reply = NULL;
      char num_str[20];
      strcpy(num_str,RedisModule_StringPtrLen(argv[ii],NULL));
      if(!is_number(num_str)){
        num_reply = RedisModule_CreateStringPrintf(ctx,"%s is not a number",RedisModule_StringPtrLen(argv[ii],NULL));
        return num_reply;
      }
      int i_number = atoi(num_str);
      if((i_number < i_min) || (i_number > i_max)){
        num_reply = RedisModule_CreateStringPrintf(ctx,"%d is out is out of range (min: %d max: %d)",i_number,i_min,i_max);
        return num_reply;
      }
    }
  }
  //execute enum rule for key or list set command
  rule_type_name = RedisModule_CreateStringPrintf(ctx,"%s",FIELD_TYPE_ENUM);
  if(RedisModule_StringCompare(rule_type,rule_type_name) == 0){
    iHashReply = RedisModule_Call(ctx,"hget","sc",hSetName,"enum_name");
    RedisModuleString* enum_name = RedisModule_CreateStringFromCallReply(iHashReply);
    for(int ii=3;ii<argc;ii++){
      RedisModuleCallReply* in_list = RedisModule_Call(ctx,"SISMEMBER","ss",enum_name,argv[ii]);
      if(RedisModule_CallReplyInteger(in_list) == 0){
        RedisModuleString* enum_reply = RedisModule_CreateStringPrintf(ctx,"%s is not found in the enum",RedisModule_StringPtrLen(argv[ii],NULL));
        return enum_reply;
      }
    }
  }
    //execute picklist rule for key or list set command
  rule_type_name = RedisModule_CreateStringPrintf(ctx,"%s",FIELD_TYPE_PICKLIST);
  if(RedisModule_StringCompare(rule_type,rule_type_name) == 0){
    iHashReply = RedisModule_Call(ctx,"hget","sc",hSetName,"picklist");
    RedisModuleString* picklist_name = RedisModule_CreateStringFromCallReply(iHashReply);
    for(int ii=3;ii<argc;ii++){
      RedisModuleCallReply* in_list = RedisModule_Call(ctx,"SISMEMBER","ss",picklist_name,argv[ii]);
      if(RedisModule_CallReplyInteger(in_list) == 0){
        RedisModuleString* picklist_reply = RedisModule_CreateStringPrintf(ctx,"%s is not found in the %s picklist",RedisModule_StringPtrLen(argv[ii],NULL),RedisModule_StringPtrLen(picklist_name,NULL));
        return picklist_reply;
      }
    }
  }
  return NULL;
}

int DeleteRule(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx);

  if (argc != 2) {
    return RedisModule_ReplyWithSimpleString(ctx,"Expecting rule name");
  }

  delete_rule(ctx, argv[1]);
  return RedisModule_ReplyWithSimpleString(ctx,"OK");

}

int LoadStringRule(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx);

  if (argc != 3) {
    return RedisModule_ReplyWithSimpleString(ctx,"Expecting key name pattern and string length");
  }

  delete_rule(ctx, argv[1]);

  char num_str[20];

  strcpy(num_str,RedisModule_StringPtrLen(argv[2],NULL));
  if(!is_number(num_str))
    return RedisModule_ReplyWithSimpleString(ctx,"Length param must be a number");
  
  if (atoi(num_str)<=0)
    return RedisModule_ReplyWithSimpleString(ctx,"Length param must be bigger than zero");

  RedisModuleString *keyName = RedisModule_CreateStringPrintf(ctx,"%s%s",FIELD_PREFIX, RedisModule_StringPtrLen(argv[1],NULL));
  RedisModule_Call(ctx,"HSET","scs",keyName,"key_pattern",argv[1]);
  RedisModule_Call(ctx,"HSET","scc",keyName,"type",FIELD_TYPE_STR);
  RedisModule_Call(ctx,"HSET","scs",keyName,"len",argv[2]);
  add_rule_2_index(ctx,argv[1]);

  return RedisModule_ReplyWithSimpleString(ctx,"OK");
}

int LoadRegExRule(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx);

  if (argc != 3) {
    return RedisModule_ReplyWithSimpleString(ctx,"Expecting key name pattern and RegEx");
  }

  delete_rule(ctx, argv[1]);

  RedisModuleString *keyName = RedisModule_CreateStringPrintf(ctx,"%s%s",FIELD_PREFIX, RedisModule_StringPtrLen(argv[1],NULL));
  RedisModule_Call(ctx,"HSET","scs",keyName,"key_pattern",argv[1]);
  RedisModule_Call(ctx,"HSET","scc",keyName,"type",FIELD_TYPE_REGEX);
  RedisModule_Call(ctx,"HSET","scs",keyName,"regex",argv[2]);
  add_rule_2_index(ctx,argv[1]);
  
  return RedisModule_ReplyWithSimpleString(ctx,"OK");
}

int LoadListRule(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx);

  if (argc != 3) {
    return RedisModule_ReplyWithSimpleString(ctx,"Expecting key name pattern and value list name");
  }
  delete_rule(ctx, argv[1]);

  RedisModuleString *keyName = RedisModule_CreateStringPrintf(ctx,"%s%s",FIELD_PREFIX, RedisModule_StringPtrLen(argv[1],NULL));
  RedisModule_Call(ctx,"HSET","scs",keyName,"key_pattern",argv[1]);
  RedisModule_Call(ctx,"HSET","scc",keyName,"type",FIELD_TYPE_PICKLIST);
  RedisModule_Call(ctx,"HSET","scs",keyName,"picklist",argv[2]);
  add_rule_2_index(ctx,argv[1]);

  return RedisModule_ReplyWithSimpleString(ctx,"OK");
}

int LoadEnumRule(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx);

  if (argc < 2) {
    return RedisModule_ReplyWithSimpleString(ctx,"Expecting key name and a list of values");
  }
  //key_pattern 1
  //value 2
  delete_rule(ctx, argv[1]);

  RedisModuleString *keyName = RedisModule_CreateStringPrintf(ctx,"%s%s",FIELD_PREFIX, RedisModule_StringPtrLen(argv[1],NULL));
  RedisModuleString *listName = RedisModule_CreateStringPrintf(ctx,"%s_%s_%s",FIELD_PREFIX,FIELD_TYPE_ENUM, RedisModule_StringPtrLen(argv[1],NULL));

  RedisModule_Call(ctx,"HSET","scs",keyName,"key_pattern",argv[1]);
  RedisModule_Call(ctx,"HSET","scc",keyName,"type",FIELD_TYPE_ENUM);
  RedisModule_Call(ctx,"HSET","scs",keyName,"enum_name",listName);
  for(int ii = 2; ii < argc; ii++){
    RedisModule_Call(ctx,"SADD","ss",listName,argv[ii]);  
  }
  add_rule_2_index(ctx,argv[1]);

  return RedisModule_ReplyWithSimpleString(ctx,"OK");
}

int LoadNumberRule(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx);

  if (argc != 4) {
    return RedisModule_ReplyWithSimpleString(ctx,"Expecting key name pattern and min and max value");
  }
  
  char num_str[20];
  int min, max;
  
  strcpy(num_str,RedisModule_StringPtrLen(argv[2],NULL));
  if(!is_number(num_str))
    return RedisModule_ReplyWithSimpleString(ctx,"Min param must be a number");
  min = atoi(num_str);

  strcpy(num_str,RedisModule_StringPtrLen(argv[3],NULL));
  if(!is_number(num_str))
    return RedisModule_ReplyWithSimpleString(ctx,"Max param must be a number");
  max = atoi(num_str);

  if(min > max){
    return RedisModule_ReplyWithSimpleString(ctx,"Min can't be larger than max");
  }

  delete_rule(ctx, argv[1]);

  RedisModuleString *keyName = RedisModule_CreateStringPrintf(ctx,"%s%s",FIELD_PREFIX, RedisModule_StringPtrLen(argv[1],NULL));
  RedisModule_Call(ctx,"HSET","scs",keyName,"key_pattern",argv[1]);
  RedisModule_Call(ctx,"HSET","scc",keyName,"type",FIELD_TYPE_NUMBER);
  RedisModule_Call(ctx,"HSET","scs",keyName,"min",argv[2]);
  RedisModule_Call(ctx,"HSET","scs",keyName,"max",argv[3]);
  add_rule_2_index(ctx,argv[1]);

  return RedisModule_ReplyWithSimpleString(ctx,"OK");
}

int LoadTableRule(RedisModuleCtx *ctx, RedisModuleString **argv, int argc){
  RedisModule_AutoMemory(ctx);

  if (argc < 3) {
    return RedisModule_ReplyWithSimpleString(ctx,"Expecting table name and a list of fields");
  }

  RedisModuleString *tableName = RedisModule_CreateStringPrintf(ctx,"%s%s",TABLE_PREFIX, RedisModule_StringPtrLen(argv[1],NULL));
  //make sure all the fields are valid
  for(int ii=2; ii < argc; ii++){
      RedisModuleCallReply* in_list = RedisModule_Call(ctx,"ZSCAN","clcs",FIELD_LISTS,0,"match",argv[ii]);
      RedisModuleCallReply* results_list = RedisModule_CallReplyArrayElement(in_list,1);
      size_t arr_len = RedisModule_CallReplyLength(results_list);
      if(arr_len == 0){
        RedisModuleString* field_eval = RedisModule_CreateStringPrintf(ctx,"Table field %s doesn't exist",RedisModule_StringPtrLen(argv[ii], NULL));
        return RedisModule_ReplyWithError(ctx,RedisModule_StringPtrLen(field_eval,NULL));
      }
  }
  //clean up table definition if exists
  RedisModule_Call(ctx,"DEL","s",tableName);
  for(int ii=2; ii < argc; ii++){
    RedisModule_Call(ctx,"SADD","ss",tableName,argv[ii]);
  }
  return RedisModule_ReplyWithSimpleString(ctx,"OK");
}

int ExecuteQueryLua(RedisModuleCtx *ctx, RedisModuleString **argv, int argc){
  RedisModule_AutoMemory(ctx); 

  if (argc < 2) {
    return RedisModule_ReplyWithSimpleString(ctx,"Expecting a script name and optional parameters");
  }

  RedisModuleString* sprocName = RedisModule_CreateStringPrintf(ctx,"%s%s",SPROC_NAME_PREFIX,RedisModule_StringPtrLen(argv[1],NULL));
  printf("sproc name is: %s\n",RedisModule_StringPtrLen(sprocName,NULL));
  RedisModuleString* sprocHash = RedisModule_CreateStringFromCallReply(RedisModule_Call(ctx,"HGET","sc",sprocName,SPROC_HASH_KEY_NAME));
  printf("sproc hash is: %s\n",RedisModule_StringPtrLen(sprocHash,NULL));
  RedisModuleCallReply* sprocRet = NULL;

  if(argc > 2){
    sprocRet = RedisModule_Call(ctx,"EVALSHA","slv",sprocHash,(long long)(argc-2),argv+2); 
  }else{
    sprocRet = RedisModule_Call(ctx,"EVALSHA","sl",sprocHash,(long long)0);
  }
  return RedisModule_ReplyWithCallReply(ctx,sprocRet);
}

int RegisterQueryLua(RedisModuleCtx *ctx, RedisModuleString **argv, int argc){

  RedisModule_AutoMemory(ctx); 

  if (argc < 3) {
    return RedisModule_ReplyWithSimpleString(ctx,"Expecting query name and lua code");
  }
  
  RedisModuleString *sprocName = NULL;
  RedisModuleString *code = NULL;
  RedisModuleCallReply *rep = NULL;

  sprocName = argv[1];
  code = argv[2];

  RedisModuleString *hash = RedisModule_CreateStringPrintf(ctx,"0");
  RedisModuleCallReply *reply = RedisModule_Call(ctx,"SCRIPT","cs","LOAD",code);
  hash  = RedisModule_CreateStringFromCallReply(reply);
  RedisModuleString *prprSprocName = RedisModule_CreateStringPrintf(ctx,"%s%s",SPROC_NAME_PREFIX,RedisModule_StringPtrLen(sprocName,NULL));
  rep = RedisModule_Call(ctx,"SADD","cs",SPROC_INDEX_LIST,prprSprocName);
  RedisModule_Call(ctx,"HSET","scs",prprSprocName,SPROC_CODE_KEY_NAME,code);
  RedisModule_Call(ctx,"HSET","scs",prprSprocName,SPROC_HASH_KEY_NAME,hash);
  
  return RedisModule_ReplyWithSimpleString(ctx,"Script Loaded");
}

int Help(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx); 
  return RedisModule_ReplyWithSimpleString(ctx,"Module to add validation rules to Redis data\nThere are two entities in this module:\n Fields that can be added with a validation rule.\n Tables that are implemented as a Redis Hashsets (a collection of fields) are created by calling schema.table_rule.\nData can be validated by calling upsert commands. To add or update data in an individual field use schema.upsert, this command evaluates field names from left to right and applies the value per the validation rule (command not applied if data is invalid). Rows can be added or updated by calling the upsert_row command which expects the row index (-1 for new) a table name and key value pairs. The field names are validated as an exact string compare and validated against the field data type rule.\nAll field commands expect a unique beginning of the key name to be evaluated (it will be matched from left on when validating)\nCommands available:\n1. schema.string_rule - enforces a string length\n2. schema.regex_rule - enforces a RegEx\n3. schema.number_rule - checks for min max in a number value\n4. schema.enum_rule - takes a list of values for an enum type validation\n5. schema.list_rule - takes a name of a Redis list with values\n6. schema.del_rule - deletes a rule, also used internally when a rule is updated\n7. schema.upsert - call a command so data can be validated\n8. schema.table_rule - add a list of field to create a row definition\n9. schema.upsert_row - creates or updates a row in a table, params are row index (-1 for new) table name and field name value pairs");
}

int UpsertRow(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx);
  if (argc < 3) {
    return RedisModule_ReplyWithSimpleString(ctx,"Expecting table (hash) name, row id (-1 for new) and arguments");
  }
  //check that the table exits
  RedisModuleString *tblName = RedisModule_CreateStringPrintf(ctx,"%s%s",TABLE_PREFIX, RedisModule_StringPtrLen(argv[2],NULL));
  RedisModuleCallReply* reply = RedisModule_Call(ctx,"KEYS","s",tblName);

  if(RedisModule_CallReplyLength(reply)<=0){
    return RedisModule_ReplyWithSimpleString(ctx,"Table name supplied doesn't exist, use schema.table_rule to create");
  }
  
  for(int ii=3; ii<argc; ii=ii+2){
    //check that all the fields are valid
    RedisModuleString* rule_name = get_key_rule(ctx,argv[ii]);
    if(rule_name == NULL){
      RedisModule_ReplyWithError(ctx,"Key name invalid");
      return REDISMODULE_OK;
    }      
    RedisModuleString* rule_eval = NULL;
    RedisModuleString* vArr[1];
    vArr[0] = RedisModule_CreateStringPrintf(ctx,"%s",RedisModule_StringPtrLen(argv[ii+1],NULL));
    rule_eval = execute_schema_rule(ctx,rule_name,vArr,1);
    if (rule_eval != NULL){
        RedisModule_ReplyWithError(ctx, RedisModule_StringPtrLen(rule_eval,NULL));
        return REDISMODULE_OK;
    }    
  }
  //create the primary key for the row
  int row_key = atoi(RedisModule_StringPtrLen(argv[1],NULL));
  RedisModuleString* set_name = NULL;
  RedisModuleString* row_key_name = NULL;
  row_key_name = RedisModule_CreateStringPrintf(ctx,"%s%s",TABLE_KEY_PREFIX,RedisModule_StringPtrLen(argv[2],NULL));
  if(row_key == -1){
    RedisModuleCallReply* key_rep = RedisModule_Call(ctx, "INCR","s",row_key_name);
    row_key = (int) RedisModule_CallReplyInteger(key_rep);
  }
  //do the hset
  set_name = RedisModule_CreateStringPrintf(ctx,"%s_%d",RedisModule_StringPtrLen(argv[2],NULL),row_key);
  RedisModuleCallReply* rep = RedisModule_Call(ctx, "HSET", "sv", set_name,argv + 3, argc - 3); 
  RedisModule_ReplyWithCallReply(ctx,rep);
  return REDISMODULE_OK;
}

int UpsertKey(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {

  RedisModule_AutoMemory(ctx);

  if (argc < 3) {
    return RedisModule_ReplyWithSimpleString(ctx,"Expecting command name and arguments");
  }

  const char* cmd = RedisModule_StringPtrLen(argv[1], NULL);

  RedisModuleString* rule_name = get_key_rule(ctx,argv[2]);
  if(rule_name == NULL)
    RedisModule_Log(ctx, "notice","NULL on finding the key");
  else
    RedisModule_Log(ctx, "notice","Found the rule, it is: %s",RedisModule_StringPtrLen(rule_name,NULL));

  RedisModuleString* rule_eval = NULL;
  rule_eval = execute_schema_rule(ctx,rule_name,argv,argc);
  if (rule_eval != NULL){
      RedisModule_ReplyWithError(ctx, RedisModule_StringPtrLen(rule_eval,NULL));
      return REDISMODULE_OK;
  }

  RedisModuleCallReply* rep = RedisModule_Call(ctx, cmd, "v", argv + 2, argc - 2);
  if(!rep){
      RedisModule_ReplyWithError(ctx, "NULL reply returned");
  }else{
      RedisModule_ReplyWithCallReply(ctx, rep);
  }

  return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx) {

  // Register the module itself
  if (RedisModule_Init(ctx, "schema", 1, REDISMODULE_APIVER_1) ==
      REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }

  // Register schema - using the shortened utility registration macro
  RMUtil_RegisterWriteCmd(ctx, "schema.string_rule", LoadStringRule);
  RMUtil_RegisterWriteCmd(ctx, "schema.regex_rule", LoadRegExRule);
  RMUtil_RegisterWriteCmd(ctx, "schema.number_rule", LoadNumberRule);
  RMUtil_RegisterWriteCmd(ctx, "schema.enum_rule", LoadEnumRule);
  RMUtil_RegisterWriteCmd(ctx, "schema.list_rule", LoadListRule);
  RMUtil_RegisterWriteCmd(ctx, "schema.table_rule", LoadTableRule);
  RMUtil_RegisterWriteCmd(ctx, "schema.del_rule", DeleteRule);
  RMUtil_RegisterWriteCmd(ctx, "schema.upsert_key", UpsertKey);
  RMUtil_RegisterWriteCmd(ctx, "schema.upsert_row", UpsertRow);
  RMUtil_RegisterWriteCmd(ctx, "schema.register_query_lua", RegisterQueryLua);
  RMUtil_RegisterWriteCmd(ctx, "schema.execute_query_lua", ExecuteQueryLua);
  RMUtil_RegisterWriteCmd(ctx, "schema.help", Help);
  
  return REDISMODULE_OK;
}
import datetime
import time
import daemon
import redis
import os
import threading

def get_redis_client():
    return redis.Redis(host='172.28.1.4', port=6379, db=0, decode_responses=True)
 
def RedisCheck():
    
    isConnected = False
    while isConnected == False: 
        f = open("/tmp/deamon.txt","w")
        f.write("umm... hello? ")
        f.flush()
        try:
            r = get_redis_client()
            p = r.pubsub()
            p.subscribe(os.getenv('REDIS_SUB_CHANNEL'))
            f.write("connected ")
            f.flush()
            isConnected = True
            PAUSE = True
        except:
            f.write("error ")
            f.flush()
            isConnected = False
            time.sleep(1)
        f.write("rev" )
        f.close()

    while PAUSE:
        f = open("/tmp/deamon.txt","w")
        print("Waiting For redisStarter...")
        f.write("Waiting For redisStarter...")
        f.flush()
        message = p.get_message()
        if message:
            command = message['data']
            f.write("command is: {}\n".format(command))
            f.flush()
            trd = threading.Thread(target=doWork,args=(command, ))
            print ('command: {0} pause is: {1}\n'.format(command,PAUSE))
            trd.start()
            if command == 'START':
                print ('command line 25: {0} pause is: {1}\b'.format(command,PAUSE))
                PAUSE = False

        time.sleep(0.5)
        f.close()
    print("Permission to start...") 

def doWork (command):
    f = open("/tmp/command.txt","w")
    f.write("command is: {0}\n".format(command))
    f.flush()
    print(command)
    os.system(command)
    f.write("after command\n")
    f.close()

with daemon.DaemonContext():
    RedisCheck()
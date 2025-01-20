#!/usr/bin/python3
from __future__ import print_function
from datetime import datetime
import paho.mqtt.client as mqtt
import psutil
import json
import time
import base64
import string
import pymongo
import traceback
import flatdict

# NETJSON filereader
def load_blueprint(config_file):
    with open(config_file, 'r') as f:
        config = json.load(f)
    
    mqtt_config = config.get('mqtt', {})
    mongo_config = config.get('mongodb', {})
    
    broker = mqtt_config.get('broker', 'localhost')
    port = mqtt_config.get('port', 1883)
    client_id = mqtt_config.get('client_id', 'mqtt-client')
    username = mqtt_config.get('username', 'user')
    password = mqtt_config.get('password', 'pass')
    topics = mqtt_config.get('topics', ["application/+/device/+/event/#"])
    
    mongo_db_host = mongo_config.get('host', 'localhost')
    mongo_db_port = mongo_config.get('port', 27017)
    mongo_db_name = mongo_config.get('database', 'mydatabase')
    
    return broker, port, client_id, username, password, topics, mongo_db_host, mongo_db_port, mongo_db_name

# load blueprint
broker, port, client_id, username, password, topics, mongo_db_host, mongo_db_port, mongo_db_name = load_blueprint("blueprint.json")

# MongoDB connection
myclient = pymongo.MongoClient(f"mongodb://{mongo_db_host}:{mongo_db_port}/")
mydb = myclient[mongo_db_name]

# Callback MQTT
def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {str(rc)}")
    for topic in topics:
        client.subscribe(topic)

def on_message(client, userdata, msg):
    print(f"Message received-> {msg.topic} {str(msg.payload)}")
    try:
        mm = json.loads(msg.payload)
        data_object = None
        try:
            data_object = base64.b64decode(mm["data"]).decode('utf8')
        except Exception:
            traceback.print_exc()
        try:
            data_object = mm["object"]
        except Exception:
            traceback.print_exc()
        if data_object:
            data = {
                "datetime": datetime.now(),
                "rxInfo": mm["rxInfo"][0],
                "txInfo": mm["txInfo"],
                "fCnt": mm["fCnt"],
                "data": mm["data"],
                "object": data_object
            }
        
        flatDict = False
        if flatDict:
            data = flatdict.FlatDict(data, delimiter='_')
            print(data)
            x = mydb[msg.topic.replace("-", "_").replace("/", "_")].insert_one(data)
        else:
            x = mydb[msg.topic].insert_one(data)
        print(x)
    except Exception as e:
        print(e)

# Funzione per avviare il client MQTT
def run():
    client = mqtt.Client(client_id)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(broker, port)
    client.loop_forever()

run()


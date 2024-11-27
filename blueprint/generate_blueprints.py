import json
import random
import math
# Set the number of devices and gateways

# Set the latitude and longitude of UNIPA
unipa_lat = 38.10391610180684
unipa_lon = 13.345738017336016
username="<username here>"
password="<passowrd here>"
mqtt_broker_hostname="<mqtt broker hostname>" #"lab.tti.unipa.it"
network_server_addr = "<network server addr>"
mqtt_broker_port=8883
mongodb_database_hostname = "<mongodb database hostname>" #"10.8.9.27"
mongodb_port = 27017
api_token = "<API token Here>"
# Set the radius (in kilometers) within which to distribute the gateways
blueprint_id=0

from geopy.distance import geodesic
import random
import math


import pandas as pd


nDevices_list=[5, 10, 20, 50, 100]
nGateways_list=[5, 10, 20, 30]
radius_list=[1,6]
output_path="blueprint_test/"
randList = range(0,50)

generate_samples_few_shot_simulation=True

if generate_samples_few_shot_simulation:
    nDevices_list=[10,100]
    nGateways_list=[1,10]
    radius_list=[1,6]
    output_path="simulation/ns3/lorawan/blueprint/"
    randList = range(0,1)




# Step 1: Read the CSV file into a DataFrame
deviceNames = pd.read_csv('smart_cities_devices.csv')

# Step 2: Shuffle the rows of the DataFrame
#deviceNames = deviceNames.sample(frac=1).reset_index(drop=True)  # frac=1 means shuffle all rows, reset_index to reset the index after shuffle


def generate_random_coordinates(lat, lon, radius):
    # Generate random angle and distance
    angle = random.uniform(0, 2 * math.pi)
    distance = random.uniform(0, radius)

    # Calculate new latitude and longitude
    new_coordinates = geodesic(kilometers=distance).destination((lat, lon), angle * 180 / math.pi)

    return new_coordinates.latitude, new_coordinates.longitude

for radius in radius_list: 
    for nDevices in nDevices_list:
        for nGateways in nGateways_list:
            #shuffle devices names
            deviceNames = deviceNames.sample(frac=1).reset_index(drop=True)  # frac=1 means shuffle all rows, reset_index to reset the index after shuffle
            for rand_id in randList:
                print(f"[{blueprint_id}] nDevices={nDevices} nGateways={nGateways}")
                # Create the JSON object
                mqtt_config={
                    "mqtt": {
                        "broker": mqtt_broker_hostname,#"lab.tti.unipa.it",
                        "port": mqtt_broker_port,
                        "client_id": "mqtt-db-logger",
                        "username": username,
                        "password": password,
                        "topics": [
                            "application/+/device/+/event/#"
                            ]
                        },
                    "mongodb": {
                        "host": mongodb_database_hostname, 
                        "port": mongodb_port,
                        "database": "Chirpstack-Supernova"
                    },
                }
                
                
                config = {
                    "type": "configuration",
                    "version": "1.0",
                    
                    "mqtt": {
                        "broker": mqtt_broker_hostname,#"lab.tti.unipa.it",
                        "port": mqtt_broker_port,
                        "client_id": "mqtt-db-logger",
                        "username": username,
                        "password": password,
                        "topics": [
                            "application/+/device/+/event/#"
                            ]
                        },
                    "mongodb": {
                        "host": mongodb_database_hostname, 
                        "port": mongodb_port,
                        "database": "Chirpstack-Supernova"
                    },
                    
                    "network": {
                        "id": "lorawan-network",
                        "name": "LoRaWAN Network",
                        "network_server": {
                            "id": "network-server-1",
                            "name": "ChirpStack Network Server",
                            "address": network_server_addr,
                            "api_token": api_token
                        },
                        "coverage":{
                            "radius":radius
                        },
                        "gateways": [],
                        "devices": []
                    }
                }

                # Generate the gateways
                for i in range(1, nGateways + 1):
                    lat, lon = generate_random_coordinates(unipa_lat,unipa_lon,radius)

                    # Add the gateway to the JSON object
                    config["network"]["gateways"].append({
                        "id": f"gateway-{i}",
                        "name": f"LoRa Gateway {i}",
                        "address": f"192.168.1.{100 + i}",
                        "latitude": lat,
                        "longitude": lon,
                        "frequency_band": "EU868",
                        "model": f"gateway-model-{i}"
                    })

                # Generate the devices
                for i in range(1, nDevices + 1):
                    lat, lon = generate_random_coordinates(unipa_lat,unipa_lon,radius)

                    # Generate random dev_eui value as EUI64
                    dev_eui = format(random.randint(0, 2**64 - 1), "016X")

                    # Set app_eui value to all zeros
                    app_eui = "0000000000000000"

                    # Set app_key value to a random 128-bit hexadecimal value
                    app_key = format(random.randint(0, 2**128 - 1), "032X")
                    sf = 7 #ALL DEVICES WITH SAME SF, TODO: add ADR or SF Planning
                    # Add the device to the JSON object
                    config["network"]["devices"].append({
                        "id": f"device-{i}",
                        #"name": f"LoRa Device {i}",
                        "name": f"{deviceNames.iloc[i%len(deviceNames)]['Device Name']}",
                        "description":f"{deviceNames.iloc[i%len(deviceNames)]['Description']}",
                        "DR":sf,
                        "dev_eui": dev_eui,
                        "app_eui": app_eui,
                        "app_key": app_key,
                        "profile": "sensor-profile-xyz",
                        "latitude": lat,
                        "longitude": lon
                    })    
                # Write the JSON object to a file
                id = format(blueprint_id, "04d")
                with open(f"{output_path}{id}_blueprint_nDevices_{nDevices}_nGateways_{nGateways}-radius-{radius}.json", "w") as f:
                    json.dump(config, f, indent=2)
                blueprint_id+=1

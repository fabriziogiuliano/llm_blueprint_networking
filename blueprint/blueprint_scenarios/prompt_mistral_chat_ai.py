

import os
from mistralai import Mistral
from dotenv import load_dotenv
load_dotenv()
model = "mistral-large-latest"
client = Mistral(api_key=os.getenv('API_KEY'))

with open("sample_blueprint.json") as f:
        sample_blueprint = f.read()
with open("sample_ns3_code.cc") as f:
        sample_ns3_code = f.read()

#blueprint_scenario_filenames=["smart_home","smart_home_mixed","smart_agriculture","smart_city"]
blueprint_scenario_filenames=["smart_home_mixed"]
for blueprint_scenario_filename in blueprint_scenario_filenames:
    print(f"OPEN: {blueprint_scenario_filename}...")
    with open(f"blueprints/{blueprint_scenario_filename}.json") as f:
            test_blueprint = f.read()

    prompt = f"""
    Given the following SAMPLE BLUEPRINT and the corresponding SAMPLE NS-3 code, use them as a sample to generate the NS3 code for the TEST BLUEPRINT. 
    
    Do Not use WiFi is not used in the TEST BLUEPRINT
    Do Not use LoRa is not used in the TEST BLUEPRINT
    
    In case of WiFi nodes take into accounts this:
    - wifi.SetStandard(WIFI_STANDARD_80211a); // Set Wi-Fi standard to 802.11a
    - declare YansWifiChannelHelper with "wifichannel", to avoid conflict in the code variables
    - initialize YansWifiChannelHelper using "YansWifiChannelHelper::Default ()"
    - "YansWifiPhyHelper phy = YansWifiPhyHelper::Default();" is wrong use "YansWifiPhyHelper phy;" instead
    - add a tracker for Udp and LoRa to save experiment outputs to file.
    FOR SCENARIOS WITH WIFI NODES:
        must includes following libraries:
            #include "ns3/yans-wifi-helper.h"
            #include "ns3/on-off-helper.h"
            #include "ns3/inet-socket-address.h"
            #include "ns3/packet-sink-helper.h"
            #include "ns3/ssid.h"
            #include "ns3/internet-stack-helper.h"
            #include "ns3/ipv4-address-helper.h"
        
        Set up mobility for WiFi nodes in this way:
        
        Ptr<ListPositionAllocator> allocatorAPWiFi = CreateObject<ListPositionAllocator>();    
        allocatorAPWiFi->Add(Vector(<lat>, <lon>, <height>)); // position depends on blueprint elements
        allocatorAPWiFi->Add(Vector(<lat>, <lon>, <height>)); // position depends on blueprint elements
        allocatorAPWiFi->Add(Vector(<lat>, <lon>, <height>)); // position depends on blueprint elements
        mobility.SetPositionAllocator(allocatorAPWiFi); 
        
        Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();    
        allocatorStaWiFi->Add(Vector(<lat>, <lon>, <height>)); // position depends on blueprint elements
        allocatorStaWiFi->Add(Vector(<lat>, <lon>, <height>)); // position depends on blueprint elements
        allocatorStaWiFi->Add(Vector(<lat>, <lon>, <height>)); // position depends on blueprint elements
        mobility.SetPositionAllocator(allocatorStaWiFi); 
        
        mobility.Install(wifiNodes);
        
        If <height> not given set 1.5 meters

    Generate only the code without "```cpp" quote it will be saved to a .cc file:

            SAMPLE BLUEPRINT:

            {sample_blueprint}

            SAMPLE NS3 CODE:

            {sample_ns3_code}

            TEST BLUEPRINT:

            {test_blueprint}

            NS3 CODE:

            """


    chat_response = client.chat.complete(
        model = model,
        temperature=0,
        messages = [
            {
                "role": "user",
                "content": prompt,
            },
        ]
    )

    response = chat_response.choices[0].message.content
    output_filename = f"ns3_output_code/{blueprint_scenario_filename}.cc"
    with open(output_filename, 'w') as file:
        file.write(response.replace("```cpp","").replace("```",""))
    print(f"SAVED TO {output_filename}")
    print("------------------------------")
print("FINISHED!")
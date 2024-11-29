

import os
from mistralai import Mistral

model = "mistral-large-latest"

client = Mistral(api_key=api_key)
blueprint_scenario_filename="smart_home"
with open("sample_blueprint.json") as f:
        sample_blueprint = f.read()
with open("sample_ns3_code.cc") as f:
        sample_ns3_code = f.read()
with open(f"blueprints/{blueprint_scenario_filename}.json") as f:
        test_blueprint = f.read()


fine_tuning="""
  
"""
#Solve this error:
#//cond="socketFactory", msg="CreateSocket: can not create a ns3::UdpSocketFactory - perhaps the node is missing the required protocol.", +1.000000000s 4 file=/home/testbed5g/NS3-AI/ns-3-dev/src/network/model/socket.cc

prompt = f"""
Given the following SAMPLE BLUEPRINT and the corresponding SAMPLE NS3 code, use them as a sample to generate the NS3 code for the TEST BLUEPRINT. 

In case of WiFi nodes take into accounts this:
1. wifi.SetStandard(WIFI_STANDARD_80211a); // Set Wi-Fi standard to 802.11a
2. declare YansWifiChannelHelper with "wifichannel", to avoid conflict in the code variables
3. "YansWifiPhyHelper phy = YansWifiPhyHelper::Default();" is wrong use "YansWifiPhyHelper phy;" instead

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
    
    Ptr<ListPositionAllocator> allocatorWiFi = CreateObject<ListPositionAllocator>();    
    allocatorWiFi->Add(Vector(<lat>, <lon>, 1.5)); // position depends on blueprint elements
    allocatorWiFi->Add(Vector(<lat>, <lon>, 1.5)); // position depends on blueprint elements
    allocatorWiFi->Add(Vector(<lat>, <lon>, 1.5)); // position depends on blueprint elements
    mobility.SetPositionAllocator(allocatorWiFi); 
    mobility.Install(wifiNodes);
  

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
with open(f"ns3_output_code/{blueprint_scenario_filename}.cc", 'w') as file:
    file.write(response.replace("```cpp","").replace("```",""))

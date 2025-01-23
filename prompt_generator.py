OUTPUT_LABEL="****** OUTPUT LLM ******"
def generatePromptSimEnv(scenario,prompt_version):
    with open("1-shot-samples/sample_real_scenario.py") as f:
        sample_real = f.read()
    with open(f"blueprints/{scenario}.json") as f:
        test_blueprint = f.read()
    with open("1-shot-samples/sample_ns3_code.cc") as f:
        sample_ns3_code = f.read()                              
    if prompt_version=="1.0-ns3": 
        fine_options="""
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

"""
        prompt = f"""
Given the following SAMPLE BLUEPRINT and the corresponding SAMPLE NS-3 code, use them as a sample to generate the NS3 code for the TEST BLUEPRINT. 
No, Explanation just python code and eventually comments

{fine_options}                    

SAMPLE NS3 CODE:

{sample_ns3_code}

TEST BLUEPRINT:

{test_blueprint}

NS3 CODE:

****** OUTPUT LLM ******
"""
    if prompt_version=="2.0-ns3": 
        prompt = f"""
Given the following TEST BLUEPRINT provide me the corresponding NS-3 code. Use the follwing SAMPLE NS-3 code and SAMPLE BLUEPRINT as reference.    
Not add description, just return cpp code.
TEST BLUEPRINT:

{test_blueprint}

SAMPLE NS3 CODE:

{sample_ns3_code}


NS3 CODE:

****** OUTPUT LLM ******

"""
    return prompt

def generatePromptRealEnv(scenario):
    with open("1-shot-samples/sample_real_scenario.py") as f:
        sample_real = f.read()
    with open(f"blueprints/{scenario}.json") as f:
        test_blueprint = f.read()
    with open("1-shot-samples/sample_ns3_code.cc") as f:
        sample_ns3_code = f.read()                              
    fine_options =f"""

It is mandatory that:
- for WiFi ACCESS POINT, you must use Fabric framework to manage connections;
- configure the AP through hostapd.conf and dnsmasq.conf;
- if no WiFi nodes or AP, do not use any code for that;
- ignore any gRPC code if there are no LoRa devices in TEST BLUEPRINT;
- configure all LoRa devices described in TEST BLUEPRINT;
- For each station add fixed IP  configuration for host=<wlan_MAC_ADDR>,<wlan_IP> in dnsmaq.conf;

Not add explaination, just python code.
"""

    prompt = f"""
Write a PYTHON CODE to set up the network from input json file TEST BLUEPRINT. Refer to the example code in SAMPLE PYTHON CODE.
{fine_options}

SAMPLE PYTHON CODE:

{sample_real}

TEST BLUEPRINT:

{test_blueprint}

PYTHON CODE:

{OUTPUT_LABEL}

"""
    return prompt

def generatePrompt(scenario,environment,prompt_version="2.0-ns3"):
    prompt_version="2.0-ns3"
    
    if environment in ["real","simulated"]:        
        if environment == "real":                        
            return generatePromptRealEnv(scenario)
        if environment == "simulated":
            return generatePromptSimEnv(scenario,prompt_version)
    else:         
        return -1
    
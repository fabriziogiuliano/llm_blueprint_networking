

import os
"""
==================================== SELECT MODEL!!!! ====================================
""" 
select_model="MISTRAL"
"""
==================================== SELECT MODEL!!!! ====================================
"""

from mistralai import Mistral
from dotenv import load_dotenv
load_dotenv()
from datetime import datetime

import google.generativeai as genai

def generatePrompt(sample_blueprint,sample_ns3_code,blueprint_scenario_filename):
    with open(f"blueprints/{blueprint_scenario_filename}_complete.json") as f:
            test_blueprint = f.read()

    prompt_old = f"""
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

    Generate only the code without "```cpp" quote it will be saved to a .cc file
    """
    prompt = f"""
    Given the following TEST BLUEPRINT provide me the corresponding NS-3 code. Use the follwing SAMPLE NS-3 code and SAMPLE BLUEPRINT as reference.    
    
            TEST BLUEPRINT:

            {test_blueprint}

            SAMPLE BLUEPRINT:

            {sample_blueprint}

            SAMPLE NS3 CODE:

            {sample_ns3_code}


            NS3 CODE:

            """
    return prompt
def runLLM(select_model,prompt):
    if select_model=="MISTRAL":
        model = "mistral-large-latest"
        client = Mistral(api_key=os.getenv('MISTRAL_API_KEY'))

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
        
    if select_model=="GEMINI":
        genai.configure(api_key=os.getenv("GEMINI_API_KEY"))
        #model = genai.GenerativeModel('gemini-2.0-flash-exp') #gemini-exp-1206
        model = genai.GenerativeModel('gemini-exp-1206')
        response = model.generate_content(prompt)
        response = response.text
    return response


blueprint_scenario_filenames=["smart_home","smart_agriculture","smart_city"]
model_names = ["MISTRAL","GEMINI"]
def main():

    for select_model in model_names:

        with open("sample_blueprint.json") as f:
                sample_blueprint = f.read()
        with open("sample_ns3_code.cc") as f:
                sample_ns3_code = f.read()


        for blueprint_scenario_filename in blueprint_scenario_filenames:
            prompt = generatePrompt(sample_blueprint,sample_ns3_code,blueprint_scenario_filename)
            
            output_filename = f"prompt_output/{blueprint_scenario_filename}_{select_model}.txt"
            with open(output_filename, 'w') as file:
                file.write(prompt)
            print(f"[{datetime.now()}][{select_model}][{blueprint_scenario_filename}] create prompt file to {output_filename}")    


            response=runLLM(select_model,prompt)

            output_filename = f"ns3_output_code/{blueprint_scenario_filename}_{select_model}.cc"
            with open(output_filename, 'w') as file:
                file.write(response.replace("```cpp","").replace("```",""))
            print(f"[{datetime.now()}][{select_model}][{blueprint_scenario_filename}] save response to {output_filename}")
    print(f"[{datetime.now()}]FINISHED!")
        
if __name__ == "__main__":
    main()


import os
from mistralai import Mistral
from dotenv import load_dotenv
load_dotenv()
model = "mistral-large-latest"
client = Mistral(api_key=os.getenv('MISTRAL_API_KEY'))

from datetime import datetime 
with open("sample_real_scenario_setup_lorawan.py") as f:
        sample_real = f.read()

#blueprint_scenario_filenames=["smart_home","smart_agriculture","smart_city"]
blueprint_scenario_filenames=["smart_home"]
for blueprint_scenario_filename in blueprint_scenario_filenames:
    print(f"OPEN: {blueprint_scenario_filename}...")
    with open(f"blueprints/{blueprint_scenario_filename}_complete.json") as f:
            test_blueprint = f.read()
    #with open(f"blueprints/{blueprint_scenario_filename}_network_configuration.json") as f:
    #        network_configuration = f.read()
            
    #- for WiFi Access Point and Stations, you must use Fabric framework to manage connections        
    #- for WiFi Stations, DHCP configiuration must match wlan_MAC with wlan_IP 
    fine_options =f"""
    
    It is mandatory that:
    - Istantiate Only the Gateways, Access point and Devices which are in the TEST BLUEPRINT;
    - if no WiFi nodes or AP, do not use any code for that;
    - if there are no LoRa devices, do not use any gRPC code and ignore any function definitions;
    - configure all devices described in TEST BLUEPRINT;
    - for WiFi ACCESS POINT, you must use Fabric framework to manage connections;
    - configure the AP through hostapd.conf and dnsmasq.conf;
    - For each station add fixed IP  configuration for host=<wlan_MAC_ADDR>,<wlan_IP> in dnsmaq.conf;
    
    """
    
    prompt = f"""
    Write a PYTHON CODE to set up the network described in TEST BLUEPRINT. Refer to the example code in SAMPLE PYTHON CODE.
    {fine_options}
    given output has just python code, no additional description or explaination 
     
    SAMPLE PYTHON CODE:

    {sample_real}

    TEST BLUEPRINT:

    {test_blueprint}
    
    PYTHON CODE:

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
    output_filename = f"real_output_code/{blueprint_scenario_filename}.py"
    with open(output_filename, 'w') as file:
        file.write(response.replace("```cpp","").replace("```",""))
    print(f"[{datetime.now()}]SAVED TO {output_filename}")
    print("------------------------------")
print("FINISHED!")
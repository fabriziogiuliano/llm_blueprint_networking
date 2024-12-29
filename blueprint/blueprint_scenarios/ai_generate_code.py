import os
from mistralai import Mistral
from dotenv import load_dotenv
from datetime import datetime
import google.generativeai as genai

import logging

logging.basicConfig(level=logging.INFO, format='[%(asctime)s][%(levelname)s][%(filename)s:%(lineno)d][%(funcName)s]%(message)s')

logger = logging.getLogger(__name__)

blueprint_scenario_filenames=["smart_home","smart_city","smart_agriculture"]
blueprint_scenario_filenames=["smart_city"]

#model_names = ["gemini-exp-1206"]
#model_names = ["llama-3.3"]
#model_names = ["gemini-exp-1206","gemini-2.0-flash-exp","mistral-large-latest"]
#blueprint_scenario_filenames=["smart_home","smart_agriculture","smart_city"]
#model_names = ["gemini-exp-1206","gemini-2.0-flash-exp","mistral-large-latest"]

model_names = ["mistral-large-latest"]
environments=["ns3"]

load_dotenv()

def generatePrompt(sample_blueprint,sample_code,test_blueprint,prompt_version="2.0-simulation"):
    
    
    prompt=""
    """
    Questa è la prima versione di prompt utilizzato, dove:
    - fine_options è del testo che stato aggiunto in alcune prove poiché non funzionava correttamente la generazione del codice
    """
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

        Generate only the code without "```cpp" quote it will be saved to a .cc file
        """
        
        prompt = f"""
        Given the following SAMPLE BLUEPRINT and the corresponding SAMPLE NS-3 code, use them as a sample to generate the NS3 code for the TEST BLUEPRINT. 
        
                {fine_options}

                SAMPLE BLUEPRINT:

                {sample_blueprint}

                SAMPLE NS3 CODE:

                {sample_code}

                TEST BLUEPRINT:

                {test_blueprint}

                NS3 CODE:

                """

    """
    in questa versione è stato modificato il prompt per ottenere codice migliore, 
    l'effetto di Lost in the middle è stato gestito mettendo il sample code in fondo.
    """        
    if prompt_version=="2.0-ns3":
        prompt = f"""
        Given the following TEST BLUEPRINT provide me the corresponding NS-3 code. Use the follwing SAMPLE NS-3 code and SAMPLE BLUEPRINT as reference.    
        Not add description, just return cpp code.
                TEST BLUEPRINT:

                {test_blueprint}

                SAMPLE BLUEPRINT:

                {sample_blueprint}

                SAMPLE NS3 CODE:

                {sample_code}


                NS3 CODE:

                """
    if prompt_version=="2.0-real":
        fine_options =f"""
        
        It is mandatory that:
        - Istantiate Only the Gateways, Access point and Devices which are in the TEST BLUEPRINT;
        - if no WiFi nodes or AP, do not use any code for that;
        - if there are no LoRa devices, do not use any gRPC code and ignore any function definitions;
        - configure all devices described in TEST BLUEPRINT;
        - for WiFi Access Point, you must use Fabric framework to manage connections;
        - configure the AP through hostapd.conf and dnsmasq.conf;
        - For each station add fixed IP  configuration for host=<wlan_MAC_ADDR>,<wlan_IP> in dnsmaq.conf;
        - for WiFi Stations, fabric connection must be provided using ETH_IP;
        
        """
        
        prompt = f"""
        Write a PYTHON CODE to set up the network described in TEST BLUEPRINT. Refer to the example code in SAMPLE PYTHON CODE.
        {fine_options}
        given output has just python code, no additional description or explaination 
        
        SAMPLE PYTHON CODE:

        {sample_code}

        TEST BLUEPRINT:

        {test_blueprint}    
        
        PYTHON CODE:

        """
    return prompt


def runLLM(select_model,prompt):
    response=None
    if select_model=="llama-3.3":
        from ollama import Client
        client = Client(
        host='http://quasar.local:11434',
        headers={'x-some-header': 'some-value'}
        )
        response = client.chat(model=select_model, messages=[
        {
            'role': 'user',
            'content': prompt,
        },
        ])
    
    if select_model=="mistral-large-latest":        
        client = Mistral(api_key=os.getenv('MISTRAL_API_KEY'))
        chat_response = client.chat.complete(
            model = select_model,
            temperature=0,
            messages = [
                {
                    "role": "user",
                    "content": prompt,
                },
            ]
        )

        response = chat_response.choices[0].message.content
        
    if select_model=="gemini-exp-1206" or select_model=="gemini-2.0-flash-exp":
        genai.configure(api_key=os.getenv("GEMINI_API_KEY"))        
        model = genai.GenerativeModel(select_model)
        response = model.generate_content(prompt)
        response = response.text
    
    if response==None:
        logger.info(f"ERROR!! {select_model} not found, return null")
    return response




def main():
    for environment in environments:
        for blueprint_scenario_filename in blueprint_scenario_filenames:
            with open(f"blueprints/{blueprint_scenario_filename}_complete.json") as f:
                        test_blueprint = f.read()
            for select_model in model_names:
                with open("sample_blueprint.json") as f:
                        sample_blueprint = f.read()
                        
                if environment=="ns3":
                    with open("sample_ns3_code.cc") as f:
                            sample_ns3_code = f.read()    
                            
                    prompt = generatePrompt(sample_blueprint,sample_ns3_code,test_blueprint,prompt_version=f"2.0-{environment}")
                
                    if not os.path.isdir("prompt_output"):                                
                        os.mkdir("prompt_output")
                    
                    output_filename = f"prompt_output/{blueprint_scenario_filename}_{select_model}_{environment}.txt"
                    
                    with open(output_filename, 'w') as file:
                        file.write(prompt)
                    logger.info(f"[{select_model}][{blueprint_scenario_filename}][{environment}] create prompt file to {output_filename}")    
                                        
                    response=runLLM(select_model,prompt)
                    
                    if not os.path.isdir("ns3_output_code"):                                
                        os.mkdir("ns3_output_code")
                    output_filename = f"ns3_output_code/{blueprint_scenario_filename}_{select_model}.cc"
                    with open(output_filename, 'w') as file:
                        file.write(response.replace("```cpp","").replace("```",""))
                    logger.info(f"[{datetime.now()}][{select_model}][{blueprint_scenario_filename}][{environment}] create prompt file to {output_filename}")    
                    logger.info("------------------------------")

                if environment=="real":
                    with open("sample_real_scenario_setup_lorawan.py") as f:
                        sample_real = f.read()
                    prompt = generatePrompt(sample_blueprint,sample_real,test_blueprint,prompt_version=f"2.0-{environment}")

                    if not os.path.isdir("prompt_output"):                                
                        os.mkdir("prompt_output")

                    output_filename = f"prompt_output/{blueprint_scenario_filename}_{select_model}_{environment}.txt"
                    with open(output_filename, 'w') as file:
                        file.write(prompt)
                    logger.info(f"[{datetime.now()}][{select_model}][{blueprint_scenario_filename}][{environment}] create prompt file to {output_filename}")    


                    response=runLLM(select_model,prompt)
                    if not os.path.isdir("real_output_code"):                                
                        os.mkdir("real_output_code")
                    output_filename = f"real_output_code/{blueprint_scenario_filename}.py"
                    with open(output_filename, 'w') as file:
                        file.write(response)
                    logger.info(f"[{datetime.now()}][{select_model}][{blueprint_scenario_filename}][{environment}] save response to {output_filename}")
                    logger.info("------------------------------")
    logger.info(f"[{datetime.now()}]FINISHED!")
        
if __name__ == "__main__":
    main()
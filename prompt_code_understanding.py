def generatePromptUnderstanding(scenario,environment,generated_code_filename):
    if environment in ["real","simulated"]:        
        if environment == "real":                        
            return generatePromptRealEnvUnderstanding(scenario,generated_code_filename)
        if environment == "simulated":
            return generatePromptSimEnvUnderstanding(scenario,generated_code_filename)
    else:         
        return -1

"""
=======================================
UNDERSTANDING FOR SIMULATED ENVIRONMENT
=======================================

Feature n.1 LoRa devices in "JSON BLUEPRINT" MUST match with "NS-3 SOURCE-CODE", if "JSON BLUEPRINT" contains LoRa devices, but "NS-3 SOURCE-CODE" no, this meanse unmatch!
Feature n.2 WiFi devices in "JSON BLUEPRINT" MUST match with "NS-3 SOURCE-CODE", if "JSON BLUEPRINT" contains WiFi devices, but "NS-3 SOURCE-CODE" no, this meanse unmatch!
Feature n.3 Location assignment (in coordinates) in "JSON BLUEPRINT"  MUST match with "NS-3 SOURCE-CODE"
"""
def generatePromptSimEnvUnderstanding(scenario, generated_code_filename):    
    with open(f"blueprints/{scenario}.json") as f:
        test_blueprint = f.read()    
    with open(generated_code_filename) as f:
        ns3_llm_code = f.read()

    prompt=f"""

Given LLM generated "NS-3 SOURCE CODE" from "JSON BLUEPRINT" check features. If features not match the score is LOW:

# Feature n.1 
    Count many LoRa Devices are defined in "JSON BLUEPRINT" and write in the motivation;
    LoRa COUNTS MUST MATCH. If "JSON BLUEPRINT" does not explicitly declare LoRa devices and "NS-3 SOURCE-CODE" use LoRa, consider it UNMATCH.
    
# Feature n.2 
    Count WiFi Devices are defined in "JSON BLUEPRINT" and write in the motivation;
    wiFi COUNTS MUST MATCH. If "JSON BLUEPRINT" does not explicitly declare WiFi devices and "NS-3 SOURCE-CODE" use WiFi, consider it UNMATCH.
    
# Feature n.3 
    Location coordinates for devices in the "NS-3 SOURCE-CODE" MUST perfectly match the coordinates of the corresponding devices in the "JSON BLUEPRINT". 
    Any discrepancy in coordinates or the absence of assigned coordinates in the "NS-3 SOURCE-CODE" for devices with coordinates in the "JSON BLUEPRINT" is considered a mismatch (unmatch).
    
# Feature n.4 
    Traffic duration and type in "JSON BLUEPRINT" MUST match with "NS-3 SOURCE-CODE"

for all features give a score between 0 and 5, where score 0 means totally unmatch, score 5 means fully match.

Note: if cpp code does not provide functions implementation and main, all features MUST be considered equal to 0. 

generate just a syntetic output as a CSV table with following columns, with semicolon separator:

SCENARIO (type string); ID_FEATURE(Number); SHORT_CLEAR_MOTIVATION (text); SCORE (Number). Note the field names must be in lowercase
For all rows Scenario is qual to {scenario}

**** "NS-3 SOURCE-CODE" ****
{ns3_llm_code}


**** "JSON BLUEPRINT" ****
{test_blueprint}

****** OUTPUT LLM ******

"""
    
    return prompt






"""
=======================================
UNDERSTANDING FOR REAL ENVIRONMENT
=======================================
"""

def generatePromptRealEnvUnderstanding(scenario,generated_code_filename):
    with open(f"blueprints/{scenario}.json") as f:
        test_blueprint = f.read()    
    with open(generated_code_filename) as f:
        real_llm_code = f.read()
    
    

    prompt=f"""
Given LLM generated "PYTHON SOURCE-CODE" from "JSON BLUEPRINT" check following features:

#Feature n.1 
    If the "JSON BLUEPRINT" includes a description of LoRa devices, then the "PYTHON SOURCE-CODE" MUST implement functions to set up LoRa. If not, consider it UNMATCH.
    If the "JSON BLUEPRINT" includes a description of LoRa devices, setup MUST be provided using gRPC function calls, if not considered it UNMATCH. 
    If "PYTHON SOURCE-CODE" wrongly use LoRa configuration functions with WiFi parameters, consider it UNMATCH
    if "PYTHON SOURCE-CODE" code does not provide any python correct functions or commands, this feature must be considered as TOTALLY UNMATCHED

#Feature n.2 
    If the "JSON BLUEPRINT" includes a description of WiFi devices, then the "PYTHON SOURCE-CODE" MUST implement functions to set up WiFi. 
    If the "JSON BLUEPRINT" includes a description of WiFi devices, setup MUST be provided using Fabric Framework, if not considered it UNMATCH.
    In WiFi setup, simple file writing is consider UNMATCH
    if WiFi "JSON BLUEPRINT" exists but  "PYTHON SOURCE-CODE" contains WiFi functions, consider UNMATCH
    if "PYTHON SOURCE-CODE" code does not provide any python correct functions or commands, this feature must be considered as TOTALLY UNMATCHED

#Feature n.3
    "JSON BLUEPRINT" fields MUST match with dictionary field in "PYTHON SOURCE-CODE", if not, consider it UNMATCH.
    if "PYTHON SOURCE-CODE" code does not provide any python correct functions or commands, this feature must be considered as TOTALLY UNMATCHED

for all features give a score between 0 and 5, where score 0 means totally unmatch, score 5 means fully match.



generate just a syntetic output as a CSV table with following columns, with semicolon separator:

SCENARIO (type string); ID_FEATURE(Number); SHORT_CLEAR_MOTIVATION (text); SCORE (Number). Note the field names must be in lowercase
For all rows Scenario is qual to {scenario}

**** "PYTHON SOURCE-CODE" **** 
{real_llm_code}


**** "JSON BLUEPRINT" **** 
{test_blueprint}

****** OUTPUT LLM ******

"""
    
    return prompt

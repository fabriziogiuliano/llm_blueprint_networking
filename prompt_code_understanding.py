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

# Feature n.0
    "NS-3 SOURCE" CODE is C++/NS-3. If not, consider it UNMATCH and set score=0 to all other features.
# Feature n.1 
    Feature n.0 MUST MATCH AND LoRa COUNTS MUST MATCH. If "JSON BLUEPRINT" does not explicitly declare LoRa devices and "NS-3 SOURCE-CODE" use LoRa, consider it UNMATCH (score=0).
    Count LoRa Devices are defined in "JSON BLUEPRINT" and write in the motivation;
    
    
# Feature n.2 
    Feature n.0 MUST MATCH if not set score = 0 AND wiFi COUNTS MUST MATCH. If "JSON BLUEPRINT" does not explicitly declare WiFi devices and "NS-3 SOURCE-CODE" use WiFi, consider it UNMATCH (score=0).
    
# Feature n.3 
    Feature n.0 MUST MATCH if not set score = 0 AND Location coordinates for devices in the "NS-3 SOURCE-CODE" MUST perfectly match the coordinates of the corresponding devices in the "JSON BLUEPRINT". 
    All "NS-3 SOURCE CODE" must be a valid c++ NS3 code, if not, consider ALL FOLLOWING FEATURES as score=0 and finish the analysis;
    Any discrepancy in coordinates or the absence of assigned coordinates in the "NS-3 SOURCE-CODE" for devices with coordinates in the "JSON BLUEPRINT" is considered a mismatch (score=0).
    
# Feature n.4 
    Feature n.0 MUST MATCH if not set score = 0 AND All "NS-3 SOURCE CODE" must be a valid c++ NS3 code, if not, consider ALL FOLLOWING FEATURES as score=0 and finish the analysis;
    Traffic duration and type in "JSON BLUEPRINT" MUST match with "NS-3 SOURCE-CODE". Any discrepancy in the "NS-3 SOURCE-CODE" for devices with coordinates in the "JSON BLUEPRINT" is considered a mismatch (score=0).


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

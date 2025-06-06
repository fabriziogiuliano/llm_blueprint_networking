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

You are a software engineer in a company that develops IoT solutions. You have been assigned to review the code generated by the LLM tool for the scenario {scenario}. The code is intended to be deployed on a real device. The code is written in Python and is intended to be run on a Raspberry Pi. The code is intended to interact with LoRa and WiFi devices. The code is intended to be run on a Raspberry Pi
You MUST Generate just a syntetic output as a CSV table with following columns, with semicolon separator:

    Column1 : SCENARIO (String); For all rows Scenario is qual to {scenario}
    Column2 : ID_FEATURE(Integer); 
    Column3 : SHORT_CLEAR_MOTIVATION (String); 
    Column4 : SUGGEST_CORRECTION (String); Fill this field with a short suggestion on how to correct the issue;
    Column5 : SCORE (Integer). 

Note: the field names must be in lowercase. 

Given LLM generated "NS-3 SOURCE CODE" from "JSON BLUEPRINT" check features. If features not match the score is LOW:

# Feature n.0
    "NS-3 SOURCE" CODE is C++/NS-3 with a main function. If not, consider it UNMATCH and set score=0 to all other features.
    
# Feature n.1 
    LoRa COUNTS MUST MATCH. If "JSON BLUEPRINT" does not explicitly declare LoRa devices and "NS-3 SOURCE-CODE" use LoRa, consider it UNMATCH (score=0);
    Count many LoRa Devices are defined in "JSON BLUEPRINT" and write in the motivation;
    All "NS-3 SOURCE CODE" must be a valid c++ NS3 code, if not, consider ALL FOLLOWING FEATURES as score=0 and finish the analysis;
    if Feature n.0 score == 0 set this feature score=0.
    
# Feature n.2 
    The number of WiFi devices MUST MATCH with "JSON BLUEPRINT", If not, consider it UNMATCH (score=0);
    If "JSON BLUEPRINT" does not explicitly declare WiFi devices and "NS-3 SOURCE-CODE" use WiFi, consider it UNMATCH (score=0);
    Count WiFi Devices are defined in "JSON BLUEPRINT" and write in the motivation;
    All "NS-3 SOURCE CODE" must be a valid c++ NS3 code, if not, consider ALL FOLLOWING FEATURES as score=0 and finish the analysis;
    if Feature n.0 score == 0 set this feature score=0.
    
# Feature n.3 
    Location coordinates for devices in the "NS-3 SOURCE-CODE" MUST MATCH the coordinates of the corresponding devices in the "JSON BLUEPRINT". If not, consider it UNMATCH (score=0); 
    All "NS-3 SOURCE CODE" must be a valid c++ NS3 code, if not, consider ALL FOLLOWING FEATURES as score=0 and finish the analysis;
    Any discrepancy in coordinates or the absence of assigned coordinates in the "NS-3 SOURCE-CODE" for devices with coordinates in the "JSON BLUEPRINT" is considered a mismatch (score=0);
    if Feature n.0 score == 0 set this feature score=0.
    
# Feature n.4 
    All "NS-3 SOURCE CODE" must be a valid c++ NS3 code, if not, consider ALL FOLLOWING FEATURES as score=0 and finish the analysis;
    Traffic duration and type in "JSON BLUEPRINT" MUST match with "NS-3 SOURCE-CODE". Any discrepancy in the "NS-3 SOURCE-CODE" for devices with coordinates in the "JSON BLUEPRINT" is considered a mismatch (score=0);
    if Feature n.0 score == 0 set this feature score=0.


for all features give a score between 0 and 5, where score 0 means totally unmatch, score 5 means fully match.

Note: if cpp code does not provide functions implementation and main, all features MUST be considered equal to 0. 

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
You are a software engineer in a company that develops IoT solutions. You have been assigned to review the code generated by the LLM tool for the scenario {scenario}. The code is intended to be deployed on a real device. The code is written in Python and is intended to be run on a Raspberry Pi. The code is intended to interact with LoRa and WiFi devices. The code is intended to be run on a Raspberry Pi
You MUST Generate just a syntetic output as a CSV table with following columns, with semicolon separator:

    Column1 : SCENARIO (String); For all rows Scenario is qual to {scenario}
    Column2 : ID_FEATURE(Integer); 
    Column3 : SHORT_CLEAR_MOTIVATION (String); 
    Column4 : SUGGEST_CORRECTION (String); Fill this field with a short suggestion on how to correct the issue;
    Column5 : SCORE (Integer). 

Note: the field names must be in lowercase. 



Given LLM generated "PYTHON SOURCE-CODE" from "JSON BLUEPRINT" check following features:

# Feature n.0 
    The "PYTHON SOURCE-CODE" MUST NOT have syntax errors. If it does, highlight the error and consider ALL FOLLOWING FEATURES as score=0 and finish the analysis.
    The "PYTHON SOURCE-CODE" must define functions. If not, consider ALL FOLLOWING FEATURES as score=0 and finish the analysis.
    The "PYTHON SOURCE-CODE" must define function calls and variable declarations. If not, consider ALL FOLLOWING FEATURES as score=0 and finish the analysis.    
    The "PYTHON SOURCE-CODE" must have more than a few lines of code. If not, consider ALL FOLLOWING FEATURES as score=0 and finish the analysis.
    The "PYTHON SOURCE-CODE" should contain function and instruction execution. If not, consider ALL FOLLOWING FEATURES as score=0 and finish the analysis.
    

# Feature n.1 
    The "PYTHON SOURCE-CODE" must be valid Python code without syntax errors. If not, highlight the error and consider ALL FOLLOWING FEATURES as score=0 and finish the analysis.
    If the "JSON BLUEPRINT" doest not require LoRA devices, then the "PYTHON SOURCE-CODE" MUST NOT implement LoRa functions. If not, set score feature as score=0.
    If the "JSON BLUEPRINT" includes a description of LoRa devices, then the "PYTHON SOURCE-CODE" MUST implement functions to set up LoRa. If not, consider it UNMATCH.
    If the "JSON BLUEPRINT" includes a description of LoRa devices, the setup MUST be provided using gRPC function calls. If not, consider it UNMATCH. 
    If the "PYTHON SOURCE-CODE" wrongly uses LoRa configuration functions with WiFi parameters, consider it UNMATCH.
    If the "PYTHON SOURCE-CODE" does not provide any correct Python functions or commands, this feature must be considered as TOTALLY UNMATCHED.

# Feature n.2 
    The "PYTHON SOURCE-CODE" must be valid Python code without syntax errors. If not, highlight the error and consider ALL FOLLOWING FEATURES as score=0 and finish the analysis.
    If the "JSON BLUEPRINT" doest not require WiFi devices, then the "PYTHON SOURCE-CODE" MUST NOT implement WiFi functions. If not, set score feature as score=0.
    If the "JSON BLUEPRINT" includes a description of WiFi devices, the setup MUST be provided using the Fabric Framework. If not, consider this feature as score=0.
    If the "JSON BLUEPRINT" includes a description of WiFi devices, then the "PYTHON SOURCE-CODE" MUST implement functions to set up WiFi. 
    If the "JSON BLUEPRINT" includes WiFi but the "PYTHON SOURCE-CODE" does not contain WiFi functions, consider this feature as score=0.
    If the "PYTHON SOURCE-CODE" does not provide any correct Python functions or commands, consider this feature as score=0.

# Feature n.3
    The "PYTHON SOURCE-CODE" must be valid Python code without syntax errors. If not, highlight the error and consider ALL FOLLOWING FEATURES as score=0 and finish the analysis.
    If the "PYTHON SOURCE-CODE" does not provide any correct Python functions or commands, consider this feature as score=0.

For all features, give a score between 0 and 5, where score 0 means totally unmatch, and score 5 means fully match.

All features must be filled in the CSV table. 

**** "PYTHON SOURCE-CODE" **** 
{real_llm_code}


**** "JSON BLUEPRINT" **** 
{test_blueprint}

****** OUTPUT LLM ******

"""
    
    return prompt

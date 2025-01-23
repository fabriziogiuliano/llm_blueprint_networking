def generatePromptUnderstanding(scenario,environment,generated_code_filename):
    if environment in ["real","simulated"]:        
        if environment == "real":                        
            return generatePromptRealEnvUnderstanding(scenario,generated_code_filename)
        if environment == "simulated":
            return generatePromptSimEnvUnderstanding(scenario,generated_code_filename)
    else:         
        return -1

def generatePromptRealEnvUnderstanding(scenario,generated_code_filename):
    with open(f"blueprints/{scenario}.json") as f:
        test_blueprint = f.read()    
    with open(generated_code_filename) as f:
        real_llm_code = f.read()
    
    

    prompt=f"""
Given following LLM generated NS-3 SOURCE CODE from JSON BLUEPRINT check if PYTHON SOURCE CODE behavior match with JSON BLUEPRINT, in particular check following features:

Feature n.1 LoRa number of devices in JSON BLUEPRINT MUST match with NS-3 code, if JSON contains LoRa devices, but NS-3 no, this meanse unmatch!
Feature n.2 WiFi number of devices in JSON BLUEPRINT MUST match with NS-3 code, if JSON contains WiFi devices, but NS-3 no, this meanse unmatch!
Feature n.3 Location assignment (in coordinates) in JSON BLUEPRINT  MUST match with NS-3
Feature n.4 Traffic duration and type in JSON BLUEPRINT MUST match with NS-3 code

for all features give a score between 0 and 5, where score 0 means totally unmatch, score 5 means fully match.

generate just a syntetic output as a CSV table with following columns, with semicolon separator:

SCENARIO (type string); ID_FEATURE( format feature_n.X); SHORT_CLEAR_MOTIVATION (text); SCORE (Number). Note the field names must be in lowercase
For all rows Scenario is qual to {scenario}

PYTHON SOURCE CODE:
{real_llm_code}


JSON BLUEPRINT:
{test_blueprint}

****** OUTPUT LLM ******

"""
    
    return prompt

def generatePromptSimEnvUnderstanding(scenario, generated_code_filename):    
    with open(f"blueprints/{scenario}.json") as f:
        test_blueprint = f.read()    
    with open(generated_code_filename) as f:
        ns3_llm_code = f.read()

    prompt=f"""
Given following LLM generated NS-3 SOURCE CODE from JSON BLUEPRINT check if NS-3 SOURCE CODE behavior match with JSON BLUEPRINT, in particular check following features:

Feature n.1 LoRa number of devices in JSON BLUEPRINT MUST match with NS-3 code, if JSON contains LoRa devices, but NS-3 no, this meanse unmatch!
Feature n.2 WiFi number of devices in JSON BLUEPRINT MUST match with NS-3 code, if JSON contains WiFi devices, but NS-3 no, this meanse unmatch!
Feature n.3 Location assignment (in coordinates) in JSON BLUEPRINT  MUST match with NS-3
Feature n.4 Traffic duration and type in JSON BLUEPRINT MUST match with NS-3 code

for all features give a score between 0 and 5, where score 0 means totally unmatch, score 5 means fully match.

generate just a syntetic output as a CSV table with following columns, with semicolon separator:

SCENARIO (type string); ID_FEATURE( format feature_n.X); SHORT_CLEAR_MOTIVATION (text); SCORE (Number). Note the field names must be in lowercase
For all rows Scenario is qual to {scenario}

NS-3 SOURCE CODE:
{ns3_llm_code}


JSON BLUEPRINT:
{test_blueprint}

****** OUTPUT LLM ******

"""
    
    return prompt
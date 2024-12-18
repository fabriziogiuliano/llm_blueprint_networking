

import os
from mistralai import Mistral
from dotenv import load_dotenv
from datetime import datetime 

with open("sample_blueprint.json") as f:
        sample_blueprint = f.read()
with open("sample_ns3_code.cc") as f:
        sample_ns3_code = f.read()

#blueprint_scenario_filenames=["smart_home","smart_home_mixed","smart_agriculture","smart_city"]
blueprint_scenario_filenames=["smart_agriculture_complete"]
for blueprint_scenario_filename in blueprint_scenario_filenames:
    print(f"OPEN: {blueprint_scenario_filename}...")
    with open(f"blueprints/{blueprint_scenario_filename}.json") as f:
            test_blueprint = f.read()

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
    print(prompt)
    output_filename = f"prompt_output/{blueprint_scenario_filename}.txt"
    with open(output_filename, 'w') as file:
        file.write(prompt)
    print(f"[{datetime.now()}]SAVED TO {output_filename}")    
    print("------------------------------")
print("FINISHED!")
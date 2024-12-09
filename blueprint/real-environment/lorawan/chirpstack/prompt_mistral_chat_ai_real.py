

import os
from mistralai import Mistral
from dotenv import load_dotenv
load_dotenv()
model = "mistral-large-latest"
client = Mistral(api_key=os.getenv('API_KEY'))


with open("sample_lorawan_setup.py") as f:
        sample_real = f.read()

#blueprint_scenario_filenames=["smart_home","smart_home_mixed","smart_agriculture","smart_city"]
blueprint_scenario_filenames=["smart_city"]
for blueprint_scenario_filename in blueprint_scenario_filenames:
    print(f"OPEN: {blueprint_scenario_filename}...")
    with open(f"blueprints/{blueprint_scenario_filename}.json") as f:
            test_blueprint = f.read()

    prompt = f"""
    Given the following SAMPLE PYTHON CODE, use them as a sample to generate the Python code for the TEST BLUEPRINT. 
    
    If WiFi nodes are indicate in TEST BLUEPRINT, use fabric to setup AP and STA devices

            SAMPLE PYTHON CODE:

            {sample_real}

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
    output_filename = f"real_output_code/{blueprint_scenario_filename}.py"
    with open(output_filename, 'w') as file:
        file.write(response.replace("```cpp","").replace("```",""))
    print(f"SAVED TO {output_filename}")
    print("------------------------------")
print("FINISHED!")
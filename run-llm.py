import torch
import os
from transformers import AutoTokenizer, AutoModelForCausalLM
from tqdm import tqdm
from mistralai import Mistral
from dotenv import load_dotenv
from datetime import datetime
import google.generativeai as genai
from google.api_core.exceptions import ResourceExhausted
from pathlib import Path
import logging
import pandas as pd
import io
import time
import random
import traceback
logging.basicConfig(level=logging.INFO, format='[%(asctime)s][%(levelname)s][%(filename)s:%(lineno)d][%(funcName)s]%(message)s')

logger = logging.getLogger(__name__)

from prompt_generator import *

from prompt_code_understanding import *


device="cuda"

def runLLMSmall(model_name,prompt):
    tokenizer = AutoTokenizer.from_pretrained(model_name)
    model = AutoModelForCausalLM.from_pretrained(model_name, torch_dtype=torch.float16).to(device)

    input_ids = tokenizer(prompt, return_tensors="pt").to(device)
    logger.info("run generate... ")
    outputs = model.generate(**input_ids,max_new_tokens=5500, pad_token_id = tokenizer.eos_token_id)
    output = tokenizer.decode(outputs[0])
    return output


def runLLMLarge(model_name,prompt):
    load_dotenv()
    response=None
    
    if model_name=="mistral-large-latest" or model_name=="mistral-large-2411" or model_name=="codestral-2501":  
        api_key=os.getenv('MISTRAL_API_KEY')
        client = Mistral(api_key=api_key)
        chat_response = client.chat.complete(
            model = model_name,
            temperature=0,
            messages = [
                {
                    "role": "user",
                    "content": prompt,
                },
            ]
        )

        response = chat_response.choices[0].message.content
        
    if model_name=="gemini-exp-1206" or model_name=="gemini-2.0-flash-exp":
        
        retries = 0
        max_retries = 5
        base_delay = 1  # seconds
        genai.configure(api_key=os.getenv("GEMINI_API_KEY"))        
        model = genai.GenerativeModel(model_name)
        while retries < max_retries:
            try:
                response = model.generate_content(prompt)
                response = response.text
                return response
            except ResourceExhausted as e:
                retries += 1
                delay = base_delay * (2 ** retries) + random.uniform(0, base_delay)
                print(f"Resource exhausted, retrying in {delay:.2f} seconds... ({retries}/{max_retries})")
                time.sleep(delay)
            except Exception as e: # Handle other exceptions if needed
                raise e
        raise Exception("Failed after multiple retries")
        
    
    if response==None:
        logger.info(f"ERROR!! {model_name} not found, return null")
    return response

def formatOutput(input):
    #Format output remove garbage strings
    output = input.split(OUTPUT_LABEL, 1)[-1].lstrip().rstrip()
    
    garbage_string_list=["</s>","```python","```<eos>","<eos>","```cpp","```"]
    
    for s in garbage_string_list:        
        output= output.replace(s,"")
    
    return output

def runLLM(mode_name,model_size,prompt):
    if model_size=="large":
        return formatOutput(runLLMLarge(mode_name,prompt))
   
    if model_size=="small":
        return formatOutput(runLLMSmall(mode_name,prompt))
    logger.info("Nothing to do, exit")
    
    return None

def run_experiment(blueprint_id, scenario,environment,model_name,model_size,prompt_version,model_name_understanding=None,experiment_label="generate_code"):
    curr_scenario = f"{blueprint_id:03}-{scenario}"
    logger.info("-------------------------------------")
    logger.info(f"MODEL: {model_name}")
    logger.info(f"scenario: {curr_scenario}")
    logger.info(f"environments: {environment}")
    logger.info("-------------------------------------")

    model_name_str=model_name.replace("/","_")


    if experiment_label == "generate_code":
        #GENERATE PROMPT
        prompt = generatePrompt(curr_scenario ,environment,prompt_version)    
        output = runLLM(model_name,model_size,prompt)

        #Save to file

        if environment=="simulated":
            src_filename=f"{curr_scenario}-{environment}-{prompt_version}-{model_name_str}-{model_size}"
            Path(f"output/{environment}/{src_filename}").mkdir(parents=True, exist_ok=True)
            output_filename = f"output/{environment}/{src_filename}/{src_filename}.cc"
        if environment=="real":
            Path(f"output/{environment}").mkdir(parents=True, exist_ok=True)
            output_filename = f"output/{environment}/{curr_scenario}-{environment}-{model_name_str}-{model_size}.py"
        
        logger.info(f"save to {output_filename}")
        with open(output_filename, "w") as file:
                file.write(output)
        return 0
    if experiment_label == "understanding":
        src_filename=f"{curr_scenario}-{environment}-{prompt_version}-{model_name_str}-{model_size}"
        generated_code_filename = f"output/{environment}/{src_filename}/{src_filename}.cc"
        prompt_understanding = generatePromptUnderstanding(scenario=curr_scenario, environment=environment, generated_code_filename=generated_code_filename)
                
        
        output = runLLM(model_name_understanding["name"],model_name_understanding["size"],prompt_understanding)
        output=output.replace("csv","")
        print(output)
        try:
            df = pd.read_csv(io.StringIO(output),sep=";")        
            return df
        except:
            pass
        return 0
def main():
    
    #Choose Model (Small)
    #model_name = "mistralai/Mistral-7B-Instruct-v0.3"
    #model_name ="mistralai/Ministral-8B-Instruct-2410" #Genera Funzioni vuote
    #model_name ="mistralai/Mistral-Large-Instruct-2407" #Too large
    #model_name ="mistralai/Mistral-Small-Instruct-2409"
    #model_name ="mistralai/Codestral-22B-v0.1"
    #model_name="mistralai/Mistral-Nemo-Instruct-2407" # Non genera codice Python per Fabric WiFi
    
    
    model_names_all=[
            #{"name":"codestral-2501","size":"large"}, #{"name":"mistralai/Mamba-Codestral-7B-v0.1","size":"small"},
            #{"name":"google/codegemma-7b-it","size":"small"},
            #{"name":"google/codegemma-7b-it","size":"small"},
            #{"name":"codellama/CodeLlama-7b-Instruct-hf","size":"small"},
            #{"name":"mistral-large-2411","size":"large"},
            {"name":"gemini-exp-1206","size":"large"},
            {"name":"gemini-2.0-flash-exp","size":"large"}
        ]
    model_name_understanding={"name":"gemini-2.0-flash-exp","size":"large"}
    print(model_name_understanding)
    model_names=model_names_all#[model_names_all[5]]
    scenarios = ["smart_agriculture","smart_city","smart_home"]
    environments = ["simulated","real"]    
    
    
    prompt_version="2.0-ns3"
    
    scenarios = ["smart_home"]
    environments = ["simulated"]

    #experiment_label="understanding"
    experiment_label="generate_code"

    df_validator = pd.DataFrame()
    for environment in environments:                
        for scenario in scenarios:                    
            for model_name in model_names:                                
                for blueprint_id in range(1,4):
                    try:
                        ret = run_experiment(
                            blueprint_id=blueprint_id,
                            scenario=scenario,
                            environment=environment,
                            model_name=model_name["name"],
                            model_size=model_name["size"],
                            prompt_version=prompt_version,
                            model_name_understanding=model_name_understanding,
                            experiment_label=experiment_label
                        ) 
                        if isinstance(ret, pd.DataFrame):
                            ret["model_name"]=model_name["name"]
                            ret["model_size"]=model_name["size"]
                            ret["environment"]=environment
                            ret["model_name_understanding"]=model_name_understanding["name"]
                            ret["model_size_understanding"]=model_name_understanding["size"]
                            df_validator = pd.concat([df_validator, ret], ignore_index=True)
                    except Exception :
                        traceback.print_exc()
                logging.info("SLEEP...")
                time.sleep(10)
                        
    print(df_validator)
    Path(f"output/validator/").mkdir(parents=True, exist_ok=True)
    df_validator.to_pickle("output/validator/results.pkl")
if __name__ == "__main__":
        main()
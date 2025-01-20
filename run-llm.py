import torch
import os
from transformers import AutoTokenizer, AutoModelForCausalLM
from tqdm import tqdm
from mistralai import Mistral
from dotenv import load_dotenv
from datetime import datetime
import google.generativeai as genai
from pathlib import Path
from prompt_generator import *
import logging

logging.basicConfig(level=logging.INFO, format='[%(asctime)s][%(levelname)s][%(filename)s:%(lineno)d][%(funcName)s]%(message)s')

logger = logging.getLogger(__name__)

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
    
    if model_name=="mistral-large-latest":  
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
        genai.configure(api_key=os.getenv("GEMINI_API_KEY"))        
        model = genai.GenerativeModel(model_name)
        response = model.generate_content(prompt)
        response = response.text
    
    if response==None:
        logger.info(f"ERROR!! {model_name} not found, return null")
    return response

def formatOutput(input):
    #Format output remove garbage strings
    output = input.split(OUTPUT_LABEL, 1)[-1].lstrip().rstrip()
    
    garbage_string_list=["</s>","```python","```<eos>","<eos>","```"]
    
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

def run_experiment(scenario,environment,model_name,model_size):
    
    logger.info("-------------------------------------")
    logger.info(f"MODEL: {model_name}")
    logger.info(f"scenario: {scenario}")
    logger.info("-------------------------------------")


    #GENERATE PROMPT
    prompt_version="1.0-ns3"
    prompt = generatePrompt(scenario,environment,prompt_version)    
    
    output = runLLM(model_name,model_size,prompt)

    #Save to file
    model_name_str=model_name.replace("/","_")
    
    
    Path(f"output/{environment}").mkdir(parents=True, exist_ok=True)
    
    
    if environment=="simulated":
        output_filename = f"output/{environment}/{scenario}-{environment}-{prompt_version}-{model_name_str}-{model_size}.cc"
    if environment=="real":
        output_filename = f"output/{environment}/{scenario}-{environment}-{model_name_str}-{model_size}.py"
    
    logger.info(f"save to {output_filename}")
    with open(output_filename, "w") as file:
            file.write(output)
    return 0

def main():
    
    #Choose Model (Small)
    #model_name = "mistralai/Mistral-7B-Instruct-v0.3"
    #model_name ="mistralai/Ministral-8B-Instruct-2410" #Genera Funzioni vuote
    #model_name ="mistralai/Mistral-Large-Instruct-2407" #Too large
    #model_name ="mistralai/Mistral-Small-Instruct-2409"
    #model_name ="mistralai/Codestral-22B-v0.1"
    #model_name="mistralai/Mistral-Nemo-Instruct-2407" # Non genera codice Python per Fabric WiFi
    
    
    #model_small_names =["mistralai/Mamba-Codestral-7B-v0.1","google/codegemma-7b-it","codellama/CodeLlama-7b-Instruct-hf"]
    
    model_small_names =["google/codegemma-7b-it","codellama/CodeLlama-7b-Instruct-hf"]
    model_large_names =["mistral-large-latest","gemini-exp-1206","gemini-2.0-flash-exp"]
    scenarios = ["smart_city","smart_agriculture","smart_home"]
    environments = ["simulated","real"]

    for environment in environments:
        for scenario in scenarios:
            
            for model_name in model_small_names:
                print(model_name)
                run_experiment(scenario="smart_city",environment=environment,model_name=model_name,model_size="small")            
            
            for model_name in model_large_names:
            
                    print(model_name)
                    run_experiment(scenario=scenario,environment=environment,model_name=model_name,model_size="large")            

    
if __name__ == "__main__":
        main()
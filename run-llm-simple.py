import torch
from transformers import AutoTokenizer, AutoModelForCausalLM
import logging

# Configura il logging di base
logging.basicConfig(level=logging.INFO, format='[%(asctime)s][%(levelname)s][%(filename)s:%(lineno)d][%(funcName)s]%(message)s')
logger = logging.getLogger(__name__)

# Determina il device da usare (GPU se disponibile, altrimenti CPU)
device = "cuda" if torch.cuda.is_available() else "cpu"
logger.info(f"Using device: {device}")

def run_simple_model(model_name: str, prompt: str, max_new_tokens: int = 100, temperature: float = 0.7, top_p: float = 0.95, top_k: int = 50):
    """
    Carica un modello Hugging Face e esegue un prompt semplice.

    Args:
        model_name (str): Il nome del modello da caricare da Hugging Face.
        prompt (str): Il prompt di testo da inviare al modello.
        max_new_tokens (int): Il numero massimo di nuovi token da generare.
        temperature (float): Il valore di temperature per la generazione (controlla la casualità).
        top_p (float): Il valore di top_p per la generazione (campionamento nucleo).
        top_k (int): Il valore di top_k per la generazione (campionamento top-k).

    Returns:
        str: L'output generato dal modello.
    """
    try:
        logger.info(f"Loading model: {model_name}")
        tokenizer = AutoTokenizer.from_pretrained(model_name)
        # Utilizza torch_dtype automatico o specificalo se necessario (es. torch.float16 per GPU)
        model = AutoModelForCausalLM.from_pretrained(model_name, torch_dtype=torch.bfloat16 if device == "cuda" else torch.float32).to(device)
        logger.info("Model loaded successfully.")

        logger.info("Tokenizing input prompt.")
        input_ids = tokenizer(prompt, return_tensors="pt").to(device)

        logger.info("Generating output...")
        # Aggiungi pad_token_id per evitare warning se il batch non è pieno (comune con batch size 1)
        outputs = model.generate(
            **input_ids,
            max_new_tokens=max_new_tokens,
            pad_token_id=tokenizer.eos_token_id,
            temperature=temperature,
            do_sample=True,
            top_p=top_p,
            top_k=top_k,
            # Evita la generazione di token di fine sequenza multipli all'interno della risposta
            eos_token_id=tokenizer.eos_token_id
            
        )

        logger.info("Decoding output.")
        # Decodifica solo la parte generata dal modello (escludendo il prompt iniziale)
        output = tokenizer.decode(outputs[0], skip_special_tokens=True)

        # Rimuovi il prompt iniziale dall'output decodificato per ottenere solo la generazione
        if output.startswith(prompt):
             output = output[len(prompt):].lstrip()


        logger.info("Generation complete.")
        return output

    except Exception as e:
        logger.error(f"An error occurred: {e}")
        return f"Error during model execution: {e}"

if __name__ == "__main__":
    # Scegli un modello Hugging Face da testare
    # Esempi di modelli piccoli e instruct-tuned:
    # "mistralai/Mistral-7B-Instruct-v0.2"
    # deepseek-ai/deepseek-coder-33b-instruct
    # deepseek-ai/deepseek-coder-6.7b-instruct

    #model_to_use = "deepseek-ai/deepseek-coder-6.7b-instruct" # Esempio
    model_to_use ="deepseek-ai/deepseek-llm-7b-base"
    # Definisci un prompt di test semplice
    test_prompt = "Scrivi una breve poesia sul mare."

    print(f"Using model: {model_to_use}")
    print(f"Prompt: {test_prompt}")

    # Esegui il modello con il prompt di test
    generated_text = run_simple_model(model_to_use, test_prompt)

    print("\n--- Generated Output ---")
    print(generated_text)
    print("------------------------")
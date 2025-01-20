def getTokenMistral(prompt, model_name = "mistral-large-latest"):
    from mistral_common.protocol.instruct.messages import (
        UserMessage,
    )
    from mistral_common.protocol.instruct.request import ChatCompletionRequest
    from mistral_common.protocol.instruct.tool_calls import (
        Function,
        Tool,
    )
    from mistral_common.tokens.tokenizers.mistral import MistralTokenizer

    tokenizer = MistralTokenizer.v3(is_tekken=True)
    tokenizer = MistralTokenizer.from_model(model_name)
    # Tokenize a list of messages
    tokenized = tokenizer.encode_chat_completion(
        ChatCompletionRequest(
            tools=None,
            messages=[
                UserMessage(content=prompt),
            ],
            model=model_name,
        )
    )
    tokens, text = tokenized.tokens, tokenized.text
    print("=================")
    print(len(tokens))    
    print(len(text))    
    print("=================")
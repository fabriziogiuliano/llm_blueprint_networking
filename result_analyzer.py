import pandas as pd
#Results analyzer
environment="simulated"
df = pd.read_pickle(f"output/validator/{environment}-results.pkl")
df.to_excel(f"output/validator/{environment}-results.xlsx")
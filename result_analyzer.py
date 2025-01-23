import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib.backends.backend_pdf import PdfPages
import os

#Results analyzer

figsize=(7,4)
environment="simulated"
df = pd.read_pickle(f"output/validator/{environment}-results.pkl")
#df.to_csv(f"output/validator/{environment}-results.csv")
#df.to_excel(f"output/validator/{environment}-results.xlsx")

# Create a directory for the PDF files if it doesn't exist
output_dir = "validator/model_performance_plots"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

# --- Overall Model Performance ---

palette=None
# 1. Average Score per Model (Bar Chart)
plt.figure(figsize=figsize)
avg_score_per_model = df.groupby("model_name")["score"].mean()
sns.barplot(
    x=avg_score_per_model.index,
    y=avg_score_per_model.values,
    hue=avg_score_per_model.index,
    palette=palette,
    legend=False,
)
plt.title("Average Score per Model")
plt.xlabel("Model Name")
plt.ylabel("Average Score")
plt.xticks(rotation=45, ha="right")
plt.tight_layout()
with PdfPages(os.path.join(output_dir, "1_average_score_per_model.pdf")) as pdf:
    pdf.savefig()
plt.close()

# 1b. Average Score per Model and Size (Grouped Bar Chart)
plt.figure(figsize=figsize)
sns.barplot(
    x="model_name",
    y="score",
    hue="model_size",
    data=df,
    palette=palette,
    errorbar=("ci", False),
)
plt.title("Average Score per Model (Grouped by Model Size)")
plt.xlabel("Model Name")
plt.ylabel("Average Score")
plt.xticks(rotation=45, ha="right")
plt.legend(title="Model Size")
plt.tight_layout()
with PdfPages(
    os.path.join(output_dir, "2_average_score_per_model_and_size.pdf")
) as pdf:
    pdf.savefig()
plt.close()

# 2. Distribution of Scores per Model (Box Plot)
plt.figure(figsize=figsize)
sns.boxplot(
    x="model_name",
    y="score",
    data=df,
    hue="model_name",
    palette=palette,
    legend=False,
)
plt.title("Distribution of Scores per Model")
plt.xlabel("Model Name")
plt.ylabel("Score")
plt.xticks(rotation=45, ha="right")
plt.tight_layout()
with PdfPages(
    os.path.join(output_dir, "3_distribution_of_scores_per_model.pdf")
) as pdf:
    pdf.savefig()
plt.close()

# --- Performance Breakdown by Feature ---

# 3. Average Score per Feature per Model (Grouped Bar Chart)

# Create a more descriptive feature mapping (optional but recommended)
feature_mapping = {
    1: "LoRa Devices Count",
    2: "WiFi Devices Count",
    3: "Location Assignment",
    4: "Traffic Duration/Type",
}
df["feature_description"] = df["id_feature"].map(feature_mapping)

plt.figure(figsize=figsize)
sns.barplot(
    x="model_name",
    y="score",
    hue="feature_description",
    data=df,
    palette=palette,
    errorbar=('ci', False)
)
plt.title("Average Score per Feature per Model")
plt.xlabel("Model Name")
plt.ylabel("Average Score")
plt.xticks(rotation=45, ha="right")
plt.legend(title="Feature")
plt.tight_layout()
with PdfPages(
    os.path.join(output_dir, "4_average_score_per_feature_per_model_grouped.pdf")
) as pdf:
    pdf.savefig()
plt.close()

# 4. Average Score per Feature per Model (Heatmap)
plt.figure(figsize=figsize)
heatmap_data = df.pivot_table(
    index="model_name", columns="feature_description", values="score", aggfunc="mean"
)
sns.heatmap(heatmap_data, annot=True, cmap="viridis", linewidths=0.5)
plt.title("Average Score per Feature per Model (Heatmap)")
plt.xlabel("Feature")
plt.ylabel("Model Name")
plt.tight_layout()
with PdfPages(
    os.path.join(output_dir, "5_average_score_per_feature_per_model_heatmap.pdf")
) as pdf:
    pdf.savefig()
plt.close()

# --- Performance by Scenario ---

# 5. Average Score per Scenario per Model (Grouped Bar Chart)
plt.figure(figsize=figsize)
sns.barplot(
    x="model_name",
    y="score",
    hue="scenario",
    data=df,
    palette=palette,
    errorbar=('ci', False)
)
plt.title("Average Score per Scenario per Model")
plt.xlabel("Model Name")
plt.ylabel("Average Score")
plt.xticks(rotation=45, ha="right")
plt.legend(title="Scenario")
plt.tight_layout()
with PdfPages(
    os.path.join(output_dir, "6_average_score_per_scenario_per_model.pdf")
) as pdf:
    pdf.savefig()
plt.close()

# --- Model Understanding ---

# 6. Distribution of Model Name Understanding Scores (Box Plot)
plt.figure(figsize=figsize)
sns.boxplot(
    x="model_name_understanding",
    y="score",
    data=df,
    hue="model_name_understanding",
    palette=palette,
    legend=False
)
plt.title("Distribution of Model Name Understanding Scores")
plt.xlabel("Model Name (Understanding)")
plt.ylabel("Score")
plt.xticks(rotation=45, ha="right")
plt.tight_layout()
with PdfPages(
    os.path.join(output_dir, "7_distribution_of_model_name_understanding_scores.pdf")
) as pdf:
    pdf.savefig()
plt.close()

print(f"Plots saved to individual PDF files in the '{output_dir}' directory.")
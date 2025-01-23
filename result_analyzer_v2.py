import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib.backends.backend_pdf import PdfPages
import os

#Results analyzer

figsize=(7,4)
environment="simulated"
df = pd.read_pickle(f"output/validator/{environment}-results.pkl")
df.to_csv(f"output/validator/{environment}-results.csv")
df.to_excel(f"output/validator/{environment}-results.xlsx")

new_df = df.copy()  # Create a copy to avoid modifying the original DataFrame
for col in new_df.columns:
    if new_df[col].dtype == 'object':  # Only apply to string columns
        new_df[col] = new_df[col].map(lambda x: x.replace("google/", "") if isinstance(x, str) else x)
        new_df[col] = new_df[col].map(lambda x: x.replace("codellama/", "") if isinstance(x, str) else x)
df = new_df

palette=None
# --- Configuration ---
FIGURE_SIZE = (7,4)  # Define a consistent figure size (width, height)
OUTPUT_DIR = "output/validator/model_performance_plots"

DEFAULT_X_LIMS = (0, 5) # Define a costant x lims

# Set a more visually appealing style
sns.set_style("whitegrid")

# Create the output directory if it doesn't exist
if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

# --- Helper Function ---
def save_plot_to_pdf(pdf_file_name):
    """Saves the current Matplotlib plot to a PDF file."""
    with PdfPages(os.path.join(OUTPUT_DIR, pdf_file_name)) as pdf:
        pdf.savefig(bbox_inches="tight")
    plt.close()

# --- Plotting Functions ---

# 1. Average Score per Model (Horizontal Bar Chart)
def plot_average_score_per_model():
    plt.figure(figsize=FIGURE_SIZE)
    avg_score_per_model = df.groupby("model_name")["score"].mean()
    sns.barplot(
        y=avg_score_per_model.index,
        x=avg_score_per_model.values,
        hue=avg_score_per_model.index,
        palette=palette,
        orient="h",
        legend=False
    )
    plt.title("Average Score per Model")
    plt.ylabel("Model Name")
    plt.xlabel("Average Score")
    plt.xlim(DEFAULT_X_LIMS)
    plt.tight_layout()
    save_plot_to_pdf("1_average_score_per_model.pdf")

# 2. Average Score per Model and Size (Grouped Horizontal Bar Chart)
def plot_average_score_per_model_and_size():
    plt.figure(figsize=FIGURE_SIZE)
    sns.barplot(
        y="model_name",
        x="score",
        hue="model_size",
        data=df,
        palette=palette,
        errorbar=("ci", False),
        orient="h",
    )
    plt.title("Average Score per Model (Grouped by Model Size)")
    plt.ylabel("Model Name")
    plt.xlabel("Average Score")
    plt.xlim(DEFAULT_X_LIMS)

    plt.legend(title="Model Size", bbox_to_anchor=(1.05, 1), loc="upper left")

    plt.tight_layout()
    save_plot_to_pdf("2_average_score_per_model_and_size.pdf")

# 3. Distribution of Scores per Model (Box Plot)
def plot_distribution_of_scores_per_model():
    plt.figure(figsize=FIGURE_SIZE)
    sns.boxplot(
        x="score",
        y="model_name",
        data=df,
        hue="model_name",
        palette=palette,
        orient="h",
        legend=False
    )
    plt.title("Distribution of Scores per Model")
    plt.xlabel("Score")
    plt.ylabel("Model Name")
    plt.xlim(DEFAULT_X_LIMS)
    plt.tight_layout()
    save_plot_to_pdf("3_distribution_of_scores_per_model.pdf")

# 4. Average Score per Feature per Model (Grouped Horizontal Bar Chart)
def plot_average_score_per_feature_per_model():
    feature_mapping = {
        1: "LoRa Devices Count",
        2: "WiFi Devices Count",
        3: "Location Assignment",
        4: "Traffic Duration/Type",
    }
    df["feature_description"] = df["id_feature"].map(feature_mapping)

    plt.figure(figsize=FIGURE_SIZE)
    sns.barplot(
        y="model_name",
        x="score",
        hue="feature_description",
        data=df,
        palette=palette,
        errorbar=("ci", False),
        orient="h",
    )
    plt.title("Average Score per Feature per Model")
    plt.ylabel("Model Name")
    plt.xlabel("Average Score")
    plt.xlim(DEFAULT_X_LIMS)

    #plt.legend(title="Feature", bbox_to_anchor=(1.05, 1), loc="upper left")
    #plt.legend()
    plt.legend(loc='upper center', bbox_to_anchor=(0.3, -0.2), ncol=5, fontsize=8)

    plt.tight_layout()
    save_plot_to_pdf("4_average_score_per_feature_per_model_grouped.pdf")

# 5. Average Score per Feature per Model (Heatmap)
def plot_average_score_per_feature_per_model_heatmap():
    feature_mapping = {
        1: "LoRa Devices Count",
        2: "WiFi Devices Count",
        3: "Location Assignment",
        4: "Traffic Duration/Type",
    }
    df["feature_description"] = df["id_feature"].map(feature_mapping)
    
    plt.figure(figsize=FIGURE_SIZE)
    heatmap_data = df.pivot_table(
        index="model_name", columns="feature_description", values="score", aggfunc="mean"
    )
    sns.heatmap(heatmap_data, annot=True, cmap="viridis", linewidths=0.5, vmin=0, vmax=5)
    plt.title("Average Score per Feature per Model (Heatmap)")
    plt.xlabel("Feature")
    plt.ylabel("Model Name")
    plt.tight_layout()
    save_plot_to_pdf("5_average_score_per_feature_per_model_heatmap.pdf")

# 6. Average Score per Scenario per Model (Grouped Horizontal Bar Chart)
def plot_average_score_per_scenario_per_model():
    plt.figure(figsize=FIGURE_SIZE)
    sns.barplot(
        y="model_name",
        x="score",
        hue="scenario",
        data=df,
        palette=palette,
        errorbar=("ci", False),
        orient="h",
    )
    plt.title("Average Score per Scenario per Model")
    plt.ylabel("Model Name")
    plt.xlabel("Average Score")
    plt.xlim(DEFAULT_X_LIMS)

    #plt.legend(title="Scenario", bbox_to_anchor=(1.05, 1), loc="upper left")
    
    #plt.legend(loc='lower center', bbox_to_anchor=(0.5, -0.4), ncol=4)
    #plt.legend(loc='lower center',  ncol=3)
    
    #plt.legend(title="Scenario", bbox_to_anchor=(1.05, -1), loc="lower center",ncol=4)
    
    plt.legend(loc='upper center', bbox_to_anchor=(0.3, -0.2), ncol=3, fontsize=8)
        

    plt.tight_layout()
    save_plot_to_pdf("6_average_score_per_scenario_per_model.pdf")

# --- Main Execution ---

if __name__ == "__main__":
    plot_average_score_per_model()
    #plot_average_score_per_model_and_size()
    #plot_distribution_of_scores_per_model()
    plot_average_score_per_feature_per_model()
    plot_average_score_per_feature_per_model_heatmap()
    plot_average_score_per_scenario_per_model()

    print(f"Plots saved to individual PDF files in the '{OUTPUT_DIR}' directory.")
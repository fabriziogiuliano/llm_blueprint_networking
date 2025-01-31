import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib.backends.backend_pdf import PdfPages
import os

#Results analyzer

figsize=(7,4)

#environment="simulated"; prompt_version="1.0-ns3"
environment="simulated"; prompt_version="2.0-ns3"
#environment="real"; prompt_version=""

feature_mapping={}
if environment=="simulated":
    feature_mapping = {
        0: "C++ Code generation",
        1: "LoRa Devices Detection",
        2: "WiFi Devices Detection",
        3: "Location Assignment",
        4: "Traffic Duration/Type",
    }

if environment=="real":
    feature_mapping = {
        1: "LoRa code matching",
        2: "WiFi code matching",        
        3: "JSON fields match with code",
    }
    
if prompt_version=="":
    filename_output=f"output/validator/{environment}-results"    
else:
    filename_output=f"output/validator/{environment}-{prompt_version}-results"

df = pd.read_pickle(f"{filename_output}.pkl")
print(df)
#print(len(df[df["scenario"=="001-smart_agriculture"]]))

exit()
df["score"]=df["score"]*2/10*100 #in percentage    
df.to_csv(f"{filename_output}.csv")
df.to_excel(f"{filename_output}.xlsx")

new_df = df.copy()  # Create a copy to avoid modifying the original DataFrame
for col in new_df.columns:
    if new_df[col].dtype == 'object':  # Only apply to string columns
        new_df[col] = new_df[col].map(lambda x: x.replace("google/", "") if isinstance(x, str) else x)
        new_df[col] = new_df[col].map(lambda x: x.replace("codellama/", "") if isinstance(x, str) else x)
df = new_df

palette=None
# --- Configuration ---
FIGURE_SIZE = (7,4)  # Define a consistent figure size (width, height)
if prompt_version=="":
    OUTPUT_DIR = f"output/validator/model_performance_plots/{environment}"
else:
    OUTPUT_DIR = f"output/validator/model_performance_plots/{environment}-{prompt_version}"
    
DEFAULT_X_LIMS = (0, 100) # Define a costant x lims

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
    #plt.ylabel("Model Name")
    plt.xlabel("Average Score [%]")
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
        errorbar=None,
        orient="h",
    )
    plt.title("Average Score per Model (Grouped by Model Size)")
    plt.ylabel("Model Name")
    plt.xlabel("Average Score [%]")
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

    df["feature_description"] = df["id_feature"].map(feature_mapping)


    plt.figure(figsize=FIGURE_SIZE)
    sns.barplot(
        y="model_name",
        x="score",
        hue="feature_description",
        data=df,
        palette=palette,
        errorbar=None,
        orient="h",
    )
    plt.title("Average Score per Feature per Model")
    plt.ylabel("Model Name")
    plt.xlabel("Average Score [%]")
    plt.xlim(DEFAULT_X_LIMS)

    #plt.legend(title="Feature", bbox_to_anchor=(1.05, 1), loc="upper left")
    #plt.legend()
    plt.legend(loc='upper center', bbox_to_anchor=(0.3, -0.2), ncol=5, fontsize=8)

    plt.tight_layout()
    save_plot_to_pdf("4_average_score_per_feature_per_model_grouped.pdf")

# 5. Average Score per Feature per Model (Heatmap)
def plot_average_score_per_feature_per_model_heatmap():
    
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
        errorbar=None,
        orient="h",
    )
    plt.title("Average Score per Scenario per Model")
    plt.ylabel("Model Name")
    plt.xlabel("Average Score [%]")
    plt.xlim(DEFAULT_X_LIMS)
    
    plt.legend(loc='upper center', bbox_to_anchor=(0.3, -0.2), ncol=3, fontsize=8)
        
    plt.tight_layout()
    save_plot_to_pdf("6_average_score_per_scenario_per_model.pdf")



def plot_average_score_per_grouped_scenario_per_model_2():  # Changed function name
    # Create a new column for scenario groups
    for i in range(0,4):
        df["scenario"] = df["scenario"].str.replace(f"00{i}-", "")
    
    # Calculate the average score per scenario group and model


    plt.figure(figsize=FIGURE_SIZE)
    sns.barplot(
        y="model_name",
        x="score",
        hue="scenario",
        data=df,
        palette=palette,
        errorbar=None,
        orient="h",
    )
    plt.title("Average Score per Scenario Group per Model")
    plt.ylabel("Model Name")
    plt.xlabel("Average Score [%]")
    plt.xlim(DEFAULT_X_LIMS)

    plt.legend(loc="upper center", bbox_to_anchor=(0.5, -0.15), ncol=3, fontsize=8)

    plt.tight_layout()
    save_plot_to_pdf("7_average_score_per_grouped_scenario_per_model.pdf")  # Changed file name



# --- Main Execution ---scenario_group

if __name__ == "__main__":
    #plot_average_score_per_model()
    #plot_average_score_per_model_and_size()
    #plot_distribution_of_scores_per_model()
    plot_average_score_per_feature_per_model()
    #plot_average_score_per_feature_per_model_heatmap()
    plot_average_score_per_scenario_per_model()
    plot_average_score_per_grouped_scenario_per_model_2()

    print(f"Plots saved to individual PDF files in the '{OUTPUT_DIR}' directory.")
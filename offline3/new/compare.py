import os
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages

# Directories for two datasets
directory_1 = "TcpScalable"
directory_2 = "TcpScalableTweak"

metrics = ["cwnd", "inflight", "next-rx", "next-tx", "rto", "rtt", "ssth"]

# Resampling interval in seconds
resample_interval = 0.1


# Function to read, aggregate, and resample data for a given metric from a directory
def aggregate_and_resample_metric(directory, metric):
    data_frames = []
    for flow in range(2):  # Loop over the 2 flows
        file_path = os.path.join(directory, f"TcpVariantsComparison-flow{flow}-{metric}.data")
        if os.path.exists(file_path):
            # Read the time series data
            df = pd.read_csv(file_path, delim_whitespace=True, header=None, names=["Time", metric])
            data_frames.append(df)
        else:
            print(f"File not found: {file_path}")

    # Merge all data frames and calculate the average
    if data_frames:
        merged_df = pd.concat(data_frames).groupby("Time").mean().reset_index()
        # Convert Time column to datetime for resampling
        merged_df["Time"] = pd.to_datetime(merged_df["Time"], unit="s")
        # Resample data to reduce density
        merged_df = merged_df.set_index("Time").resample(f"{resample_interval}S").mean().fillna(
            method="ffill").reset_index()
        # Convert the Time column back to plain seconds
        merged_df["Time"] = (merged_df["Time"] - pd.Timestamp("1970-01-01")) / pd.Timedelta(seconds=1)
        return merged_df
    else:
        print(f"No data found for metric: {metric}")
        return None


# Plotting function for comparison
def plot_metric_comparison(metric, data_1, data_2, pdf_pages):
    plt.figure(figsize=(10, 6))
    if data_1 is not None:
        plt.plot(data_1["Time"], data_1[metric], label=f"TcpScalable Avg {metric}", color="magenta")
    if data_2 is not None:
        plt.plot(data_2["Time"], data_2[metric], label=f"TcpScalableTweak Avg {metric}", color="green")

    plt.xlabel("Time (s)")
    plt.ylabel(metric.capitalize())
    plt.title(f"Comparison of {metric.capitalize()} Between Two Datasets")
    plt.legend()
    plt.grid()
    pdf_pages.savefig()  # Save the current figure to the PDF
    plt.close()


# Create a PDF to save all plots
pdf_file = "TcpComparison.pdf"
with PdfPages(pdf_file) as pdf_pages:
    # Process each metric and add comparison plots to the PDF
    for metric in metrics:
        avg_data_1 = aggregate_and_resample_metric(directory_1, metric)
        avg_data_2 = aggregate_and_resample_metric(directory_2, metric)
        plot_metric_comparison(metric, avg_data_1, avg_data_2, pdf_pages)

print(f"All comparison plots saved to {pdf_file}")

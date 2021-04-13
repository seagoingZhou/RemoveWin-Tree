import os
import csv
import sys
import re
import functools
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Polygon
import pandas as pd
import seaborn as sns

def data_gen(file1, model1, file2, model2):
    data = {} 
    data["model"] = []
    data["overhead(byte)"] = []
    data["time(s)"] = []
    cnt = 0
    for line in open(file1):
        cnt = cnt + 1
        data["model"].append(model1)
        data["overhead(byte)"].append(float(line))
        data["time(s)"].append(cnt)
    cnt = 0
    for line in open(file2):
        cnt = cnt + 1
        data["model"].append(model2)
        data["overhead(byte)"].append(float(line))
        data["time(s)"].append(cnt)
    return data

def plot(datas):
    df = pd.DataFrame.from_dict(datas)
    fig, ax = plt.subplots()
    sns.lineplot(x='time(s)', y = "overhead(byte)", hue = "model", data = df)
    sns.set(font_scale = 2)
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels)
    plt.legend(fontsize=12, loc='upper left',facecolor='w')
    plt.show()

def main():
    file1 = "D:/data/tree/raw-data/overhead/addrem/rw-9-7000-100-20/s.ovhd"
    model1 = "ins-del"
    file2 = "D:/data/tree/raw-data/overhead/chg/rw-9-7000-100-20/s.ovhd"
    model2 = "chg"
    datas = data_gen(file1, model1, file2, model2)
    plot(datas)

if __name__ == "__main__":
    main()
    print("done!\n")
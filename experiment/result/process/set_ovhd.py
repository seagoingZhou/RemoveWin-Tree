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

MODEL_MAP = {2:"operation speed(op/s)", 3:"network delay(ms)"}
TYPE_MAP = {'or':'OR-Set','pn':'PN-Set','rw':'RWF-Set'}
TYPE_PRI = {'or':1,'pn':2,'rw':0}

def cmp(a1,a2):
    return TYPE_PRI[re.split('-',a1)[0]] - TYPE_PRI[re.split('-',a2)[0]]
    

def data_gen(data,file1, model1):
    cnt = 0
    for line in open(file1):
        cnt = cnt + 1
        data["type"].append(model1)
        data["overhead(byte)"].append(float(line))
        data["time(s)"].append(cnt)
    return data

def plot(datas):
    df = pd.DataFrame.from_dict(datas)
    fig, ax = plt.subplots()
    sns.lineplot(x='time(s)', y = "overhead(byte)", hue = "type", data = df)
    sns.set(font_scale = 2)
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels)
    plt.legend(fontsize=12, loc='upper left',facecolor='w')
    plt.show()

def main():
    directory = "D:/data/set/overhead"
    filenames=sorted(os.listdir(directory),key=functools.cmp_to_key(cmp))
    data = {} 
    data["type"] = []
    data["overhead(byte)"] = []
    data["time(s)"] = []
    for file in filenames:
        file_list = re.split('-',file)
        model = TYPE_MAP[file_list[0]]
        file_dir = directory+'/'+file + "/s.ovhd"
        data = data_gen(data,file_dir,model)
    plot(data)

if __name__ == "__main__":
    main()
    print("done!\n")
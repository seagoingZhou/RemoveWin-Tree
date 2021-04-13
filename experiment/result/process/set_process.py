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

MODEL = 0;
REPLICA = 1
DELAY = 3
SPEED = 2
directory = "./ProcessedData/speed"

MODEL_MAP = {2:"operation speed(op/s)", 3:"network delay(ms)"}
TYPE_MAP = {'or':'OR-Set','pn':'PN-Set','rw':'RWF-Set'}
TYPE_PRI = {'or':1,'pn':2,'rw':0}

MODEL = REPLICA
def cmp(a1,a2):
    key1 = float(re.split('-',a1)[MODEL])
    key2 = float(re.split('-',a2)[MODEL])
    if key1 == key2:
        return TYPE_PRI[re.split('-',a1)[0]] - TYPE_PRI[re.split('-',a2)[0]]
    return key1-key2

def jaccord(directory="."):
    length = 0;
    similarity = 0;
    for line in open(directory + "/s.set"):
        length += 1
        point = line.split(' ')
        similarity += (float(point[2])/(float(point[0])+float(point[1])-float(point[2])))
    res = similarity/length
    return res

def data_gen(directory):
    filenames=sorted(os.listdir(directory),key=functools.cmp_to_key(cmp))
    datas = {} 
    datas['type'] = []
    datas['similarity'] = []
    datas['operation speed(op/s)'] = []
    datas['network delay(ms)'] = []
    datas['replica number'] = []
    for file in filenames:
        file_list = re.split('-',file)
        jaccord_value = jaccord(directory+'/'+file);
        datas['type'].append(TYPE_MAP[file_list[0]])
        datas['replica number'].append(file_list[1])
        datas['operation speed(op/s)'].append(file_list[2])
        datas['network delay(ms)'].append(file_list[3])
        datas['similarity'].append(jaccord_value)
    return datas

def data_gen_simple(directory):
    filenames=sorted(os.listdir(directory),key=functools.cmp_to_key(cmp))
    datas = {} 
    datas['type'] = []
    datas['similarity'] = []
    datas['operation speed(op/s)'] = []
    datas['network delay(ms)'] = []
    datas['replica number'] = []
    for file in filenames:
        file_list = re.split('-',file)
        for line in open(directory+'/'+file + "/s.set"):
            datas['type'].append(TYPE_MAP[file_list[0]])
            datas['replica number'].append(file_list[1])
            datas['operation speed(op/s)'].append(file_list[2])
            datas['network delay(ms)'].append(file_list[3])
            point = line.split(' ')
            similarity = (float(point[2])/(float(point[0])+float(point[1])-float(point[2])))
            datas['similarity'].append(similarity)
    return datas

def plot(datas):
    df = pd.DataFrame.from_dict(datas)
    fig, ax = plt.subplots()
    sns.boxplot(x='replica number', y = "similarity", hue = "type", data = df)
    sns.set(font_scale = 2)
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels)
    plt.legend(fontsize=10, loc='upper right',facecolor='w')
    plt.show()

def main():
    directory = "D:/data/set/complex-replica"
    datas = data_gen(directory)
    plot(datas)

if __name__ == "__main__":
    main()
    print("done!\n")




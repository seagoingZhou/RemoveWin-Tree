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

MODEL = 0
REPLICA = 1
DELAY = 3
SPEED = 2
directory = "./ProcessedData/speed"

MODEL_MAP = {1:"replica number", 2:"operation speed(op/s)", 3:"network delay(ms)"}

MODEL = SPEED
def cmp(a1,a2):
    key1 = float(re.split('-',a1)[MODEL])
    key2 = float(re.split('-',a2)[MODEL])
    return key1-key2




def TreeSimilarity(directory):
    filenames=sorted(os.listdir(directory),key=functools.cmp_to_key(cmp))
    #columns = []
    datas = {}
    maxlen = 0
    
    for file in filenames:
        col=re.split('-',file)[MODEL]
        sgldata = []
        #print(file)
        with open(directory+"/"+file) as fin:
            cin = csv.reader(fin)
            sglraw = [row for row in cin]
            #print(sglraw[0])
        for point in sglraw:
            print(point)
            if len(point) == 3:
                cnt_grd = re.split('#', point[0])
                sgldata.append(float(cnt_grd[1])/(3 * (float(point[1]) + float(point[2]) - float(cnt_grd[0]))))
                #sgldata.append(float(point[0]))
                #sgldata.append(float(point[0])/((float(point[1]) + float(point[2]))))
        datas[col]=sgldata 
    
    return datas

def merge_data(raw_data, merged_data, model):
    for key in raw_data:
    	for val in raw_data[key]:
            merged_data[MODEL_MAP[MODEL]].append(float(key))
            merged_data["similarity"].append(float(val))
            merged_data["model"].append(model)
    #return merge_data


DIR_MAP = {REPLICA:"replica", SPEED:"speed", DELAY:"delay"}
save_dir1 = "D:/data/tree/processed-data/{model}/addrem".format(model = DIR_MAP[MODEL])
save_dir2 = "D:/data/tree/processed-data/{model}/chg".format(model = DIR_MAP[MODEL])

def process(dir1, dir2):
    merged_data = {}
    merged_data[MODEL_MAP[MODEL]] = []
    merged_data["similarity"] = []
    merged_data["model"] = []
    data1 = TreeSimilarity(dir1)
    data2 = TreeSimilarity(dir2)

    merge_data(data1, merged_data, "ins-del")
    merge_data(data2, merged_data, "chg")
    df = pd.DataFrame.from_dict(merged_data)
    print(df.head())
    sns.boxplot(x=MODEL_MAP[MODEL], y = "similarity", hue = "model", data = df)
    sns.set(font_scale = 2)
    plt.show()

process(save_dir1, save_dir2)




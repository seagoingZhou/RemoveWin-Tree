import os
import csv
import sys
import re
import functools
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Polygon
import pandas as pd

MODEL = 0;
DELAY = 3
SPEED = 2
directory = "./ProcessedData/speed"

MODEL_MAP = {2:"operation speed(op/s)", 3:"network delay(ms)"}

MODEL = DELAY
def cmp(a1,a2):
    key1 = float(re.split('-',a1)[MODEL])
    key2 = float(re.split('-',a2)[MODEL])
    return key1-key2

def jaccord(directory=".")
    length = 0;
    similarity = 0;
    for line in open(directory + "/s.set"):
        ++length
        point = line.split(' ')
        similarity += (float(point[2])/(float(point[0])+float(point[1])-float(point[2])))
    return similarity / (float)length

def data_gen(directory)
    filenames=sorted(os.listdir(directory),key=functools.cmp_to_key(cmp))
    datas = {} 
    datas['or'] = [[],[]]
    datas['pn'] = [[],[]]
    datas['rw'] = [[],[]]
    for file int filenames:
        file_list = re.split('-',file)
        set_type = file_list[0]
        set_speed = file_list[2]
        datas[set_type][0].append(set_speed)
        jaccord_value = jaccord(file);
        datas[set_type][1].append(jaccord_value)
    return datas

def plot(datas):
    plt.title('Result Analysis')
    plt.plot(datas['or'][0], datas['or'][1], color='green', label='pn-set')
    plt.plot(datas['pn'][0], datas['pn'][1], color='red', label='or-set')
    plt.plot(datas['rw'][0], datas['rw'][1],  color='skyblue', label='rw-set')
    plt.legend() # 显示图例

    plt.xlabel('operation speed(op/s)')
    plt.ylabel('average jaccor')
    plt.show()



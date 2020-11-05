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
SPEED = 1
directory = "./ProcessedData/delay"

MODEL = DELAY
def cmp(a1,a2):
    key1 = float(re.split('[,\(]',a1)[MODEL])
    key2 = float(re.split('[,\(]',a2)[MODEL])
    return key1-key2




def TreeSimilarity(directory):
    filenames=sorted(os.listdir(directory),key=functools.cmp_to_key(cmp))
    #columns = []
    datas = {}
    maxlen = 0
    
    for file in filenames:
        col=re.split('[,\(]',file)[MODEL]
        sgldata = []
        #print(file)
        with open(directory+"/"+file) as fin:
            cin = csv.reader(fin)
            sglraw = [row for row in cin]
            #print(sglraw[0])
        for point in sglraw:
            sgldata.append(1 - float(point[0])/max(float(point[1]),float(point[2])))
        datas[col]=sgldata 
        maxlen = max(maxlen,len(sgldata))
    
    for file in filenames:
        col=re.split('[,\(]',file)[MODEL]
        diff = maxlen-len(datas[col])
        datas[col].extend([None]*diff)
    return datas


data = TreeSimilarity(directory)
df = pd.DataFrame.from_dict(data)
df.plot.box()
plt.show()




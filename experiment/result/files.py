import os
import csv

foldlist = os.listdir("./RawData")
#for fold in foldlist:
listT = [[0,1,3],[2,3,4],[4,6,5]]
d = "{dir}/{foldname}".format(dir='./RawData',foldname=foldlist[0])
print(foldlist)
'''
with open(d+"/s.tree") as f:
    for line in f.readlines():
        print(line)
'''



import tago
import sys
import csv
import time
import numpy as np
from sklearn.neighbors import KNeighborsClassifier
import pandas as pd

# Upload data to dashboard
def upload_to_dashboard(position):
    dictArr = []
    data = {
        'variable': "position",
        'value': position,
    }
    dictArr.append(data)
    result = mydevice.insert(dictArr)


# Training Data file
df = pd.read_csv("~/csse4011/prac3/TrainingData1.csv")


# Get rssi values
values = np.array((df['RSSI'].str.translate(
    str.maketrans({'[': '', ']': '', "'":''})).tolist()))
newArr = []


# Sort array values
for row in values:
    val = row.split(',')
    val = np.array(val)
    val = val.astype(float)
    newArr.append(val)

#transform to np arrays
RSSIData = np.array(newArr)
positionLabel = np.array(df['Position'])

#create model
knn = KNeighborsClassifier(n_neighbors=3)
knnmodel = knn.fit(RSSIData, positionLabel)

#device to pull data from
mydevice = tago.Device('3b706237-7aff-4dd7-b1a9-028158f4ac19')

filtering = {
        "variable": "rssi",
        'query': 'last_value',
}

while(1):
    value = mydevice.find(filtering)
    listofRssi = value['result'][0]['value'].strip('][').split(', ')
    rssi = []
    for item in listofRssi:
        rssi.append(int(item.strip('""')))
    value = np.array(rssi)
    position = knnmodel.predict([value]) # prediction
    print(position) 
    upload_to_dashboard(position[0]) #upload prediction
    time.sleep(2)

import numpy as np
from bleak import BleakScanner
import struct
import asyncio
import pandas as pd
import re
from sklearn.neighbors import KNeighborsClassifier
from sklearn import metrics
import threading
import tago
from math import sqrt, atan, pi

#Output Data
device = tago.Device('cd448ca0-221a-4433-9565-dcc2f77711c5')


def upload_to_dashboard(arr1, arr2, action):
    x = round(arr1[0],3)
    y = round(arr1[1],3)
    z = round(arr1[2],3)
    pitch1 = round(atan(x / sqrt(y * y + z * z)) * 180.0 / pi,3)
    roll1 = round(atan(y / sqrt(x * x + z * z)) * 180.0 / pi,3)

    x2 = round(arr2[0],3)
    y2 = round(arr2[1],3)
    z2 = round(arr2[2],3)
    pitch2 = round(atan(x2 / sqrt(y2 * y2 + z2 * z2)) * 180.0 / pi,3)
    roll2 = round(atan(y2 / sqrt(x2 * x2 + z2 * z2)) * 180.0 / pi,3)

    dictArr = []
    data = {
        'variable': f"Action",
        'value': action,
    }
    dictArr.append(data)
    data = {
        'variable': f"x1Accel",
        'unit': 'ms^-2',
        'value': x,
    }
    dictArr.append(data)
    data = {
        'variable': f"x2Accel",
        'unit': 'ms^-2',
        'value': x2,
    }
    dictArr.append(data)
    data = {
        'variable': f"y1Accel",
        'unit': 'ms^-2',
        'value': y,
    }
    dictArr.append(data)
    data = {
        'variable': f"y2Accel",
        'unit': 'ms^-2',
        'value': y2,
    }
    dictArr.append(data)
    data = {
        'variable': f"z1Accel",
        'unit': 'ms^-2',
        'value': z,
    }
    dictArr.append(data)
    data = {
        'variable': f"z2Accel",
        'unit': 'ms^-2',
        'value': z2,
    }
    dictArr.append(data)
    

    data = {
        'variable': "Pitch1",
        'unit': '°',
        'value': pitch1,
    }
    dictArr.append(data)
    data = {
        'variable': "Roll1",
        'unit': '°',
        'value': roll1,
    }
    dictArr.append(data)
    data = {
        'variable': "Pitch2",
        'unit': '°',
        'value': pitch2,
    }
    dictArr.append(data)
    data = {
        'variable': "Roll2",
        'unit': '°',
        'value': roll2,
    }
    dictArr.append(data)
    result = device.insert(dictArr)

# Addresses of our two thingies
thingy1Name = "Thingy_1"
thingy2Name = "Thingy"

# Booleans that keep track of current queue
thingy1Bool = False
thingy2Bool = False

# Array used for holding values set in the asynchronous callback
tempDictArr = np.empty(2, dtype=object)

# Training Data
df = pd.ExcelFile("TrainingData.xlsx")
# Parse Training Data
dfs = df.parse()
s = np.array((dfs['Accelerometor Data'].str.translate(
    str.maketrans({'[': '', ']': ''})).tolist()))
newArr = []

for row in s:
    t = row.split(',')
    t = np.array(t)
    t = t.astype(float)
    newArr.append(t)

accelArr = np.array(newArr)
actionArr = np.array(dfs['Action'])

# Create classifiers
knn = KNeighborsClassifier(n_neighbors=3)
knn = knn.fit(accelArr, actionArr)


def detection_callback(device, advertisement_data):
    """Asynch Callback

    Args:
        device : Bleak device object
        advertisement_data : Advertisement Data Read
    """

    global thingy1Bool
    global thingy2Bool
    try:
        if (device.name == thingy1Name or device.name == thingy2Name) and advertisement_data.service_data:
            a = next(iter(advertisement_data.service_data.values()))
            x = struct.unpack('f', a[:4])[0]
            y = struct.unpack('f', a[4:8])[0]
            z = struct.unpack('f', a[8:12])[0]
            accelArr = [x, y, z]
            # print(accelArr)
    except Exception as e:
        print("Advertisement Data")
        print(advertisement_data.service_data)
        print(e)
    if device.name == thingy1Name and advertisement_data.service_data:
        tempDictArr[0] = accelArr
        thingy1Bool = True
    elif device.name == thingy2Name and advertisement_data.service_data:
        tempDictArr[1] = accelArr
        thingy2Bool = True
    if thingy2Bool and thingy1Bool:
        tempAccel = np.append(tempDictArr[0], tempDictArr[1])
        action = knn.predict([tempAccel])
        print(action[0])
        thingy1Bool = False
        thingy2Bool = False
        uploadThread = threading.Thread(
            target=upload_to_dashboard, args=(tempDictArr[0], tempDictArr[1], action[0]))
        uploadThread.start()

async def run():
    scanner = BleakScanner(detection_callback)
    while True:
        await scanner.start()
        await asyncio.sleep(0.5)
        await scanner.stop()

loop = asyncio.get_event_loop()
loop.run_until_complete(run())
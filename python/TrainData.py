import numpy as np
from bleak import BleakScanner
import struct
import asyncio
import csv
import tago
import time
from playsound import playsound
import threading
from math import sqrt, atan, pi

filename = input("Enter A Filename: (enter 'd' for default)")
if filename == 'd':
    filename = "TrainingData"

action = input("Enter Your Activity: ")


dashinput = input("Would like to have data uploaded to the dash (Takes longer): ")
if (dashinput == 'y' or dashinput == 'yes'):
    todash = True
else: 
    todash = False
duration = input("Enter How Many Seconds You Want To Collect Data For: ")

dictArr = []

start = 0

thingy1bool = False
thingy2bool = False
accelArrDict = np.empty(2, dtype=object)

#Training Data
my_device1 = tago.Device('7da25920-808a-45dc-96ed-8ce8a1e2556c')




def upload_to_dashboard(arr1, arr2, action):
    x = round(arr1[0],3)
    y = round(arr1[1],3)
    z = round(arr1[2],3)
    pitch1 = round(atan(x / sqrt(y * y + z * z)) * 180.0 / pi,3)
    roll1 = round(atan(y / sqrt(x * x + z * z)) * 180.0 / pi,3)
    arr1 = [x,y,z]
    x2 = round(arr2[0],3)
    y2 = round(arr2[1],3)
    z2 = round(arr2[2],3)
    pitch2 = round(atan(x2 / sqrt(y2 * y2 + z2 * z2)) * 180.0 / pi,3)
    roll2 = round(atan(y2 / sqrt(x2 * x2 + z2 * z2)) * 180.0 / pi,3)
    arr2 = [x2,y2,z2]
    dictArr = []
    data = {
        'variable': f"d1Accel",
        'unit': 'ms^-2',
        'value': str(arr1),
    }
    dictArr.append(data)
    data = {
        'variable': f"d2Accel",
        'unit': 'ms^-2',
        'value': str(arr2),
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
    result = my_device1.insert(dictArr)
    # print(result)


def write_data():
    csv_file = f"{filename}.csv"
    csv_columns = ['Accelerometor_Data', 'Action']
    try:
        with open(csv_file, 'a', newline='') as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=csv_columns)
            for data in dictArr:
                writer.writerow(data)
    except IOError:
        print("I/O error")




def detection_callback(device, advertisement_data):
    global thingy1bool
    global thingy2bool
    try:
        if (device.name == "Thingy_1" or device.name == "Thingy") and advertisement_data:
            a = next(iter(advertisement_data.service_data.values()))            
            x = struct.unpack('f', a[:4])[0]
            y = struct.unpack('f', a[4:8])[0]
            z = struct.unpack('f', a[8:12])[0]
            accelArr = [x, y, z]
            # print(f"{device.name}, {accelArr}")
    except Exception as e:
        print("Advertisement Data")
        print(advertisement_data.service_data)
        print(e)
    if device.name == "Thingy_1" and advertisement_data:
        accelArrDict[0] = accelArr
            # upload_to_dashboard(x, y, z, action)
        thingy1bool = True
    elif device.name == "Thingy" and advertisement_data:
        accelArrDict[1] = accelArr
        thingy2bool = True
    if thingy2bool and thingy1bool:
        percentage = ((time.time() - start)/float(duration))*100
        percentage = round(percentage, 2)
        if percentage > 100:
            percentage = 100.00
        print(f"Current Percentage {percentage}%")
        trainData = {"Accelerometor_Data": [accelArrDict[0], accelArrDict[1]], "Action": action}
        dictArr.append(trainData)
        thingy1bool = False
        thingy2bool = False
        if todash:
            #start dashboard upload thread
            uploadThread = threading.Thread(
                target=upload_to_dashboard, args=(accelArrDict[0], accelArrDict[1], action[0]))
            uploadThread.start()


async def run():
    global start
    scanner = BleakScanner(detection_callback)
    start = time.time()
    await scanner.start()
    await asyncio.sleep(float(duration))
    await scanner.stop()
    end = time.time()

loop = asyncio.get_event_loop()
loop.run_until_complete(run())
write_data()
playsound('bell.wav')
print("Data has been successfully written to the file")


#Note for setup:

#"thingy" - leg, right pocket, light outwards. Sams top left.
#"thingy1" - arm, light outwards top left
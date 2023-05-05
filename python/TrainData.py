import serial
import sys
import csv
import time
import numpy as np
import tago

#number of samples
samples = sys.argv[1]

#upload to dash?
fast = sys.argv[2]

#training data key
mydevice = tago.Device('6a647e83-6a79-4b60-8b56-d17c32f75c55')

#node replacement dict
nodeDict = {
    '0': 'Null',
    '1': 'NodeA',
    '2': 'NodeB',
    '3': 'NodeC',
    '4': 'NodeD',
    '5': 'NodeE',
    '6': 'NodeF',
    '7': 'NodeG',
    '8': 'NodeH',
    '9': 'NodeI',
    '10': 'NodeJ',
    '11': 'NodeK',
    '12': 'NodeL'
}

# upload data in a specific string
def upload_to_dashboard(rssi,nodes,position):
    
    dictArr = []
    data = {
        'variable': f"rssi_data",
        'value': str(rssi + "$" + nodes),
    }
    dictArr.append(data)
    data = {
        'variable': "position",
        'value': position,
    }
    dictArr.append(data)
    result = mydevice.insert(dictArr)

#order rssi value
def get_rssi(rssi,nodes):
    rssi_out = []
    for element in list(nodes.split(',')):
        if int(element) != 0:
            rssi_out.append(rssi[int(element)-1])
    return rssi_out

#replace node number with labels
def replace_toString(nodes):
    strings = []
    for i in nodes.split(','):
        strings.append(nodeDict[str(i)])
    return strings


def read_serial(ser):
    row = []
    result = b''
    prompt = b'CSSE4011:~$'
    while not result.endswith(prompt):
        byte = ser.read()
        if not byte:
            break
        result += byte
    result = result[:-len(prompt)]
    result = result.replace(b'\r\n', b'\n')
    result = result.split(b'^')

    if len(result) > 1:
        values = result[1].split(b'~')
        output = values[0].decode('ascii')
        nodes = values[1].decode('ascii')
        output.split(',')
        nodes.split(',')
        values = nodes.split('#')[0]

        return output, values
    



try:
    ser = serial.Serial(port="/dev/ttyACM0", baudrate=115200, bytesize=8, timeout=2, stopbits=serial.STOPBITS_ONE)

except Exception as e: 
    print(e)
    print("Cannot Connnect")


row = []
dataset = []
count = 0
start = time.time()
notdone = 1
while(notdone):
    print("Enter X:")
    x = input()
    print("Enter Y:")
    y = input()
    while(count < int(samples)):
        try:
            result, nodes = read_serial(ser)
            if (result != None) and (nodes != None):
                #valid data
                rssi = result.split(',')
                rssi_new = get_rssi(rssi, nodes)
                nodes = replace_toString(nodes)
                if fast == 'n':
                    #if here, uploads to dash
                    upload_to_dashboard(str(rssi_new), str(nodes),str([x,y]))

                row.append(rssi_new)
                row.append([x,y])
                dataset.append(row)
                row = []
                count = count + 1
            
                if np.mod(count,50) == 0:
                    print(count)
                    #sanity print
                        
        except TypeError:
            pass
      
    print("Done Training?")
    donetrain = input()
    if donetrain == 'y':
        notdone = 0
    count = 0
    
header = []
header.append("RSSI")
header.append("Position")


with open(f'TrainingData1232.csv', 'w', encoding='UTF8', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(header)
        writer.writerows(dataset)
end = time.time()
print(end-start)
print("Done")
import serial
import sys
import csv
import time
import numpy as np
import tago

#tago device
mydevice = tago.Device('3b706237-7aff-4dd7-b1a9-028158f4ac19')

#upload rssi and node data
def upload_to_dashboard(rssi,nodes):
    
    dictArr = []
    data = {
        'variable': f"rssi_data",
        'value': str(rssi + "$" + nodes),
    }
    dictArr.append(data)
    result = mydevice.insert(dictArr)


#read uart
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
    
#dictionary for string replacements
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

#replaces node numbers with node labels
# i.e 1 -> nodeA
def replace_toString(nodes):
    strings = []
    for i in nodes.split(','):
        strings.append(nodeDict[str(i)])
    return strings
        
#sorts the ordering of the rssi array
def get_rssi(rssi,nodes):
    rssi_out = []
    for element in list(nodes.split(',')):
        if int(element) != 0:
            rssi_out.append(rssi[int(element)-1])
    return rssi_out


try:
    ser = serial.Serial(port="/dev/ttyACM0", baudrate=115200, bytesize=8, timeout=2, stopbits=serial.STOPBITS_ONE)

except Exception as e: 
    print(e)
    print("Cannot Connnect")

row = []
dataset = []
count = 0
start = time.time()
while(1):
    try:
        result, nodes = read_serial(ser)
        if (result != None) and (nodes != None):
            rssi = result.split(',')
            rssi_new = get_rssi(rssi, nodes)
            nodes = replace_toString(nodes)
            print(rssi_new)

            upload_to_dashboard(str(rssi_new), str(nodes))
            
    except TypeError:
        pass

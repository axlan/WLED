import time
import math

import requests

globe_url = 'http://192.168.1.100'

start_yaw = None
start_pitch = None
start_roll = None

def normalize(deg):
    if deg < 180:
        deg += 360
    if deg > 180:
        deg -= 360
    return deg

while True:
    r = requests.get(globe_url + '/json')
    print(r.json()['info'])
    imu = r.json()['info']['u']['IMU']

    orient = imu['Orientation']
    yaw = math.degrees(orient[0])
    pitch = math.degrees(orient[1])
    roll = math.degrees(orient[2])

    if start_yaw is None:
        start_yaw = yaw
        start_pitch = pitch
        start_roll = roll

    yaw -= start_yaw
    yaw = normalize(yaw)
    pitch -= start_pitch
    pitch = normalize(pitch)
    roll -= start_roll
    roll = normalize(roll)

    grav = imu['Gravity']
    gravX = grav[0]
    gravY = grav[1]
    gravZ = grav[2]

    print(f'{yaw:7.2f}, {pitch:7.2f}, {roll:7.2f}')
    print(f'{gravX:7.4f}, {gravY:7.4f}, {gravZ:7.4f}')
    print()

    time.sleep(0.5)

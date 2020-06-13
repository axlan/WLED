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
    imu = r.json()['info']['imu']

    yaw = math.degrees(imu['yaw'])
    pitch = math.degrees(imu['pitch'])
    roll = math.degrees(imu['roll'])

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

    gravX = imu['gravx']
    gravY = imu['gravy']
    gravZ = imu['gravz']

    print(f'{yaw:7.2f}, {pitch:7.2f}, {roll:7.2f}')
    print(f'{gravX:7.4f}, {gravY:7.4f}, {gravZ:7.4f}')
    print()

    time.sleep(0.5)

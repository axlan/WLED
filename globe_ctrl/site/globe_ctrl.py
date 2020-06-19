
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

import time
import math

import requests

globe_url = 'http://192.168.1.100'

df = pd.read_csv('globe_ctrl/centers.csv')

GYRO_THRESH = 10

centers = {}

for row in df.iterrows():
    row = row[1]
    center = np.array([row['x'], row['y'], row['z']])
    centers[row['face']] = center
    
def get_cmd():
    r = requests.get(globe_url + '/json')
    imu = r.json()['info']['imu']

    gravX = imu['gravx']
    gravY = imu['gravy']
    gravZ = imu['gravz']

    best_dist = 100
    for face, center in centers.items():
        grav_vector = np.array([gravX, gravY, gravZ])
        distance = np.linalg.norm(grav_vector - center)
        if distance < best_dist:
            best_face = face
            best_dist = distance
        print(f'{face}: {distance:7.4f}')

    gyro_mag = math.sqrt(imu['gyrox']**2 + imu['gyroy']**2 + imu['gyroz']**2)
    print(f'gyro_mag:{gyro_mag}')

    return best_face, gyro_mag > GYRO_THRESH



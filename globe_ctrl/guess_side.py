
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

import time
import math

import requests

globe_url = 'http://192.168.1.100'

df = pd.read_csv('globe_ctrl/centers.csv')

centers = {}

for row in df.iterrows():
    row = row[1]
    center = np.array([row['x'], row['y'], row['z']])
    centers[row['face']] = center
    


while True:
    r = requests.get(globe_url + '/json')
    imu = r.json()['info']['u']['IMU']

    grav = imu['Gravity']

    print(f'{grav[0]:7.4f}, {grav[1]:7.4f}, {grav[2]:7.4f}')

    best_dist = 100
    for face, center in centers.items():
        grav_vector = np.array(grav)
        distance = np.linalg.norm(grav_vector - center)
        if distance < best_dist:
            best_face = face
            best_dist = distance
        print(f'{face}: {distance:7.4f}')
        
    print(best_face)

    time.sleep(1)



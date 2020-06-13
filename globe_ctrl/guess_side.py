
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

import time
import math

import requests

globe_url = 'http://192.168.1.100'

df = pd.read_csv('centers.csv')

centers = {}

for row in df.iterrows():
    row = row[1]
    center = np.array([row['x'], row['y'], row['z']])
    centers[row['face']] = center
    


while True:
    r = requests.get(globe_url + '/json')
    imu = r.json()['info']['imu']

    gravX = imu['gravx']
    gravY = imu['gravy']
    gravZ = imu['gravz']

    print(f'{gravX:7.4f}, {gravY:7.4f}, {gravZ:7.4f}')

    best_dist = 100
    for face, center in centers.items():
        grav_vector = np.array([gravX, gravY, gravZ])
        distance = np.linalg.norm(grav_vector - center)
        if distance < best_dist:
            best_face = face
            best_dist = distance
        print(f'{face}: {distance:7.4f}')
        
    print(best_face)

    time.sleep(1)



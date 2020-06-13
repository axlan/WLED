
import requests
import pandas as pd
import matplotlib.pyplot as plt

from collections import defaultdict
import time

globe_url = 'http://192.168.1.100'

faces = [
    'SF',
    'NY',
    'ES',
    'EU',
    'VN',
    'NZ',
    'TA'
]

axis = ['x', 'y', 'z']

start_time = time.time()

with open('grav.csv', 'w') as fd:
    fd.write('time(s),face,x,y,z\n')
    for face in faces:
        input(f"Show Face: {face}")
        # data = defaultdict(list)
        for i in range(100):
            r = requests.get(globe_url + '/json')
            imu = r.json()['info']['imu']
            timestamp = time.time() - start_time
            #for a in axis:
            #    data[a].append(imu[f'grav{a}'])
            fd.write(f'{timestamp},{face},{imu["gravx"]},{imu["gravy"]},{imu["gravz"]}\n')
            time.sleep(0.2)
            print(f'\r{i}/{150}')
        # df = pd.DataFrame(data)
        # print(df.describe())
        # plt.plot(df['x'],'x-')
        # plt.plot(df['y'],'x-')
        # plt.plot(df['z'],'x-')
        # plt.show()
        # pass

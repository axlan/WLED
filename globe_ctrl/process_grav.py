
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

df = pd.read_csv('grav.csv')

fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')


faces = [
    'SF',
    'NY',
    'ES',
    'EU',
    'VN',
    'NZ',
    'TA'
]

centers = {}
radius = 0.3

prop_cycle = plt.rcParams['axes.prop_cycle']
colors = prop_cycle.by_key()['color']

with open('centers.csv', 'w') as fd:
    fd.write('face,x,y,z\n')
    for i, face in enumerate(faces):
        face_df = df[df['face'] == face]

        center = face_df.mean().to_dict()
        fd.write(f'{face},{center["x"]},{center["y"]},{center["z"]}\n')

        centers[face] = center
        ax.scatter(face_df['x'], face_df['y'], face_df['z'])

        # draw sphere
        u, v = np.mgrid[0:2*np.pi:10j, 0:np.pi:5j]
        x = np.cos(u)*np.sin(v) * radius + center['x']
        y = np.sin(u)*np.sin(v) * radius + center['y']
        z = np.cos(v) * radius + center['z']
        ax.plot_wireframe(x, y, z, color=colors[i])



plt.show()


import os
import json
import math

from flask import Flask, render_template, Response, redirect, request

from led_ctrl import highlight
from globe_ctrl import get_cmd

app = Flask(__name__)

with open('globe_ctrl/cities.json') as fd:
  city_leds = json.load(fd)

selected = {'idx': 0, 'step': 0, 'face': '', "locked": False}

def haversine_distance(mk1, mk2):
    R = 3958.8 # Radius of the Earth in miles To use kilometers, set R = 6371.0710
    rlat1 = math.radians(float(mk1['lat'])) # Convert degrees to radians
    rlat2 = math.radians(float(mk2['lat'])) # Convert degrees to radians
    difflat = rlat2-rlat1 # Radian difference (latitudes)
    difflon = math.radians(float(mk2['lng'])-float(mk1['lng'])) # Radian difference (longitudes)
    d = 2.0 * R * math.asin(math.sqrt(math.sin(difflat/2)*math.sin(difflat/2)+math.cos(rlat1)*math.cos(rlat2)*math.sin(difflon/2)*math.sin(difflon/2)))
    return d

@app.route('/get_map_params')
def get_map_params():
    face, move = get_cmd()

    selected['locked'] = not move and selected['face'] == face

    if not selected['locked']:
        face_steps = []
        for i, city in enumerate(city_leds):
            if face == city['face']:
                face_steps += [ (i, step) for step in city['steps'] ]
        face_steps = sorted(face_steps, key=lambda x: x[1])
        

        if selected['face'] != face:
            selected['step'] = 0
            selected['face'] = face
        elif move:
            selected['step'] += 1
            selected['step'] %= len(face_steps)
        selected['idx'] = face_steps[selected['step']][0]
        print(face_steps)
        print(selected)
        highlight(selected['idx'])
    return Response(json.dumps(city_leds[selected['idx']]), mimetype='application/json')

@app.route('/set_center', methods=['POST'])
def set_center():
    print(request.form)
    closest_dist = 10**10
    for i, city in enumerate(city_leds):
        d = haversine_distance(request.form, city)
        if d < closest_dist:
            closest_dist = d
            selected['idx'] = i
    selected['locked'] = True
    highlight(selected['idx'])
    return Response(status=200)

@app.route('/')
def hello():
    return redirect('/static/index.html')

if __name__ == '__main__':
    app.run(debug=True)

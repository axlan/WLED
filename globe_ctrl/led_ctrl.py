

import requests
import time
import json
from typing import List

globe_url = 'http://192.168.1.100'

with open('globe_ctrl/cities.json') as fd:
  city_leds = json.load(fd)

num_steps = 0
for city in city_leds:
  num_steps = max([num_steps] + city['steps'])
num_steps += 1

newHeaders = {'Content-type': 'application/json', 'Accept': 'text/plain'}

text_data = '''{{
    "seg": [
      {{
        "id": 0,
        "start": {},
        "len": 1,
        "col": [
          [
            255,
            0,
            17
          ],
          [
            0,
            0,
            0
          ],
          [
            0,
            0,
            0
          ]
        ],
        "fx": 0,
        "sx": 60,
        "ix": 128,
        "pal": 0,
        "sel": false,
        "rev": false
      }}
    ]
 }}'''



def single_light(led_id):
    r = requests.post(globe_url + '/json', data=text_data.format(led_id), headers=newHeaders)
    print(r.status_code)
    print(r.text)

def cycle_all():
    while True:
        for i in range(num_steps):
            single_light(i)
            time.sleep(.5)

def highlight(target):
    data = {'seg': [
        {
            "id": 0,
            "start": 0,
            "stop": target,
            "col": [[0, 0,  255], [0, 0, 0], [0, 0, 0]],
            'fx': 15,
            # Relative effect speed
            'sx': 128,
            # Effect intensity
            'ix': 128,
            "pal": 0,
        },
        {
            "id": 2,
            "start": target,
            "stop": target + 1,
            "col": [[255, 0,  0], [0, 0, 0], [0, 0, 0]],
            'fx': 0,
            # Relative effect speed
            'sx': 128,
            # Effect intensity
            'ix': 128,
            "pal": 0,
        },
        {
            "id": 1,
            "start": target + 1,
            "stop": num_steps,
            "col": [[0, 0,  255], [0, 0, 0], [0, 0, 0]],
            'fx': 15,
            # Relative effect speed
            'sx': 128,
            # Effect intensity
            'ix': 128,
            "pal": 0
        }
    ]}
    r = requests.post(globe_url + '/json', data=json.dumps(data), headers=newHeaders, timeout=1)
    print(r.status_code, r.text)

def free_run():
    data = {'seg': [
        {
        "id": 0,
        "start": 0,
        "len": num_steps,
        "col": [[255, 0,  0], [0, 0, 0], [0, 0, 0]],
        'fx': 30,
        'sx': 20,
        'ix': 0,
        "pal": 9,
        },
        {
        "id": 1,
        "start": 0,
        "stop": 0,
        },
        {
        "id": 2,
        "start": 0,
        "stop": 0,
        }]}
    r = requests.post(globe_url + '/json', data=json.dumps(data), headers=newHeaders, timeout=1)
    print(r.status_code, r.text)
#cycle_all()
highlight(16)
#free_run()

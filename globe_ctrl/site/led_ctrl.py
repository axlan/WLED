
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


def find_step(step_idx):
    for i, val in enumerate(city_leds):
        if step_idx in val['steps']:
            return i

led_path = [ find_step(i) for i in range(num_steps + 1) ]

def cycle_all():
    while True:
        for i in led_path:
            single_light(i)
            time.sleep(.5)

def highlight(target):
  data = {'seg': []}
  if target != 0:
    data['seg'].append({
      "id": 0,
      "start": 0,
      "len": target,
      "col": [[0, 0,  255], [0, 0, 0], [0, 0, 0]],
      'fx': 15,
      # Relative effect speed
      'sx': 128,
      # Effect intensity
      'ix': 128,
    })
  data['seg'].append({
      "id": 1,
      "start": target,
      "len": 1,
      "col": [[255, 0,  0], [0, 0, 0], [0, 0, 0]],
      'fx': 0,
      # Relative effect speed
      'sx': 128,
      # Effect intensity
      'ix': 128,
    })
  if target != len(city_leds) - 1:
    data['seg'].append({
      "id": 2,
      "start": target + 1,
      "len": len(city_leds) - target,
      "col": [[0, 0,  255], [0, 0, 0], [0, 0, 0]],
      'fx': 15,
      # Relative effect speed
      'sx': 128,
      # Effect intensity
      'ix': 128,
    })
    r = requests.post(globe_url + '/json', data=json.dumps(data), headers=newHeaders, timeout=1)
    print(r.status_code, r.text)


# cycle_all()

#highlight(13)


import requests
import time

globe_url = 'http://192.168.1.100'

city_leds = [
    ('new york', 'NY', [3]),
    ('vancouver', 'SF', [1]),
    ('san francisco', 'SF', [0]),
    ('tahiti', 'TA', [32]),
    ('aukland', 'NZ', [29,31]),
    ('nelson', 'NZ', [30]),
    ('cat ba', 'VN', [20]),
    ('hanoi', 'VN', [19,21,23]),
    ('sa pa', 'VN', [22]),
    ('chiang mai', 'VN', [24]),
    ('bangkok', 'VN', [18,25,27]),
    ('ko tao', 'VN', [26]),
    ('singapore', 'VN', [28]),
    ('greece', 'EU', [16]),
    ('prague', 'EU', [7]),
    ('berlin', 'EU', [6]),
    ('munich', 'EU', [8]),
    ('geneva', 'EU', [9]),
    ('amsterdam', 'EU', [5]),
    ('paris', 'ES', [4, 10]),
    ('largos', 'ES', [13]),
    ('seville', 'ES', [12, 14]),
    ('barcelona', 'EU', [11, 15]),
    ('naples', 'EU', [17]),
]


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
        if step_idx in val[2]:
            return i

def cycle_all():
    while True:
        for i in range(33):
            single_light(find_step(i))
            time.sleep(.5)
            

cycle_all()
#!/usr/bin/env python
import json
import time

# Dependency installed with `pip install paho-mqtt`.
# https://pypi.org/project/paho-mqtt/
import paho.mqtt.client as mqtt


# SET THE SERVER BELOW TO THE IP OF THE MQTT BROKER TO USE!
MQTT_SERVER_ADDR = '192.168.1.110'

state = {'label': ''}

csv_fd = open('log_file.csv', 'a')

# Define MQTT callbacks
def on_connect(client, userdata, connect_flags, reason_code, properties):
    print("Connected with result code "+str(reason_code))
    state['start_time'] = None
    client.subscribe("wled/e5a658/dice/#")

def on_message(client, userdata, msg):
    if msg.topic.endswith('roll_label'):
        state['label'] = msg.payload.decode('ascii')
        print(f"Label set to {state['label']}")
    elif msg.topic.endswith('roll'):
      json_str = msg.payload.decode('ascii')
      msg_data = json.loads(json_str)
      # Convert the relative timestamps reported to the dice to an approximate absolute time.
      # The "last_time" check is to detect if the ESP32 was restarted or the counter rolled over.
      if state['start_time'] is None or msg_data['time'] < state['last_time']:
          state['start_time'] = time.time() - (msg_data['time'] / 1000.)
      state['last_time'] = msg_data['time']
      timestamp = state['start_time'] + (msg_data['time'] / 1000.)
      csv_fd.write(f"{timestamp:.3f}, {msg_data['name']}, {state['label']}, {msg_data['state']}, {msg_data['val']}\n")
      csv_fd.flush()
      if msg_data['state'] == 1:
          print(f"{timestamp:.3f}: {msg_data['name']} rolled {msg_data['val']}")

# Create an MQTT client
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

# Set MQTT callbacks
client.on_connect = on_connect
client.on_message = on_message

# Connect to the MQTT broker
client.connect(MQTT_SERVER_ADDR, 1883, 60)

while client.loop(timeout=1.0) == mqtt.MQTT_ERR_SUCCESS:
    time.sleep(.1)

print('Connection Failure')

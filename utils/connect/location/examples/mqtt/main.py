# https://www.thethingsnetwork.org/forum/t/a-python-program-to-listen-to-your-devices-with-mqtt/9036/6
# Get data from MQTT server
# Run this with python 3, install paho.mqtt prior to use

import paho.mqtt.client as mqtt
import json

APPEUI = '70B3D57ED0026221'
APPID = 'location_0'
PSW = 'ttn-account-v2.CsS9fzfyLpt0kAG-d3Fyvb2pPBS38Za38-WY92MVXN4'


def on_connect(mqttc, mosq, obj,rc):
    print('INFO: Connected with result code: {}'.format(rc))
    if not rc == 0:
        print("ERROR: Could not connect to TTN")
    # subscribe for all devices of user
    mqttc.subscribe('+/devices/+/up')


def on_message(mqttc,obj,msg):
    data_json = json.loads(msg.payload)
    print(data_json)

mqttc= mqtt.Client()
# Assign event callbacks
mqttc.on_connect=on_connect
mqttc.on_message=on_message

mqttc.username_pw_set(APPID, PSW)
mqttc.connect("eu.thethings.network",1883,60)

while True:
    mqttc.loop()

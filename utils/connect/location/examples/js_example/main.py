from flask import jsonify, render_template, request, Flask
import paho.mqtt.client as mqtt
import json

# MQTT variables
APPEUI = '70B3D57ED0026221'
APPID = 'location_0'
PSW = 'ttn-account-v2.CsS9fzfyLpt0kAG-d3Fyvb2pPBS38Za38-WY92MVXN4'
mqttc= mqtt.Client()

# Flask variables
app = Flask(__name__)
app.config["TEMPLATES_AUTO_RELOAD"] = True

@app.route("/", defaults={"js": "base"})
@app.route("/<any(base):js>")
def index(js):
    return render_template("{0}.html".format(js), js=js)

def on_connect(mqttc, mosq, obj,rc):
    print('INFO: Connected with result code: {}'.format(rc))
    if not rc == 0:
        print("ERROR: Could not connect to TTN")
    # subscribe for all devices of user
    mqttc.subscribe('+/devices/+/up')


def on_message(mqttc,obj,msg):
    data_json = json.loads(msg.payload)
    print(data_json)

def setup_mqtt():
    # Assign event callbacks
    mqttc.on_connect=on_connect
    mqttc.on_message=on_message

    mqttc.username_pw_set(APPID, PSW)
    mqttc.connect("eu.thethings.network",1883,60)

def loop_mqtt():
    mqttc.loop()

if __name__ == '__main__':
    app.run()


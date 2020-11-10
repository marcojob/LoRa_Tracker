from flask import jsonify, render_template, request, Flask
import paho.mqtt.client as mqtt
import json
import threading
import base64

MAX_STATIONS = 10
MAX_POSITIONS = 100
GPS_ACCURACY = 5

# data_json
data_json = {}
station_json = {}
position_json = {}

# data json file
data_file = 'data/data.json'

# MQTT variables
APPEUI = '70B3D57ED0026221'
APPID = 'location_0'
PSW = 'ttn-account-v2.CsS9fzfyLpt0kAG-d3Fyvb2pPBS38Za38-WY92MVXN4'
mqttc = mqtt.Client()

# Flask variables
app = Flask(__name__)
app.config["TEMPLATES_AUTO_RELOAD"] = True


@app.route("/", defaults={"js": "base"})
@app.route("/<any(base):js>")
def index(js):
    return render_template("{0}.html".format(js), data_json=data_json)


def on_connect(mqttc, mosq, obj, rc):
    print('INFO: Connected with result code: {}'.format(rc))
    if not rc == 0:
        print("ERROR: Could not connect to TTN")
    # subscribe for all devices of user
    mqttc.subscribe('+/devices/+/up')


def on_message(mqttc, obj, msg):
    payload_json = json.loads(msg.payload)
    print("INFO: Received new data")
    parse_data(payload_json)


def parse_data(payload_json):
    global data_json
    # Parse station data
    for station in payload_json['metadata']['gateways']:
        lat = station['latitude']
        lng = station['longitude']
        if not pos_in_list(data_json['stations'], lat, lng):
            data_json['stations'].append({'latitude': lat, 'longitude': lng})
            if len(data_json['stations']) > MAX_STATIONS:
                del data_json['stations'][0]
    # Parse GPS data
    msg_bytes = base64.b64decode(payload_json['payload_raw'])
    pos_lat = msg_bytes[0] & int('01111111', 2)
    pos_lat = pos_lat << 8
    pos_lat = pos_lat | msg_bytes[1]
    pos_lat = pos_lat << 8
    pos_lat = pos_lat | msg_bytes[2]
    pos_lat = pos_lat << 8
    pos_lat = pos_lat | msg_bytes[3]
    pos_lat = pos_lat >> 6
    sign = msg_bytes[0] >> 7
    if sign:
        pos_lat -= pos_lat
    pos_lat /= 10**GPS_ACCURACY

    pos_lng = msg_bytes[3] & int('00011111', 2)
    pos_lng = pos_lng << 8
    pos_lng = pos_lng | msg_bytes[4]
    pos_lng = pos_lng << 8
    pos_lng = pos_lng | msg_bytes[5]
    pos_lng = pos_lng << 8
    pos_lng = pos_lng | msg_bytes[6]
    pos_lng = pos_lng >> 4

    sign = msg_bytes[3] & int('00100000', 2)
    sign = sign >> 5
    if sign:
        pos_lng -= pos_lng
    pos_lng /= 10**GPS_ACCURACY

    data_json['positions'].append({'latitude': pos_lat, 'longitude': pos_lng})
    if len(data_json['positions']) > MAX_POSITIONS:
        del data_json['positions'][0]

    data_json['soc'] = msg_bytes[6] & int('1111',2)

    data_json['time'] = payload_json['metadata']['time']
    with open(data_file, 'w') as f:
        json.dump(data_json, f)

def pos_in_list(data_list, lat, lng):
    for pos in data_list:
        if pos['latitude'] == lat and pos['longitude'] == lng:
            return True
    return False


class Mqtt_Thread(threading.Thread):
    def __init__(self, threadID, name, counter):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.counter = counter

    def run(self):
        try:
            print('INFO: Starting mqtt thread')
            mqttc.on_connect = on_connect
            mqttc.on_message = on_message

            mqttc.username_pw_set(APPID, PSW)
            mqttc.connect("eu.thethings.network", 1883, 60)

            with open(data_file, 'r') as f:
                global data_json
                data_json = json.load(f)
            while True:
                mqttc.loop()
        except Exception as e:
            print('ERROR: {}'.format(e))


if __name__ == '__main__':
    mqtt_thread = Mqtt_Thread(1, "Thread-1", 1)
    mqtt_thread.start()

    app.run(host='0.0.0.0')

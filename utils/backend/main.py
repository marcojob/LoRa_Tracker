import logging
import json
import threading
import os
import paho.mqtt.client as mqtt
import base64

from time import sleep
from random import random

from telegram.ext import Updater, CommandHandler, MessageHandler, Filters

# Enable logging
logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    level=logging.INFO)

logger = logging.getLogger(__name__)

IDS_JSON_FILE = 'ids.json'
CONFIG_JSON_FILE = 'config.json'
MAX_LIVE_PERIOD = 86400  # 24 hours
PAYLOAD_BYTES = 9  # <Latitude: 4 bytes><Longitude: 4 bytes><SOC: 1 byte>


class Telegram_Backend():
    def __init__(self):
        # Variables
        self.config = {"token": None, "users": []}
        self.last_soc = 0

        # Load config
        if os.path.isfile(CONFIG_JSON_FILE):
            with open(CONFIG_JSON_FILE, 'r') as file:
                self.config = json.load(file)
        else:
            logger.error(
                "Config file not found, created empty config. Fill it.")
            with open(CONFIG_JSON_FILE, 'w') as file:
                json.dump(self.config, file)
            exit()

        # Template for all chat IDs
        self.id_template = {"chat_id": None, "msg_id": None,
                            "bat_id": None, "tracker_id": None}

        chat_ids = {u: self.id_template for u in self.config["users"]}

        # Load chat ids
        if os.path.isfile(IDS_JSON_FILE):
            with open(IDS_JSON_FILE, 'r') as file:
                self.chat_ids = json.load(file)

                # Check if we added any users
                for user in chat_ids.keys():
                    if not user in self.chat_ids.keys():
                        self.chat_ids[user] = self.id_template
                        logger.info('Added user {}'.format(user))

                # Check if we removed any users
                users_to_delete = list()
                for user in self.chat_ids.keys():
                    if not user in chat_ids.keys():
                        users_to_delete.append(user)

                for user in users_to_delete:
                    del self.chat_ids[user]
                    logger.info('Removed user {}'.format(user))

            # Dump chat ids
            with open(IDS_JSON_FILE, 'w') as file:
                json.dump(self.chat_ids, file)
        else:
            self.chat_ids = chat_ids

        # Create the Updater and pass it your bot's token.
        self.updater = Updater(self.config["token"], use_context=True)

        # Bot
        self.bot = self.updater.bot

        # Get the dispatcher to register handlers
        dp = self.updater.dispatcher

        # on different commands - answer in Telegram
        dp.add_handler(CommandHandler("start", self.start_command))
        dp.add_handler(CommandHandler("add_user", self.add_user))
        dp.add_handler(CommandHandler("rm_user", self.rm_user))

        # Start polling, non-blocking!
        self.updater.start_polling()

    def start_command(self, update, context):
        """Send a message when the command /start is issued."""

        # Extract user id and username
        chat_id = update.message.chat.id
        username = update.message.chat.username
        msg = update.message.text.split(" ")

        # User not allowed
        if not username in self.config["users"]:
            update.message.reply_text(
                'Unknown user, please contact maintainer')
            return

        if not len(msg) == 2:
            update.message.reply_text(
                'Please specify your tracker ID with /start <ID>')
            return

        # Tracker ID
        tracker_id = msg[1]

        # Get ids
        self.chat_ids[username]["chat_id"] = chat_id
        self.chat_ids[username]["msg_id"] = None
        self.chat_ids[username]["bat_id"] = None
        self.chat_ids[username]["tracker_id"] = tracker_id

        # Dump chat ids
        with open(IDS_JSON_FILE, 'w') as file:
            json.dump(self.chat_ids, file)

        # Success message
        update.message.reply_text(
            'Started LoRa_Tracker application for user {} with tracker ID {}'.format(username, tracker_id))

    def add_user(self, update, context):
        """Add a new user"""

        # Check if user is allowed to use it
        username = update.message.chat.username
        if not username == self.config["users"][0]:
            logger.warning("User {} cannot add users".format(username))
            return

        # Find command argument
        command_txt = '/add_user'
        command_msg = update.message.text.split(" ")
        del command_msg[0]

        if not len(command_msg) == 1:
            logger.warning("Add user cmd wrong format")
            return

        # Append to allowed users
        new_user = command_msg[0]
        if not new_user in self.config["users"]:
            self.config["users"].append(new_user)
            self.chat_ids[new_user] = self.id_template
            logger.info("Added user {}".format(new_user))

            # Write it
            with open(CONFIG_JSON_FILE, 'w') as file:
                json.dump(self.config, file)

    def rm_user(self, update, context):
        """Remove a user"""

        # Check if user is allowed to use it
        username = update.message.chat.username
        if not username == self.config["users"][0]:
            logger.warning("User {} cannot remove users".format(username))
            return

        # Find command argument
        command_txt = '/rm_user'
        command_msg = update.message.text.split(" ")
        del command_msg[0]

        if not len(command_msg) == 1:
            logger.warning("Remove user cmd wrong format")
            return

        # Delete from allowed users
        rm_user = command_msg[0]
        if rm_user in self.config["users"]:
            self.config["users"].remove(rm_user)
            logger.info("Removed user {}".format(rm_user))

            # Write it
            with open(CONFIG_JSON_FILE, 'w') as file:
                json.dump(self.config, file)

            # Delete chat ids for this user
            if rm_user in self.chat_ids.keys():
                del self.chat_ids[rm_user]
                with open(IDS_JSON_FILE, 'w') as file:
                    json.dump(self.chat_ids, file)

    def send_live_location(self, username, lat, lon):
        """ Send a live gps position to given user"""
        chat_id = self.chat_ids[username]["chat_id"]

        if chat_id is None:
            return

        msg_id = self.chat_ids[username]["msg_id"]

        # If there is already an active live location we just edit it
        if not msg_id is None:
            try:
                ret = self.bot.editMessageLiveLocation(
                    chat_id, msg_id, latitude=lat, longitude=lon, disable_notification=True)
            except Exception as e:
                logger.warning('Could not edit message: {}'.format(e))
                ret = True

            # Returns true on failure
            if not ret is True:
                return

            # Delete the previous msg, in order to keep chat clean
            try:
                self.bot.delete_message(chat_id, msg_id)
            except Exception as e:
                logger.warning('Could not delete message: {}'.format(e))

        # Either live location is not valid or we did not have one active
        logger.info('Could not edit live location for {}'.format(username))
        m = self.bot.send_location(chat_id, latitude=lat, longitude=lon,
                                   live_period=MAX_LIVE_PERIOD, disable_notification=True)

        self.chat_ids[username]["msg_id"] = m.message_id

        # Dump chat ids
        with open(IDS_JSON_FILE, 'w') as file:
            json.dump(self.chat_ids, file)

    def send_soc(self, username, soc):
        """Send SOC message"""
        chat_id = self.chat_ids[username]["chat_id"]
        soc_msg = 'Bat: {}%'.format(soc)

        if chat_id is None:
            return

        bat_id = self.chat_ids[username]["bat_id"]

        # If there is already an active live location we just edit it
        if not bat_id is None:
            # If SOC is same we do not edit
            if soc == self.last_soc:
                return

            try:
                ret = self.bot.edit_message_text(
                    soc_msg, chat_id, bat_id)
            except Exception as e:
                logger.warning('Could not edit message: {}'.format(e))
                ret = True

            # Update last SOC
            self.last_soc = soc

            # Returns true on failure
            if not ret is True:
                return

            # Delete the previous msg, in order to keep chat clean
            try:
                self.bot.delete_message(chat_id, bat_id)
            except Exception as e:
                logger.warning('Could not delete message: {}'.format(e))

        # Either live location is not valid or we did not have one active
        logger.info('Could not edit battery message for {}'.format(username))
        m = self.bot.send_message(self.chat_ids[username]["chat_id"], soc_msg)

        # Update last SOC
        self.last_soc = soc

        self.chat_ids[username]["bat_id"] = m.message_id

        # Dump chat ids
        with open(IDS_JSON_FILE, 'w') as file:
            json.dump(self.chat_ids, file)


class MQTT_TTN():
    def __init__(self):
        # Telegram backend
        self.tb = Telegram_Backend()

        APPID = self.tb.config["APPID"]
        PSW = self.tb.config["PSW"]

        # MQTT Client
        self.mqttc = mqtt.Client()

        self.mqttc.on_connect = self.on_connect
        self.mqttc.on_message = self.on_message

        self.mqttc.username_pw_set(APPID, PSW)

    def start(self):
        self.mqttc.connect("eu.thethings.network", 1883, 60)

        while True:
            self.mqttc.loop()

    def on_connect(self, mqttc, mosq, obj, rc):
        logger.info('Connected with result code: {}'.format(rc))
        if not rc == 0:
            logger.error("ERROR: Could not connect to TTN")
            return

        # subscribe for all devices of user
        mqttc.subscribe('+/devices/+/up')

    def on_message(self, mqttc, obj, msg):
        payload_json = json.loads(msg.payload)
        msg_bytes = base64.b64decode(payload_json['payload_raw'])
        dev_id = payload_json['dev_id']
        logger.info('Received {} bytes'.format(len(msg_bytes)))

        # Check length of msg
        if not len(msg_bytes) == PAYLOAD_BYTES:
            logger.error('Invalid payload size: {}'.format(len(msg_bytes)))
            return

        # Otherwise parse the msg
        self.parse_payload(msg_bytes, dev_id)

    def parse_payload(self, msg, dev_id):
        # Assemble latitude
        lat = msg[0]
        lat = lat << 8 | msg[1]
        lat = lat << 8 | msg[2]
        lat = lat << 8 | msg[3]
        lat = self.twos_comp(lat, 32)
        lat /= 10**7

        # Assemble longitude
        lon = msg[4]
        lon = lon << 8 | msg[5]
        lon = lon << 8 | msg[6]
        lon = lon << 8 | msg[7]
        lon = self.twos_comp(lon, 32)
        lon /= 10**7

        # SOC in %
        soc = round(msg[8]/255*100.0, 2)

        for user in self.tb.chat_ids.keys():
            if self.tb.chat_ids[user]['tracker_id'] == dev_id:
                self.tb.send_live_location(user, lat, lon)
                self.tb.send_soc(user, soc)

    def twos_comp(self, val, bits):
        """compute the 2's complement of int value val"""
        if (val & (1 << (bits - 1))) != 0:
            val = val - (1 << bits)
        return val


def main():
    mqtt_ttn = MQTT_TTN()

    while True:
        try:
            # Blocking call, while loop makes sure to just restart
            mqtt_ttn.start()
        except Exception as e:
            logger.error('MQTT thread failed, restarting')

    # Test payloads
    # lat = {"0": 47.399978, "1": 40.749806}
    # lon = {"0": 8.546835, "1": -73.987806}
    # Payload 1
    # 1C40A9A4 051824BE 64

    # Payload 2
    # 1849ED4C D3E65B54 32


if __name__ == '__main__':
    main()

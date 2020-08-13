import logging
import json
import threading
import os

from time import sleep

from config import API_TOKEN, ALLOWED_USERS
from telegram.ext import Updater, CommandHandler, MessageHandler, Filters

# Enable logging
logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    level=logging.INFO)

logger = logging.getLogger(__name__)

IDS_JSON_FILE = 'ids.json'
MAX_LIVE_PERIOD = 86400


class Telegram_Backend():
    def __init__(self):
        # Variables
        id_template = {"chat_id": None, "msg_id": None, "tracker_id": None}
        chat_ids = {u: id_template for u in ALLOWED_USERS}

        # Load chat ids
        if os.path.isfile(IDS_JSON_FILE):
            with open(IDS_JSON_FILE, 'r') as file:
                self.chat_ids = json.load(file)

                # Check if we added any users
                for user in chat_ids.keys():
                    if not user in self.chat_ids.keys():
                        self.chat_ids[user] = id_template
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
        self.updater = Updater(API_TOKEN, use_context=True)

        # Bot
        self.bot = self.updater.bot

        # Get the dispatcher to register handlers
        dp = self.updater.dispatcher

        # on different commands - answer in Telegram
        dp.add_handler(CommandHandler("start", self.start_command))
        dp.add_handler(CommandHandler("help", self.help_command))

        # Filters
        filters = Filters.user(username=None)
        filters.add_usernames(ALLOWED_USERS)

        # Start polling, non-blocking!
        self.updater.start_polling()


    def start_command(self, update, context):
        """Send a message when the command /start is issued."""

        # Extract user id and username
        chat_id = update.message.chat.id
        username = update.message.chat.username

        if username in ALLOWED_USERS:
            self.chat_ids[username]["chat_id"] = chat_id
            self.chat_ids[username]["msg_id"] = None

            # Dump chat ids
            with open(IDS_JSON_FILE, 'w') as file:
                json.dump(self.chat_ids, file)

            # Success message
            update.message.reply_text(
                'Started LoRa_Tracker application for user {}'.format(username))
        else:
            # Failure message
            update.message.reply_text(
                'Unknown user, please contact maintainer')

    def help_command(self, update, context):
        """Send a message when the command /help is issued."""
        update.message.reply_text('Send /start to start application')

    def send_message(self, username, msg):
        """ Send a message to given user"""
        self.bot.sendMessage(self.chat_ids[username],"test")

    def send_live_location(self, username, lat, lon):
        """ Send a live gps position to given user"""
        chat_id = self.chat_ids[username]["chat_id"]

        if chat_id is None:
            return

        msg_id = self.chat_ids[username]["msg_id"]

        # If there is already an active live location we just edit it
        if not msg_id is None:
            try:
                ret = self.bot.editMessageLiveLocation(chat_id, msg_id, latitude=lat, longitude=lon, disable_notification=True)
            except Exception as e:
                logger.warning('Could not edit message: {}'.format(e))
                ret = True

            # Returns true on failure
            if not ret is True:
                return

        # Delete the previous msg, in order to keep chat clean
        self.bot.delete_message(chat_id, msg_id)

        # Either live location is not valid or we did not have one active
        logger.info('Could not edit live location for {}'.format(username))
        m = self.bot.send_location(chat_id, latitude=lat, longitude=lon, live_period=MAX_LIVE_PERIOD, disable_notification=True)
        self.chat_ids[username]["msg_id"] = m.message_id

        # Dump chat ids
        with open(IDS_JSON_FILE, 'w') as file:
            json.dump(self.chat_ids, file)


def main():
    # Backend class object,start
    tb = Telegram_Backend()

    # Temporary fixed location for testing
    lat = 47.399978
    lon = 8.546835
    while(True):
        sleep(3)
        lat += 0.0001
        lon += 0.0001
        tb.send_live_location(ALLOWED_USERS[0], lat, lon)


if __name__ == '__main__':
    main()

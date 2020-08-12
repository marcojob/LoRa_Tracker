import logging

from config import API_TOKEN, ALLOWED_USERS
from telegram.ext import Updater, CommandHandler, MessageHandler, Filters

# Enable logging
logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    level=logging.INFO)

logger = logging.getLogger(__name__)

chat_ids = {u: None for u in ALLOWED_USERS}


# Define a few command handlers. These usually take the two arguments update and
# context. Error handlers also receive the raised TelegramError object in error.
def start_command(update, context):
    """Send a message when the command /start is issued."""

    # Extract user id and username
    chat_id = update.message.chat.id
    username = update.message.chat.username

    if username in ALLOWED_USERS:
        chat_ids[username] = chat_id

        # Success message
        update.message.reply_text('Started LoRa_Tracker application for user {}'.format(username))
    else:
        # Failure message
        update.message.reply_text('Unknown user, please contact maintainer' )


def help_command(update, context):
    """Send a message when the command /help is issued."""
    update.message.reply_text('Send /start to start application')


def echo(update, context):
    """Echo the user message."""
    update.message.reply_text(update.message.text)


def main():
    """Start the bot."""
    # Create the Updater and pass it your bot's token.
    # Make sure to set use_context=True to use the new context based callbacks
    # Post version 12 this will no longer be necessary
    updater = Updater(API_TOKEN, use_context=True)

    # Get the dispatcher to register handlers
    dp = updater.dispatcher

    # on different commands - answer in Telegram
    dp.add_handler(CommandHandler("start", start_command))
    dp.add_handler(CommandHandler("help", help_command))

    # Filters
    filters = Filters.user(username=None)
    filters.add_usernames(ALLOWED_USERS)

    # on noncommand i.e message - echo the message on Telegram
    dp.add_handler(MessageHandler(filters, echo))

    # Loop
    # updater.bot.sendMessage(chat_id,"test")

    # Start the Bot
    updater.start_polling()

    # Run the bot until you press Ctrl-C or the process receives SIGINT,
    # SIGTERM or SIGABRT. This should be used most of the time, since
    # start_polling() is non-blocking and will stop the bot gracefully.
    updater.idle()


if __name__ == '__main__':
    main()
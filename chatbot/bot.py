# bot.py

import os  # for importing env vars for the bot to use
from twitchio.ext import commands
import boto3
import json
import logging

bot = commands.Bot(
    # set up the bot
    irc_token=os.environ['TMI_TOKEN'],
    client_id=os.environ['CLIENT_ID'],
    nick=os.environ['BOT_NICK'],
    prefix=os.environ['BOT_PREFIX'],
    initial_channels=[os.environ['CHANNEL']]
)

boto3.session.Session(profile_name='iot_playground')
client = boto3.client('iot-data')


frank_topic = "frank/action"

forward_tupe = ("forward", "forwards", "go")
backward_tupe = ("backward", "backwards", "back", "reverse", "retreat")
pen_tupe = ("pen", "marker", "texta", "sharpie")
up_tupe = ("up", "raise", "lift")
down_tupe = ("down", "lower", "drop", "mark")
lamp_tupe = ("lamp", "light", "led", "floodlight")


logger = logging.basicConfig(
    filename=".\\logs\\bot_log.txt", 
    level=logging.DEBUG
    )


def publishActionPayload(payload):
    resp = client.publish(
        topic=frank_topic,
        qos=0,
        payload=json.dumps(payload)
    )
    # if resp["ResponseMetadata"]["HTTPStatusCode"] != 200:
    #     logger.error(f"iot publish: '{resp}'")
    # else:
    #     logger.info(f"iot publish: '{resp}'")

# bot.py, below bot object
@bot.event
async def event_ready():
    'Called once when the bot goes online.'
    print(f"{os.environ['BOT_NICK']} is online!")
    resp = client.publish(
                topic=frank_topic,
                qos=0,
                payload="Client is online"
                )
    print(f"iot response: {resp}")
    ws = bot._ws  # this is only needed to send messages within event_ready
    await ws.send_privmsg(os.environ['CHANNEL'], "/me has landed!")


# bot.py, below event_ready
@bot.event
async def event_message(message):
    # Runs every time a message is sent in chat.
    # make sure the bot ignores itself and the streamer
    if message.author.name.lower() == os.environ['BOT_NICK'].lower():
        return

    slugs = message.content.lower().split(" ")

    cmd = slugs[1].lower()
    #logger.info(f"user: {message.author.name}, sent: {message.content}")

    if cmd == "left":
        # TODO check for angle!!!

        publishActionPayload({
            "action": "motion",
            "parameters": {
                "left_demand": -65536,
                "right_demand": 65536,
                "duration": 200
            }
        })

        await message.channel.send(
            f"Ok @{message.author.name}, turning left 90 degrees")

    elif cmd == "right":

        publishActionPayload({
            "action": "motion",
            "parameters": {
                "left_demand": 65536,
                "right_demand": -65536,
                "duration": 200
            }
        })

        await message.channel.send(
            f"Ok @{message.author.name}, turning right 90 degrees")

    elif cmd in forward_tupe:
        publishActionPayload({
            "action": "motion",
            "parameters": {
                "left_demand": 65536,
                "right_demand": 65536,
                "duration": 1000
            }
        })

        await message.channel.send(
            f"Ok @{message.author.name}, moving forward"
            )

    elif cmd in backward_tupe:
        publishActionPayload({
            "action": "motion",
            "parameters": {
                "left_demand": -65536,
                "right_demand": -65536,
                "duration": 1000
                }
            })

        await message.channel.send(
            f"Ok @{message.author.name}, moving backward"
            )

    elif cmd in pen_tupe:
        # TODO check length of slugs for action, avoid
        try:
            action = slugs[2].lower()
        except IndexError:
            action = ""

        if action in up_tupe:
            publishActionPayload({
                "action": "pen",
                "parameters": {
                    "position": 0,
                }
            })

            await message.channel.send(
                f"Ok @{message.author.name}, raising pen"
            )

        elif action in down_tupe:
            publishActionPayload({
                "action": "pen",
                "parameters": {
                    "position": 256,
                }
            })
                  
            await message.channel.send(
                f"Ok @{message.author.name}, lowering pen")

        else:
            await message.channel.send(
                f"Sorry @{message.author.name}, "
                "please tell me to raise or lower"
            )
    
    elif cmd in lamp_tupe:
       # TODO check length of slugs for action, avoid
        try:
            action = slugs[2].lower()
        except IndexError:
            action = ""

        if action == "on":
            publishActionPayload({
                "action": "lamp",
                "parameters": {
                    "state": 256,
                }
            })

            await message.channel.send(
                f"Ok @{message.author.name}, let there be light")

        elif action == "off":
            publishActionPayload({
                "action": "lamp",
                "parameters": {
                    "state": 0,
                }
            })

            await message.channel.send(
                f"@{message.author.name}, plunged the world into darkness")

        else:
            await message.channel.send(
                f"Sorry @{message.author.name}, "
                "please tell me if you want the lamp on or off")

    else:
        await message.channel.send(
            f"Sorry @{message.author.name}, i dont understand the command")


if __name__ == "__main__":
    bot.run()

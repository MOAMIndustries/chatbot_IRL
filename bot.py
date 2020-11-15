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


frank_topic = "/"
frank_action = {
    "action": "idle",
    "direction": None,
    "angle": 90,
    "duration": 0
}


forward_tupe = ("forward", "forwards", "go")
backward_tupe = ("backward", "backwards", "back", "reverse", "retreat")
pen_tupe = ("pen", "marker", "texta", "sharpie")
up_tupe = ("up", "raise", "lift")
down_tupe = ("down", "lower", "drop", "mark")


logger = logging.basicConfig(filename=".\\bot_log.txt", level=logging.DEBUG)

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
    'Runs every time a message is sent in chat.'

    # make sure the bot ignores itself and the streamer
    if message.author.name.lower() == os.environ['BOT_NICK'].lower():
        return

    # await message.channel.send(f"Hi, @{message.author.name}!")
    slugs = message.content.lower().split(" ")

    cmd = slugs[1]

    if cmd == "left":
        # TODO check for angle!!!
        
        frank_action['action'] = "move"
        frank_action['direction'] = "left"
        frank_action['angle'] = 90
        frank_action['duraction'] = -1
        
        resp = client.publish(
            topic=frank_topic,
            qos=0,
            payload=json.dumps(frank_action)
            )

        await message.channel.send(
            f"Ok @{message.author.name}, turning left 90 degrees"
            )

    elif cmd == "right":        
        frank_action['action'] = "move"
        frank_action['direction'] = "right"
        frank_action['angle'] = 90
        frank_action['duraction'] = -1
        
        resp = client.publish(
            topic=frank_topic,
            qos=0,
            payload=json.dumps(frank_action)
            )

        await message.channel.send(
            f"Ok @{message.author.name}, turning right 90 degrees"
            )

    elif cmd in forward_tupe:        
        frank_action['action'] = "move"
        frank_action['direction'] = "forward"
        frank_action['angle'] = 0
        frank_action['duraction'] = 1
        
        resp = client.publish(
            topic=frank_topic,
            qos=0,
            payload=json.dumps(frank_action)
            )
        await message.channel.send(
            f"Ok @{message.author.name}, moving forward"
            )

    elif cmd in backward_tupe:
        frank_action['action'] = "move"
        frank_action['direction'] = "forward"
        frank_action['angle'] = 0
        frank_action['duraction'] = 1
        
        resp = client.publish(
            topic=frank_topic,
            qos=0,
            payload=json.dumps(frank_action)
            )

        await message.channel.send(
            f"Ok @{message.author.name}, moving backward"
            )

    elif cmd in pen_tupe:
        # TODO check length of slugs for action, avoid
        try:
            action = slugs[2]
        except IndexError:
            action = ""

        if action in up_tupe:
            frank_action['action'] = "pen"
            frank_action['direction'] = "raise"
            frank_action['angle'] = 0
            frank_action['duraction'] = 1
            
            resp = client.publish(
                topic=frank_topic,
                qos=0,
                payload=json.dumps(frank_action)
                )

            await message.channel.send(
                f"Ok @{message.author.name}, raising pen"
            )

        elif action in down_tupe:
            frank_action['action'] = "pen"
            frank_action['direction'] = "lower"
            frank_action['angle'] = 0
            frank_action['duraction'] = 1
            
            resp = client.publish(
                topic=frank_topic,
                qos=0,
                payload=json.dumps(frank_action)
                )
                  
            await message.channel.send(
                f"Ok @{message.author.name}, lowering pen"
            )

        else:
            await message.channel.send(
                f"Sorry @{message.author.name}, "
                "please tell me to raise or lower"
            )

    else:
        await message.channel.send(
            f"Sorry @{message.author.name}, i dont understand the command"
            )



if __name__ == "__main__":
    bot.run()

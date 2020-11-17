# DaRoboto 
### The literal chat robot

This is a robot that can be controlled via twitch chat. It's going to have a bunch of commands for actions & eventually will be able to provide feedback as well

Twitch -> EC2 -> IoT Core -> Robot Thing

### Commands
| Command | Action |
|:--------|-------:|
|forward | Drives the robot forwards 1 second |
|left | turns left on the spot 90 degrees |
|right | tursn right on the spot 90 degrees |
|back | drives the robot backwards for 1 second |
|pen up | raises the pen from the paper |
|pen down | lowers the pen to the paper|

### Folder structure
`/chatbot` contains all code and dependancies for the chatbot agent running in ec2.

`/frank` contains code and platformio environment for the robot defined as frank

## Launching bot from cli
`pipenv run python bot.py`
#!/home/kpmealey/git/Greenthumb_Flow/server/server_venv/bin/python3

from celery import Celery, Task
from flask import Flask
from celery.schedules import crontab
import time
import json
from datetime import datetime
import os
import pandas as pd
import requests

celery = Celery('tasks', broker = 'redis://localhost:6379/0', backend = 'redis://localhost:6379/1')
celery.conf.timezone = 'America/New_York'

# This can be the weather getting
@celery.task
def retrieve_weather():
    forcast()
    print("Retrieved weather data!")

# sends watering control message to contoller -- can be used by manual override
@celery.task
def send_watering_msg(zone, solenoid_state, solenoid_on_duration):
    print(f'\{\"zone\":{zone}, \"solenoid state\":{solenoid_state}, \"solenoid on duration\":{solenoid_on_duration}\}')

# sends scheduled watering control messages to controller
@celery.task
def send_scheduled_watering_msg(zone, amount):
    # get json obj from ../database/rate_conversions.json
    with open('../database/rate_conversions.json', 'r') as json_file:
        data = json.load(json_file)
    conv = data["zone"]
    duration = amount/conv
    print(f'\{\"zone\":{zone}, \"solenoid state\":1, \"solenoid on duration\":{duration}\}')

# schedules watering control messages that go to controller to get forwarded
@celery.task()
def schedule_watering(sender, **kwargs):
    # get json obj from ../database/watering_schedule.json
    with open('../database/watering_schedule.json', 'r') as json_file:
        data = json.load(json_file)

    # check if date is current, stall if not
    while data["date"] != datetime.now().strftime('%Y-%m-%d'):
        time.sleep(60.0) # check every minute for an update

    # maybe implement some timeout...

    # now that it's updated, queue tasks to the controller
    for zone_dict in data["zones"]:
        zone = zone_dict["zone"]
        for hour, amount in enumerate(zone_dict["watering_schedule"]):
            if amount > 0:
                sender.add_periodic_task(crontab(minute=0, hour=hour), send_scheduled_watering_msg.s(zone, amount), name=f'*zone {zone} water at {hour:2}:00*')

@celery.on_after_configure.connect
def setup_periodic_tasks(sender, **kwargs):
    sender.add_periodic_task(60.0*30.0, retrieve_weather(), name='weather: add every 30 minutes')
    sender.add_periodic_task(crontab(minute=0, hour=0), schedule_watering(sender), name='water: add at midnight')

def forcast(city="South Bend"):
    # Enter your API key here
    api_key = os.getenv("API_KEY")

    # base_url variable to store url
    base_url = "http://api.openweathermap.org/data/2.5/weather?"

    # complete_url variable to store
    # complete url address
    complete_url = base_url + "appid=" + api_key + "&q=" + city

    # get method of requests module
    # return response object
    response = requests.get(complete_url)

    # json method of response object 
    # convert json format data into
    # python format data
    x = response.json()
    #print(x,"\n")

    # Now x contains list of nested dictionaries
    # Check the value of "cod" key is equal to
    # "404", means city is found otherwise,
    # city is not found
    if x["cod"] != "404":

        # store the value of "main"
        # key in variable y
        y = x["main"]

        # store the value corresponding
        # to the "temp" key of y
        current_temperature = y["temp"]
        ftemp = int(((float(current_temperature))-273.15)*1.8+32)#Converts to fahrenheit
		
        tobj = time.localtime(time.time())
        time_str = time.asctime(tobj)
  
        time_str.replace(" ","--")

        curdir = os.path.dirname(__file__)
        head, tail = os.path.split(curdir)

        df = pd.read_csv(head + '/database/weather.csv')
        new_row = {'time':time_str, 'temperature':ftemp,'humidity':y["humidity"],'weather':x["weather"][0]["description"]}
        df = pd.concat([df, pd.DataFrame([new_row])], ignore_index=True)
        df.to_csv(head + '/database/weather.csv')

    else:
        print(" City Not Found ")
    

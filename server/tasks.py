#!/home/kpmealey/git/Greenthumb_Flow/server/server_venv/bin/python3

from celery import Celery, Task
from flask import Flask
from celery.schedules import crontab
import time
import os
import pandas as pd
import requests

celery = Celery('tasks', broker='redis://localhost:6379/0', backend='redis://localhost:6379/1')

# This can be the weather getting
@celery.task
def retrieve_weather():
    forcast()
    print("Retrieved weather data!")

# This can be the watering schedule creation and watering data saving
@celery.task()
def schedule_watering():
    # fine tune neural net
    # get result
    # add new watering tasks
    print("Created and save watering data!")

@celery.on_after_configure.connect
def setup_periodic_tasks(sender, **kwargs):
    sender.add_periodic_task(60.0, retrieve_weather.s(), name='weather: add every 1 minute')
    sender.add_periodic_task(30.0, schedule_watering.s(), name='water: add every 30')


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
    

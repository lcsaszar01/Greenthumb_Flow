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
import sys
sys.path.insert(0, '../ml_tasks')
from make_csvs import make_csvs
from finetune_model import finetune_model
from  create_watering_schedule import create_watering_schedule

celery = Celery('tasks', broker = 'redis://localhost:6379/0', backend = 'redis://localhost:6379/1')
timezone = 'America/New_York'
celery.conf.timezone = timezone

data_folder_path = "../demo_data"
weather_path = f"{data_folder_path}/weather.csv"
forecast_path = f"{data_folder_path}/forecast.json"

controller_ip = '10.7.169.198'  # Replace with your Arduino's IP address

# Encode the data in the URL
def data2req(data_dict):
    encoded_data = '&'.join([f'{key}={value}' for key, value in data.items()])
    url = f'http://{controller_ip}/?{encoded_data}'

# This can be the weather getting
@celery.task
def retrieve_weather():
    get_current_weather()
    print("Retrieved weather data!")

@celery.task
def request_sensor_data(zone):

    print(f"Requesting sensor data from {zone}!")    
    message = {"id":"SERV", "packet number":0, "type":"request", "request_type":"sensor", "zone":zone}
    url = data2req(message)
    response = requests.get(url)
    
    # Check the response
    print(f'Status Code: {response.status_code}')
    print('Response Content:')
    rint(response.text)

# sends scheduled watering control messages to controller
@celery.task
def send_scheduled_watering_msg(zone, amount):
    message = {"id":"SERV", "packet number":0, "type":"request", "request_type":"solenoid", "zone":zone, "amount":0.25}
    url = data2req(message)
    response = requests.get(url)
    
    # Check the response
    print(f'Status Code: {response.status_code}')
    print('Response Content:')
    print(response.text)

# schedules watering control messages that go to controller to get forwarded
@celery.task()
def schedule_watering(sender, **kwargs):

    # update forecast.json
    forecast = get_weather_forecast()

    # get sunrise string (needed in 'record watering data')
    sunrise_utc = forecast["city"]["sunrise"]
    timezone = forecast["city"]["timezone"] # shift in seconds from UTC
    sunrise = datetime.utcfromtimestamp(sunrise+timezone).strftime('%Y-%m-%d %H:%M:%S')
    
    # call finetune_model.py
    finetune_model()

    # call create_watering_schedule.py
    create_watering_schedule()

    # get json obj from ../database/watering_schedule.json
    with open('../database/watering_schedule.json', 'r') as json_file:
        data = json.load(json_file)

    # check if date is current, stall if not
    while data["date"] != datetime.now().strftime('%Y-%m-%d'):
        time.sleep(60.0) # check every minute for an update

    # maybe implement some timeout...

    # now that it's updated, queue tasks to the controller
    for zone in data["schedules"]:
        amount = data["schedules"][zone]["amount"]
        time = datetime.strptime(data["schedules"][zone]["amount"], '%Y-%m-%d %H:%M:%S')
        if amount > 0:
            sender.add_periodic_task(crontab(minute=time.minute, hour=time.hour), send_scheduled_watering_msg.s(zone, amount), name=f'*zone {zone} water at {time.hour:2}:{time.minute:2}*')

        # record watering data in watering.csv
        watering_dict={"time":time, "zone":zone, "amount (L)":amount, "sunrise_time":datetime.utcfromtimestamp(forecast["city"]["sunrise"]).strftime('%Y-%m-%d %H:%M:%S')} 

@celery.on_after_finalize.connect
def setup_periodic_tasks(sender, **kwargs):
    # Check csvs
    #check_csvs()
    sender.add_periodic_task(30.0, request_sensor_data("A"), name='sensor A: add every 20 minutes')
    sender.add_periodic_task(30.0, request_sensor_data("B"), name='sensor B: add every 20 minutes')
    sender.add_periodic_task(60.0*60.0*3, retrieve_weather(), name='weather: add every 3 hours')
    sender.add_periodic_task(crontab(minute=0, hour=0), schedule_watering(sender), name='water: add at midnight')

def check_csvs():
    if not os.path.isdir('../database'):
       os.mkdir('../database')

    for path in ['../database/weather.csv','../database/watering.csv','../database/sensors.csv']:
        if not os.path.exists(path):
            make_csvs(path)  

def get_current_weather(city="South Bend"):
    api_key = os.getenv("API_KEY")
    base_url = "http://api.openweathermap.org/data/2.5/weather?&units=imperial"
    complete_url = base_url + "appid=" + api_key + "&q=" + city
    response = requests.get(complete_url)
    x = response.json()
    
    if x["cod"] != "404":

        new_entry = {}
        new_entry["time"] = datetime.now(pytz.timezone(timezone)).strftime('%Y-%m-%d %H:%M:%S')
        new_entry["temperature (F)"] = x["main"]["temp"]
        new_entry["humidity (percent)"] = x["main"]["humidity"]
        new_entry["clouds (percent coverage)"] = x["clouds"]["all"]
        new_entry["wind (mph)"] = x["wind"]["speed"]
        new_entry["weather"] = x["weather"]["description"]
        new_entry["zone"] = "all"

        df = pd.read_csv(weather_path)
        df = pd.concat([df, pd.DataFrame([new_row])], ignore_index=True)
        df.to_csv(weather_path)

    else:
        print(" City Not Found ")
    
def get_weather_forecast(city="South Bend"):

    # Enter your API key here
    api_key = os.getenv("API_KEY")

    # base_url variable to store url
    base_url = "http://api.openweathermap.org/data/2.5/forecast?&units=imperial"

    # complete_url variable to store
    # complete url address
    complete_url = base_url + "appid=" + api_key + "&q=" + city

    # get method of requests module
    # return response object
    response = requests.get(complete_url)

    x = response.json()

    if x["cod"] != "404":
        with open(forecast_path, 'w') as json_file:
            json.dump(x, json_file)

    else:
        print(" City Not Found ")
        return None

    return x

#!/home/kpmealey/git/Greenthumb_Flow/server/server_venv/bin/python3

from celery import Celery, Task
from flask import Flask
from celery.schedules import crontab
import time

celery = Celery('tasks', broker='redis://localhost:6379/0', backend='redis://localhost:6379/1')

# This can be the weather getting
@celery.task
def retrieve_weather():
    print("Retrieved weather data!")

# This can be the watering schedule creation and watering data saving
@celery.task()
def schedule_watering():
    print("Created and save watering data!")

@celery.on_after_configure.connect
def setup_periodic_tasks(sender, **kwargs):
    sender.add_periodic_task(10.0, retrieve_weather.s(), name='weather: add every 10')
    sender.add_periodic_task(30.0, schedule_watering.s(), name='water: add every 30')

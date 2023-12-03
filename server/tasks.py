#!/home/kpmealey/git/Greenthumb_Flow/server/server_venv/bin/python3

from celery import Celery, Task
from flask import Flask
from celery.schedules import crontab
import time

celery = Celery('tasks', broker='redis://localhost:6379/0')

# This can be the watering schedule creation and watering data saving
@celery.task
def background_task():
    time.sleep(10)
    print("Background task completed!")

# This can be the weather getting
@celery.task(bind=True, name='tasks.periodic_task', default_retry_delay=300, max_retries=3)
#def periodic_task(self):
def periodic_task():
    print("Periodic task executed")

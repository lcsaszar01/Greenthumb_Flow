#!/home/kpmealey/git/Greenthumb_Flow/server/server_venv/bin/python3


from flask import Flask, render_template, request
from weather import forcast
from markupsafe import escape
from markupsafe import Markup
import schedule
import time
import pandas as pd
import json
from celery import Celery, Task
from celery.schedules import crontab
import tasks

app = Flask(__name__)
celery = Celery(app.name, broker='redis://localhost:6379/0', include=['tasks'])
app.config.update(
    CELERY_BROKER_URL='redis://localhost:6379/0',
    CELERY_RESULT_BACKEND='redis://localhost:6379/1'
)
app.config['CELERYBEAT_SCHEDULE'] = {
    'periodic-task': {
        'task': 'tasks.periodic_task',
        'schedule': crontab(minute=50, hour=17),
        #'schedule': 30.0,
        'options': {
            'expires':15.0,
        },
    },
}
app.config['CELERY_TIMEZONE'] = 'America/New_York'

@app.route('/')
@app.route('/index')
def index(name=None):
    result = tasks.background_task.delay()
    return render_template('index.html', task_id=result.id)

@app.route('/weather')
def get_weather():
    city = request.args.get('city')
    weather_data = forcast()

    return render_template(
        "weather.html",
        
        status=weather_data[0]["discrip[tion]"].capitalize(),
        temp=f"{weather_data['main']['temp']:.1f}",
        feels_like=f"{weather_data['main']['feels_like']:.1f}"
    )


# first three lines given by ChatGPT -- untested
@app.route('/receive_serial_data', methods=['POST'])
def receive_serial_data():
    sensor_data = request.json  # Assuming data is sent as JSON
    

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000)

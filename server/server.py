#!/home/kpmealey/git/Greenthumb_Flow/server/server_venv/bin/python3


from flask import Flask, render_template, request
from weather import forecast
from markupsafe import escape
from markupsafe import Markup
import schedule
import time
import pandas as pd
import json
from celery import Celery, Task
from celery.schedules import crontab
import tasks
4
app = Flask(__name__)
app.config.from_pyfile('celeryconfig.py')

@app.route('/')
@app.route('/index')
def index(name=None):
    return render_template('index.html') 
# task_id=result.id)

@app.route('/more')
def more(name=None):
    return render_template('more.html') 

@app.route('/login')
def log_in(name=None):
    return render_template('log_in.html') 

@app.route('/weather')
def get_weather(name=None):
    city = request.args.get('city')
    weather_data = forecast(city)

    return render_template(
        "weather_report.html",
        
    )


if __name__ == "__main__":
    app.run()

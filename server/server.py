#!/home/kpmealey/git/Greenthumb_Flow/server/server_venv/bin/python3


from flask import Flask, render_template, request
import numpy as np 

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
import os

import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

prefix = os.path.dirname(__file__)
basedir, tail_trash = os.path.split(prefix)


app = Flask(__name__)
app.config.from_pyfile('celeryconfig.py')

@app.route('/')
@app.route('/index')
def index(name=None):
    return render_template('index.html') 
# task_id=result.id)

def get_plot(): 
    df = pd.read_csv("/Users/lcsaszar/Documents/Greenthumb_Flow/demo_data/sensors.csv")
    df = df[df["zone"].isin(["A"])]
    df = df.drop(columns=['time'])

    ax = df.plot(kind="scatter", x="moisture",y="humidity", color="b", label="moisture vs. humidity")
    df.plot(x="temperature",y="humidity", color="r", label="temp. vs. humidity", ax=ax)
    df.plot( x="moisture",y="ph", color="g", label="moisture vs. ph", ax=ax)
    df.plot( x="temperature",y="uv", color="orange", label="temp. vs uv", ax=ax)
    df.plot( x="humidity",y="ph", color="purple", label="humidity vs. ph", ax=ax)
    ax.set_xlabel("horizontal label")
    ax.set_ylabel("vertical label")
    return plt 

@app.get('/data') 
def plot_data(): 
    # Get the matplotlib plot  
    plot = get_plot() 

    # Save the figure in the static directory 
    # CHANGE TO YOUR ABSOLUTE PATH BEFORE RUNNING 
    plot.savefig("/Users/lcsaszar/Documents/Greenthumb_Flow/server/static/plot.png") 
  
    return render_template('matplotlib-plot1.html') 

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
    app.run(debug=True, port=5000, host='0.0.0.0')

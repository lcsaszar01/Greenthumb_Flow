#!/home/kpmealey/git/Greenthumb_Flow/server/server_venv/bin/python3


from flask import Flask, render_template, request
import numpy as np 
import matplotlib.pyplot as plt 
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

app = Flask(__name__)
app.config.from_pyfile('celeryconfig.py')

@app.route('/')
@app.route('/index')
def index(name=None):
    return render_template('index.html') 
# task_id=result.id)

def get_plot(): 
    df = pd.read_csv("C:/Users/longf/Greenthumb_Flow/database/sensors.csv")
    df = df[df["zone"].isin(["A"])]
    df = df.drop(columns=['time'])

    # data = { 
    #     'a': np.arange(50), 
    #     'c': np.random.randint(0, 50, 50), 
    #     'd': np.random.randn(50) 
    # } 
    # data['b'] = data['a'] + 10 * np.random.randn(50) 
    # data['d'] = np.abs(data['d']) * 100
  
    # df.plot.scatter('a', 'b', c='c', s='d', data=data) 

    # ax1 = df.plot(kind='scatter', x='moisture', y='humidity', color='r')
    # ax2 = df.plot(kind='scatter', x='moisture', y='humidity', color='r')
    # df.plot.xlabel('X label') 
    # plot.ylabel('Y label') 
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
    plot.savefig("c:/Users/longf/Greenthumb_Flow/server/static/plot.png") 
  
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

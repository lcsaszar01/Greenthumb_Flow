#!/home/kpmealey/miniconda3/envs/lawnlogic/bin/python3


from flask import Flask, render_template, request
from weather import forcast
from markupsafe import escape
from markupsafe import Markup
import schedule
import time
import pandas as pd
import json

app = Flask(__name__)

app = Flask(__name__)
app.config.from_mapping(
    CELERY=dict(
        broker_url="redis://localhost",
        result_backend="redis://localhost",
        task_ignore_result=True,
    ),
)

@app.route('/')
@app.route('/index')
def index(name=None):
    return render_template('index.html')

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
    celery_init_app(app)
    app.run(host="0.0.0.0", port=8000)

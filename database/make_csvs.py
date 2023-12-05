#!/home/kpmealey/miniconda3/envs/lawnlogic/bin/python3

import pandas as pd
import random
import json
import time

time_15 = [stamp.strftime('Tue--Dec----5--%H:%M:%S') for stamp in pd.date_range(start='2023-12-05 00:00:00', end='2023-12-05 23:45:00', freq='15T')]
time_30 = [stamp.strftime('Tue--Dec----5--%H:%M:%S') for stamp in pd.date_range(start='2023-12-05 00:00:00', end='2023-12-05 23:45:00', freq='30T')]
time_60 = [stamp.strftime('Tue--Dec----5--%H:%M:%S') for stamp in pd.date_range(start='2023-12-05 00:00:00', end='2023-12-05 23:45:00', freq='60T')]

with open('weather_descriptions.json', 'r') as file:
    data = json.load(file)
weather_descriptions = data.get('weather descriptions')

def generate_random(n, range_min, range_max):
    return [random.uniform(range_min, range_max) for _ in range(n)]

#sensor_df = pd.DataFrame(columns=["time", "zone", "moisture", "humidity", "temperature", "ir", "vis", "uv", "ph"])
n = len(time_15)
sensor_data = {"time":sorted(time_15*2), "zone":["A", "B"]*n, "moisture":generate_random(n*2,0,100), "humidity":generate_random(n*2,0,100), "temperature":generate_random(n*2, 40, 90),"ir":generate_random(n*2, 0,100), "vis":generate_random(n*2, 0, 100), "uv":generate_random(n*2, 0, 100), "ph":generate_random(n*2, 5, 8)}
sensor_df = pd.DataFrame(sensor_data)
sensor_df.to_csv('sensors.csv', index=False)

#watering_df = pd.DataFrame(columns=["time", "zone", "amount (L)"])
m = len(time_60)
watering_data = {"time":sorted(time_60*2), "zone":["A", "B"]*m, "amount (L)":generate_random(m*2, 0, 500)}
watering_df = pd.DataFrame(watering_data)
watering_df.to_csv('watering.csv', index=False)


#weather_df = pd.DataFrame(columns=["time","temperature", "weather"])
p = len(time_30)
weather_data = {"time":time_30, "temperature":generate_random(p,40,90), "weather":random.choices(weather_descriptions, k=p)}
weather_df = pd.DataFrame(weather_data)
weather_df.to_csv('weather.csv', index=False)

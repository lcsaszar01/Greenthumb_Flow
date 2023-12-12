#!/home/kpmealey/miniconda3/envs/lawnlogic/bin/python3

import pandas as pd
import random
import json
import time

# '2023-12-08 06:00:00'

start_date = '2023-11-00 00:00:00'
end_date = '2023-12-06 23:59:00'
time_20 = [stamp.strftime() for stamp in pd.date_range(start=start_date, end=end_date, freq='20T')]
time_3H = [stamp.strftime() for stamp in pd.date_range(start=start_date, end=end_date, freq='180T')]
time_1D = [(stamp + pd.Timedelta(hours=random.randint(6,10))).strftime() for stamp in pd.date_range(start=start_date, end=end_date, freq='1440T')]

with open('weather_descriptions.json', 'r') as file:
    data = json.load(file)
weather_descriptions = data.get('weather descriptions')

def generate_random(n, range_min, range_max):
    return [random.uniform(range_min, range_max) for _ in range(n)]

#sensor_df = pd.DataFrame(columns=["time", "zone", "moisture", "humidity", "temperature", "ir", "vis", "uv", "ph"])
n = len(time_20)
sensor_data = {"time":sorted(time_20*2), "zone":["A", "B"]*n, "moisture":generate_random(n*2,0,100), "humidity":generate_random(n*2,0,100), "temperature":generate_random(n*2, 40, 90),"ir":generate_random(n*2, 0,100), "vis":generate_random(n*2, 0, 100), "uv":generate_random(n*2, 0, 100), "ph":generate_random(n*2, 5, 8)}
sensor_df = pd.DataFrame(sensor_data)
sensor_df.to_csv('sensors.csv', index=False)

#weather_df = pd.DataFrame(columns=["time","temperature", "weather"])
p = len(time_3H)
weather_data = {"time":time_3H, "temperature (F)":generate_random(p,40,90), "humidity (percent)":generate_random(p, 0, 100), "clouds (percent coverage)":generate_random(p, 0, 100), "wind (mph)":generate_random(p, 0, 50), "weather":random.choices(weather_descriptions, k=p)}
weather_df = pd.DataFrame(weather_data)
weather_df.to_csv('weather.csv', index=False)

#watering_df = pd.DataFrame(columns=["time", "zone", "amount (L)"])
m = len(time_1D)
watering_data = {"time":sorted(time_1D*2), "zone":["A", "B"]*m, "amount (L)":generate_random(m*2, 0, 500)}
watering_df = pd.DataFrame(watering_data)
watering_df.to_csv('watering.csv', index=False)
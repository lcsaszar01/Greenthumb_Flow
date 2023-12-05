#!/home/kpmealey/miniconda3/envs/lawnlogic/bin/python3

import pandas as pd
import random
import json
import time

time_str= time.asctime(time.localtime(time.time())).replace(" ","--")

with open('weather_descriptions.json', 'r') as file:
    data = json.load(file)
weather_descriptions = data.get('weather descriptions')

def generate_random(n, range_min, range_max):
    return [random.uniform(range_min, range_max) for _ in range(n)]

#sensor_df = pd.DataFrame(columns=["time", "zone", "moisture", "humidity", "temperature", "ir", "vis", "uv", "ph"])
n = 24*4
sensor_data = {"time":[time_str]*n*2, "zone":["A", "B"]*n, "moisture":generate_random(n*2,0,100), "humidity":generate_random(n*2,0,100), "temperature":generate_random(n*2, 40, 90),"ir":generate_random(n*2, 0,100), "vis":generate_random(n*2, 0, 100), "uv":generate_random(n*2, 0, 100), "ph":generate_random(n*2, 5, 8)}
sensor_df = pd.DataFrame(sensor_data)
sensor_df.to_csv('sensors.csv', index=False)

#watering_df = pd.DataFrame(columns=["time", "zone", "amount (L)"])
m = 24
watering_data = {"time":[time_str]*m*2, "zone":["A", "B"]*m, "amount (L)":generate_random(m*2, 0, 500)}
watering_df = pd.DataFrame(watering_data)
watering_df.to_csv('watering.csv', index=False)


#weather_df = pd.DataFrame(columns=["time","temperature", "weather"])
p = 24*2
weather_data = {"time":[time_str]*p, "temperature":generate_random(p,40,90), "weather":random.choices(weather_descriptions, k=p)}
weather_df = pd.DataFrame(weather_data)
weather_df.to_csv('weather.csv', index=False)

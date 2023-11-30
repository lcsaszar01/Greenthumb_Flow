#!/home/kpmealey/miniconda3/envs/lawnlogic/bin/python3

import pandas as pd

sensor_df = pd.DataFrame(columns=["time", "zone", "moisture", "humidity", "temperature", "ir", "vis", "uv", "ph"])
sensor_df.to_csv('sensors.csv', index=False)

watering_df = pd.DataFrame(columns=["time", "zone", "amount (L)", "pace (L/hr)"])
watering_df.to_csv('watering.csv', index=False)

weather_df = pd.DataFrame(columns=["time","temperature", "pressure", "weather"])
weather_df.to_csv('weather.csv', index=False)

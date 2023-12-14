#!/home/kpmealey/miniconda3/envs/lawnlogic/bin/python3

import pandas as pd

data_folder_path = "../database"

def make_csvs(path):
    if 'sensors' in path:
        pd.DataFrame(columns=["time", "zone", "moisture", "humidity", "temperature", "ir", "vis", "uv", "ph"]).to_csv(path)
    
    elif 'weather' in path:
        pd.DataFrame(columns=["time","temperature (F)","humidity (percent)","clouds (percent coverage)","wind (mph)","weather"]).to_csv(path)
    
    elif 'watering' in path:
        pd.DataFrame(columns=["time", "zone", "amount (L)", "sunrise_time"]).to_csv(path)
    else:
        pass

if __name__=='__main__':
    make_csvs(f'{data_folder_path}/sensors.csv')
    make_csvs(f'{data_folder_path}/weather.csv')
    make_csvs(f'{data_folder_path}/watering.csv')

#!/home/kpmealey/git/Greenthumb_Flow/server/server_venv/bin/python3

import pandas as pd
import numpy as np
from keras.models import load_model
from datetime import datetime
import json
from scipy import spatial
from itertools import product
from sklearn.preprocessing import MinMaxScaler
from scipy.optimize import minimize
import ml_utils

data_path = "../demo_data"
watering_path = f"{data_path}/watering.csv"
weather_path = f"{data_path}/weather.csv"
sensor_path = f"{data_path}/sensors.csv"
weather_descriptions_path = '../configs/weather_descriptions.json'
model_path = "../models"
watering_schedule_path = "../configs/watering_schedule.json"

zones = ["A", "B"]

def calculate_cosine_similarity(x, y):
    
    # Ensure length of x and y are the same
    if len(x) != len(y) :
        return None
    
    return 1 - spatial.distance.cosine(x, y)

# Define the optimization objective function
def objective_function(params, model, scaler_water_amount, scaler_water_time, N_days):

    # Transform back into matrix shape
    params = params.reshape(N_days, 2)
    
    # Inverse transform parameters
    amount_watered = scaler_water_amount.inverse_transform([[params[iday][0] for iday in range(N_days)]])
    time_of_watering = scaler_water_time.inverse_transform([[params[iday][1] for iday in range(N_days)]])

    # Create input for the model
    watering_input = np.array([[[amount_watered[0][iday], time_of_watering[0][iday]] for iday in range(N_days)]])
    weather_input = np.array([weather_feats_normalized])

    # Use the base model to predict soil moisture
    input_data = [watering_input, weather_input]
    predicted_sequence = model.predict(input_data)
    predicted_sequence = predicted_sequence.reshape(predicted_sequence.shape[1])

    # Calculate cosine similarity
    similarity = calculate_cosine_similarity(predicted_sequence, optimal_sequence)

    return -similarity  # Minimize the negative similarity to maximize actual similarity

def create_watering_schedule():

    # Get weather forecast
    with open(f'{data_path}/forecast.json', 'r') as json_file:
        forecast_data = json.load(json_file)["list"]
    forecast_dict = {column:[] for column in ["time", "temperature (F)", "humidity (percent)", "clouds (percent coverage)", "wind (mph)", "weather", "zone"]}
    for i in range(len(data)):
        forecast_dict["time"].append(datetime.utcfromtimestamp(forecast_data[i]["dt"]).strftime('%Y-%m-%d %H:%M:%S'))
        forecast_dict["temperature (F)"].append(forecast_data[i]["main"]["temp"])
        forecast_dict["humidity (percent)"].append(forecast_data[i]["main"]["humidity"])
        forecast_dict["clouds (percent coverage)"].append(forecast_data[i]["clouds"]["all"])
        forecast_dict["wind (mph)"].append(forecast_data[i]["wind"]["speed"])
        forecast_dict["weather"].append(forecast_data[i]["weather"][0]["description"])
        forecast_dict["zone"].append("all")
    weather_forecast = pd.DataFrame(forecast_dict)
    weather_forecast_feats = ml_utils.get_weather_feats(weather_forecast, 0, len(weather_forecast), zones, weather_descriptions_path)[zones[0]][0]

    # Get sunrise time (will need this for later)

    sunrise = forecast_data["city"]["sunrise"]
    timezone = forecast_data["city"]["timezone"] # shift in seconds from UTC

    sunrise_dt = datetime.utcfromtimestamp(sunrise+timezone)

    # Get sequence of moisture levels to optimize toward (assuming there is an ideal pattern for 5 days)
    with open(f'{data_path}/optimals.json', 'r') as json_file:
        data = json.load(json_file)
    optimal_sequence = data["soil moisture sequence"]
    # guesses for good starts for optimization:
    initial_amount_watered = data["initial guess 5 day watering amounts"]
    initial_time_of_watering = data["initial guess 5 day watering times"]

    # Get range of possible amounts and times to test

    # Define the range of values for water_amount and water_time
    amount_range = np.linspace(100, 300, 20)
    time_range = np.linspace(-120, 180, 20)

    # Create a grid of all possible combinations of water_amount and water_time
    water_amount, water_time = np.meshgrid(amount_range, time_range)

    # Flatten the grids to get 1D arrays
    water_amount_combos = water_amount.flatten()
    water_time_combos= water_time.flatten()

    # Those are all the possible combinations of amounts and times, but now we need to account for sequences of 5 days where any day could either have a watering event or not

    # Generate all possible combinations
    combinations = list(product([0,1], repeat=5))

    # Convert each combination to a string
    binary_arrays = np.array([[combo[x] for x in range(len(combo))] for combo in combinations])

    N_val_combos = len(water_amount_combos)
    N_event_combos = binary_arrays.shape[0]
    N_days = binary_arrays.shape[1]

    water_amount = np.empty((N_val_combos, N_event_combos, N_days))
    water_time = np.empty((N_val_combos, N_event_combos, N_days))
    for i in range(N_val_combos):
        water_amount[i] = binary_arrays*water_amount_combos[i]
        water_time[i] = binary_arrays*water_time_combos[i]

    Nsamples = N_val_combos*N_event_combos
    water_amount = water_amount.reshape(Nsamples, N_days)
    water_time = water_time.reshape(Nsamples, N_days)

    # Scale transform data:
    scaler_water_amount = MinMaxScaler()
    scaler_water_time = MinMaxScaler()
    scaler_weather = MinMaxScaler()
    scaler_target = MinMaxScaler()

    # Normalize data
    water_amount_normalized = scaler_water_amount.fit_transform(water_amount.reshape(-1,N_days)).reshape(water_amount.shape)
    water_time_normalized = scaler_water_time.fit_transform(water_time.reshape(-1,N_days)).reshape(water_time.shape)
    watering_feats_normalized = np.array([[[water_amount_normalized[x][y],water_time_normalized[x][y]] for y in range(N_days)] for x in range(Nsamples)])
    weather_feats_normalized = scaler_weather.fit_transform(weather_forecast_feats.reshape(-1, 40*45)).reshape(weather_forecast_feats.shape)

    # Initial parameter values
    initial_params = np.array([[scaler_water_amount.transform(np.array([initial_amount_watered]))[0][i],
            scaler_water_time.transform(np.array([initial_time_of_watering]))[0][i]] for i in range(N_days)]).reshape(N_days*2)

    # Create dict to populate:
    watering_schedule_dict = {"date":datetime.now().strftime('%Y-%m-%d'), "schedules":{zone:{} for zone in zones}}

    for zone in zones:

        zone_model = load_model(f'{model_path}/{zone}_model.h5')
        zone_model.compile(optimizer='adam', loss='mean_squared_error') # must be same as what it was originally trained with

        # Perform optimization
        result = minimize(objective_function, initial_params, args=(zone_model, scaler_water_amount, scaler_water_time, N_days), method='Nelder-Mead')

        # Extract optimal parameters
        optimal_params = result.x.reshape(5,2)

        # Extract optimal values
        optimal_amount_watered = scaler_water_amount.inverse_transform([[optimal_params[iday][0] for iday in range(N_days)]])
        optimal_time_of_watering = scaler_water_time.inverse_transform([[optimal_params[iday][1] for iday in range(N_days)]])
    
        # Convert:
        optimal_amount_watered = optimal_amount_watered[0][0]
        optimal_time_of_watering = (sunrise_dt + pd.Timedelta(minutes=round(optimal_time_of_watering[0][0]))).strftime('%Y-%m-%d %H:%M:%S')

        # Save to watering_schedule_dict
        watering_schedule_dict["schedules"][zone]["amount"] = optimal_amount_watered
        watering_schedule_dict["schedules"][zone]["time"] = optimal_time_of_watering

    # Convert and write JSON object to file
    with open(watering_schedule_path, "w") as outfile: 
        json.dump(watering_schedule_dict, outfile)

if __name__=='__main__':
    create_watering_schedule()

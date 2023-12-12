import pandas as pd
import random
from datetime import datetime
import json
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import MinMaxScaler
import tensorflow as tf
from tensorflow.keras.models import Sequential, Model
from tensorflow.keras.layers import Input, LSTM, Dense, concatenate

# Get "watering" features that occur once per day

def get_watering_feats(watering_df, start_row, end_row):

    # Should check for valid start and end rows

    watering_feats = {}
    for zone in list(watering_df["zone"].unique()):

        watering_df_zone = watering_df[watering_df["zone"] == zone]

        # Get amount watered
        watering_dict = {"amount (L)" : list(watering_df_zone["amount (L)"].iloc[start_row:end_row])}

        # Find how many minutes after sunrise the lawn was watered each day
        watering_dict["min_after_sunrise"] = []
        for i in range(start_row, end_row):
            n_seconds = (datetime.strptime(watering_df_zone["time"].iat[i], '%Y-%m-%d %H:%M:%S') - datetime.strptime(watering_df_zone["sunrise_time"].iat[i], '%Y-%m-%d %H:%M:%S')).seconds

            if n_seconds > 6*60*60:
                n_minutes = -1*round((24*60*60 - n_seconds)/60)
            else:
                n_minutes = round(n_seconds/60)
            watering_dict["min_after_sunrise"].append(n_minutes)

        # Find initial soil moisture before watering
        #watering_dict["initial_moisture"]= []
        #sensor_datetimes = {datetime.strptime(sensor_df_zone["time"].iat[i], '%Y-%m-%d %H:%M:%S'):i for i in range(len(sensor_df_zone))}
        #for i in range(start_row, end_row):
        #    water_stamp = datetime.strptime(watering_df_zone["time"].iat[i], '%Y-%m-%d %H:%M:%S')
        #    closest_datetime = min((dt for dt in sensor_datetimes.keys() if dt < water_stamp), key=lambda x: abs(x - water_stamp))
        #    watering_dict["initial_moisture"].append(sensor_df_zone["moisture"].iat[sensor_datetimes[closest_datetime]])
                
        watering_feats_zone = np.array(pd.DataFrame(watering_dict))

        # Make into sequences of 5 days
        watering_feats_zone = np.lib.stride_tricks.sliding_window_view(watering_feats_zone, window_shape=(5,2)).reshape(watering_feats_zone.shape[0]-4, 5, 2)

        watering_feats[zone] = watering_feats_zone

    return watering_feats

# Transform textual weather descriptions to vectors

def vectorize_weather(weather_df, start_row, end_row, weather_descriptions):

    weather_data = list(weather_df["weather"].iloc[start_row:end_row])
    
    vectorizer = CountVectorizer()
    vectorizer.fit(weather_descriptions)
    weather_vec = vectorizer.transform(weather_data).toarray()

    return weather_vec

# Get weather features

def get_weather_feats(weather_df, start_row, end_row, zones, weather_descriptions_path):

    # First, vectorize weather descriptions:
    
    with open(weather_descriptions_path, 'r') as json_file:
        data = json.load(json_file)
    weather_descriptions = data["weather descriptions"]

    weather_vec = vectorize_weather(weather_df, start_row, end_row, weather_descriptions)
    weather_df_vec = pd.concat((weather_df.iloc[start_row:end_row], pd.DataFrame(weather_vec)), axis=1)

    # Then add a column with dates so we can seperate by day:
    date_dict = {"date": [datetime.strptime(weather_df["time"].iat[irow], '%Y-%m-%d %H:%M:%S').date() for irow in range(start_row, end_row)]}
    weather_df_date = pd.concat((weather_df_vec, pd.DataFrame(date_dict)), axis=1)

    # get relevant columns:
    weather_cols = list(weather_df_date.columns)
    weather_cols.remove("time")
    weather_cols.remove("zone")
    weather_cols.remove("weather")
    weather_cols.remove("date")

    # make sequential by 5 days
    weather_seqs = np.empty((len(weather_df_date["date"].unique())-4, 5, 8, len(weather_cols))) # 8 is 24/3 (how many weather reports per day)
    last_start_date = list(weather_df_date["date"].unique())[-1] - pd.Timedelta(days=4)
    for istartdate, start_date in enumerate(weather_df_date["date"].unique()):
        if start_date <= last_start_date:
            for iday in range(5):
                date = start_date + pd.Timedelta(days=iday)
                weather_seq = np.array(weather_df_date[(weather_df_date["date"] == date)][weather_cols])
                weather_seqs[istartdate][iday] = (weather_seq)

    weather_seqs = weather_seqs.reshape((len(weather_df_date["date"].unique())-4), 40, len(weather_cols))
    weather_feats = {zone:weather_seqs for zone in zones}
    return weather_feats

# Get target feature -- soil moisture

def get_target_feat(sensor_df, start_row, end_row):

    target_feat = {}

    for zone in sensor_df["zone"].unique():
        sensor_df_zone = sensor_df[sensor_df["zone"] == zone]
        date_dict = {"date": [datetime.strptime(sensor_df_zone["time"].iat[irow], '%Y-%m-%d %H:%M:%S').date() for irow in range(start_row, end_row)]}
        sensor_df_date = pd.concat((pd.DataFrame({"moisture":list(sensor_df_zone["moisture"].iloc[start_row:end_row])}), pd.DataFrame(date_dict)), axis=1)

        # make sequential by 5 days
        moisture_seqs = np.empty((len(sensor_df_date["date"].unique()) - 4, 5, 72)) # 72 is 24*60/20 (how many sensor reports per day)
        last_start_date = list(sensor_df_date["date"].unique())[-1] - pd.Timedelta(days=4)
        for istartdate, start_date in enumerate(sensor_df_date["date"].unique()):
            if start_date <= last_start_date:
                for iday in range(5):
                    date = start_date + pd.Timedelta(days=iday)
                    moisture_seq = np.array(sensor_df_date[(sensor_df_date["date"] == date)]["moisture"])
                    moisture_seqs[istartdate][iday] = moisture_seq

        moisture_seqs = moisture_seqs.reshape((len(sensor_df_date["date"].unique())-4), 360)

        target_feat[zone] = moisture_seqs
    
    return target_feat

def create_training_data(watering_feats, weather_feats, target_feat, zone):

    watering_feats_normalized = MinMaxScaler().fit_transform(watering_feats[zone].reshape(-1, 5*2)).reshape(watering_feats[zone].shape)
    weather_feats_normalized = MinMaxScaler().fit_transform(weather_feats[zone].reshape(-1, 40*45)).reshape(weather_feats[zone].shape)
    target_feat_normalized = MinMaxScaler().fit_transform(target_feat[zone])

    # Split data into training and testing sets
    watering_train, watering_test, weather_train, weather_test, target_train, target_test = train_test_split(
        watering_feats_normalized, weather_feats_normalized,
        target_feat_normalized, test_size=0.2, random_state=42
    )

    return watering_train, watering_test, weather_train, weather_test, target_train, target_test

def create_model(watering_shape, weather_shape, output_shape):
    watering_input = Input(shape=watering_shape)
    weather_input = Input(shape=weather_shape)

    # LSTM layers for processing sequential watering features
    watering_lstm = LSTM(16, return_sequences=True)(watering_input)

    # LSTM layers for processing sequential weather features
    weather_lstm = LSTM(32, return_sequences=True)(weather_input)
    weather_lstm = LSTM(16, return_sequences=True)(weather_lstm)

    # Flatten and concatenate sequential inputs
    flat_watering_lstm = tf.keras.layers.Flatten()(watering_lstm)
    flat_weather_lstm = tf.keras.layers.Flatten()(weather_lstm)
    concatenated_inputs = concatenate([flat_watering_lstm, flat_weather_lstm])

    # Dense layers for final predictions
    dense1 = Dense(64, activation='relu')(concatenated_inputs)
    output = Dense(output_shape, activation='relu')(dense1)

    # Create the model
    model = Model(inputs=[watering_input, weather_input], outputs=output)

    return model
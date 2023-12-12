import pandas as pd
from keras.models import load_model
import ml_utils

data_path = "../database"
watering_path = f"{data_path}/watering.csv"
weather_path = f"{data_path}/weather.csv"
sensor_path = f"{data_path}/sensors.csv"
weather_descriptions_path = '../configs/weather_descriptions.json'
model_path = "../models"

# Get data:
watering_df = pd.read_csv(watering_path)
weather_df = pd.read_csv(weather_path)
weather_df = pd.concat([weather_df, pd.DataFrame({"zone":["all"]*len(weather_df)})], axis=1)
sensor_df = pd.read_csv(sensor_path)

# Extract features from data:
watering_feats = ml_utils.get_watering_feats(watering_df, 0, len(watering_df[watering_df["zone"]=="A"]))
weather_feats = ml_utils.get_weather_feats(weather_df, 0, len(weather_df), list(sensor_df["zone"].unique()), weather_descriptions_path)
target_feat = ml_utils.get_target_feat(sensor_df, 0, len(sensor_df[sensor_df["zone"] == "A"]))

# Get Pretrained model
pre_trained_model = load_model('../models/base_model.h5')
pre_trained_model.compile(optimizer='adam', loss='mean_squared_error')  # Same as above

# Finetune for each zone
for zone in sensor_df["zone"].unique():

    watering_train, watering_test, weather_train, weather_test, target_train, target_test = ml_utils.create_training_data(watering_feats, weather_feats, target_feat, zone)

    # Train Model
    pre_trained_model.fit([watering_train, weather_train], target_train, epochs=10, batch_size=32, validation_data=([watering_test, weather_test], target_test))

    # Save Model
    pre_trained_model.save(f'{model_path}/{zone}_model.h5')
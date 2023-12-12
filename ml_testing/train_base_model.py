import ml_utils
import pandas as pd

data_path = "../demo_data"
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

# Create training and testing data:
zone = "A" # arbitrary, since this is supposed to represent original training data
watering_train, watering_test, weather_train, weather_test, target_train, target_test = ml_utils.create_training_data(watering_feats, weather_feats, target_feat, zone)

# Create and train LSTM model:
model = ml_utils.create_model((5,2), (40,45), (360))
model.compile(optimizer='adam', loss='mean_squared_error')
model.fit([watering_train, weather_train], target_train, epochs=10, batch_size=32, validation_data=([watering_test, weather_test], target_test))

# Save Model
model.save(f"{model_path}/base_model.h5")
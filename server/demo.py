#!/usr/bin/env python3

import time
import json
import requests
controller_ip = '10.7.169.198'  # Replace with your Arduino's IP address
# Encode the data in the URL
def data2req(data_dict):
    encoded_data = '&'.join([f'{key}={value}' for key, value in data_dict.items()])
    return f'http://{controller_ip}/?{encoded_data}'
def retrieve_weather():
    get_current_weather()
    print("Retrieved weather data!")
def request_sensor_data(zone):
    print(f"Requesting sensor data from Zone {zone}!")
    message = {"id":"SERV", "packet number":0, "type":"request", "request_type":"sensor", "zone":zone}
    url = data2req(message)
    response = requests.get(url)
    # Check the response
    print(f'Status Code: {response.status_code}')
    print('Response Content:')
    print(response.text)
# sends scheduled watering control messages to controller
def send_scheduled_watering_msg(zone, amount):
    print(f"Watering Zone {zone} with {amount} L");
    message = {"id":"SERV", "packet number":0, "type":"request", "request_type":"solenoid", "zone":zone, "amount":0.25}
    url = data2req(message)
    response = requests.get(url)
    # Check the response
    print(f'Status Code: {response.status_code}')
    print('Response Content:')
    print(response.text)
print("##############################################")
print("")
print("                 LAWNLOGIC                    ")
print("")
print("##############################################")
print("")
print("Test One: Getting Initial sensor data")
print("")
request_sensor_data("A")
print("")
request_sensor_data("B")
print("")
print("Test Two: Water the grass")
send_scheduled_watering_msg("A", 0.5)
time.sleep(5)
send_scheduled_watering_msg("B", 0.5)
print("")
print("Track Sensor Values")
for i in range(5):
    request_sensor_data("A")
    print("")
    request_sensor_data("B")
    print("")
    time.sleep(5)

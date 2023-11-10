#!/bin/usr/env 

'''Template for the code taken from https://www.geeksforgeeks.org/python-find-current-weather-of-any-city-using-openweathermap-api/

Changed for project by @lcsaszar

'''

# import required modules
import requests, time, os
flag = 1
    
def forcast(whattime):
	# Enter your API key here
	api_key = "b3cdaf5851aca17e278e72476ff2ed93"

	# base_url variable to store url
	base_url = "http://api.openweathermap.org/data/2.5/weather?"

	# Give city name
	city_name = "South Bend"

	# complete_url variable to store
	# complete url address
	complete_url = base_url + "appid=" + api_key + "&q=" + city_name

	# get method of requests module
	# return response object
	response = requests.get(complete_url)

	# json method of response object 
	# convert json format data into
	# python format data
	x = response.json()
	#print(x,"\n")

	# Now x contains list of nested dictionaries
	# Check the value of "cod" key is equal to
	# "404", means city is found otherwise,
	# city is not found
	if x["cod"] != "404":

		# store the value of "main"
		# key in variable y
		y = x["main"]

		# store the value corresponding
		# to the "temp" key of y
		current_temperature = y["temp"]

		ftemp = int(((float(current_temperature))-273.15)*1.8+32)#Converts to fahrenheit
		
		# store the value corresponding
		# to the "pressure" key of y
		current_pressure = y["pressure"]

		# store the value corresponding
		# to the "humidity" key of y
		current_humidity = y["humidity"]	

		# store the value of "weather"
		# key in variable z
		z = x["weather"]

		# store the value corresponding 
		# to the "description" key at 
		# the 0th index of z
		weather_description = z[0]["description"]

		tobj = time.localtime(whattime)
		time_str = time.asctime(tobj)
  
		time_str.replace(" ","--")

		curdir = os.path.dirname(__file__)
		head, tail = os.path.split(curdir)

		fd = open(head+"/server/history/"+time_str[0:3]+"_"+time_str[4:7]+"_"+time_str[8:10]+".txt", "a+")
  
		forcast_str = {time_str: [str(ftemp), str(current_pressure),str(current_humidity), weather_description]}
		fd.write(str(forcast_str)+"\n")
		fd.close()

	else:
		print(" City Not Found ")
 
while(flag != 0):
    time.sleep(1800)
    whattime = time.time()
    forcast(whattime)
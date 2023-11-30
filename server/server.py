from flask import Flask, render_template, request
from weather import forcast
from markupsafe import escape
from markupsafe import Markup

app = Flask(__name__)

@app.route('/')
@app.route('/index')
def index(name=None):
    return render_template('index.html')

@app.route('/weather')
def get_weather():
    city = request.args.get('city')
    weather_data = forcast()
    return render_template(
        "weather.html",
        
        status=weather_data[0]["discrip[tion]"].capitalize(),
        temp=f"{weather_data['main']['temp']:.1f}",
        feels_like=f"{weather_data['main']['feels_like']:.1f}"
    )


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000)   
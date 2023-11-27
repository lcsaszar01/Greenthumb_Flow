from flask import Flask, render_template
from flask import url_for
from flask import render_template
from markupsafe import escape
from markupsafe import Markup
from weather import forcast 



app = Flask(__name__)

@app.route('/home/')
@app.route('/home/<name>')
def home(name=None):
    url_for('static', filename='stylesheets/site.css')
    return render_template('home.html', name=name)


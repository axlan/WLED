import os
import json

from flask import Flask, render_template, Response, redirect, request

app = Flask(__name__)

center = {
        'lat': 37.7656809,
        'lng': -122.3957522,
        'range': 100.0,
        'zoom': 11.0,
    }

@app.route('/get_map_params')
def get_map_params():
    return Response(json.dumps(center), mimetype='application/json')

@app.route('/set_center', methods=['POST'])
def set_center():
    print(request.form)
    center['lat'] = request.form['lat']
    center['lng'] = request.form['lng']
    return Response(status=200)

@app.route('/')
def hello():
    return redirect('/static/index.html')

if __name__ == '__main__':
    app.run(debug=True)

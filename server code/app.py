from flask import Flask, jsonify, render_template, render_template_string, request
import datetime
import pandas as pd
import json
import plotly
import plotly.express as px

app = Flask(__name__)

tempMin = 999
tempMax = 0
humidMin = 999
humidMax = 0
initialized = 0
apneaCounter = 0
lastBreathTimestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
lastPiezoReading = 0
apnea_flag = False

@app.route("/update", methods=["GET"])
def update():
    global initialized
    
    # ========== UPDATE STATS HERE TOO ==========
    global tempMin
    if (initialized == 1): tempMin = min(tempMin, float(persistent_vals[-1][3]))
    global humidMin
    if (initialized == 1): humidMin = min(humidMin, float(persistent_vals[-1][2]))

    if (initialized == 0):
        initialized = 1

    global tempMax
    tempMax = max(tempMax, float(persistent_vals[-1][3]))
    global humidMax
    humidMax = max(humidMax, float(persistent_vals[-1][2]))
    
    tempVals = [float(x[3]) for x in persistent_vals[1:]]
    if len(tempVals) > 0:
      tempAvg = sum(tempVals) / len(tempVals)
    else:
      tempAvg = 0

    humidVals = [float(x[2]) for x in persistent_vals[1:]]
    if len(humidVals) > 0:
      humidAvg = sum(humidVals) / len(humidVals)
    else:
      humidAvg = 0

    global apneaCounter
    global lastBreathTimestamp
    global lastPiezoReading
    global apnea_flag
    if (lastPiezoReading >= 700) and int(persistent_vals[-1][1]) < 700:
        lastBreathTimestamp = persistent_vals[-1][0]
        apnea_flag = True

    interval = (datetime.datetime.strptime(persistent_vals[-1][0], '%Y-%m-%d %H:%M:%S') -\
        datetime.datetime.strptime(lastBreathTimestamp, '%Y-%m-%d %H:%M:%S')).seconds
    print(f"TESTING: {interval}")

    now = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    if apnea_flag and (datetime.datetime.strptime(now, '%Y-%m-%d %H:%M:%S') -\
        datetime.datetime.strptime(lastBreathTimestamp, '%Y-%m-%d %H:%M:%S')).seconds >= 10 and\
            int(persistent_vals[-1][1]) < 700:
        apneaCounter += 1
        apnea_flag = False
    
    apneaSeverity = "Benign (0-4)"

    if 5 <= apneaCounter < 15:
        apneaSeverity = "Mild (5-14)"
    elif 15 <= apneaCounter <= 30:
        apneaSeverity = "Moderate (15-30)"
    elif 30 < apneaCounter:
        apneaSeverity = "Severe (31+)"

    lastPiezoReading = int(persistent_vals[-1][1])

    return jsonify((datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            persistent_vals[-1][1], persistent_vals[-1][2], persistent_vals[-1][3], persistent_vals[-1][4], 
            persistent_vals[-1][5], persistent_vals[-1][6], persistent_vals[-1][7], 
            persistent_vals[-1][8], persistent_vals[-1][9], persistent_vals[-1][10], persistent_vals[-1][11],
            tempMin, tempMax, tempAvg, humidMin, humidMax, humidAvg, apneaCounter, apneaSeverity))


def chart_appearance(fig):
    fig.update_xaxes(title_font_color="white", tickfont_color="white", gridcolor="#3c3c3c")
    fig.update_yaxes(title_font_color="white", tickfont_color="white", gridcolor="#3c3c3c")
    fig.update_traces(line_color="#49cc56", line_width=3)
    fig.update_layout({'paper_bgcolor': 'rgba(0,0,0,0)', 'title_font_color': 'rgb(255,255,255)', 'plot_bgcolor': 'rgb(0,0,0)'})
    return fig

persistent_vals = [(datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'),0,0,0,0,0,0,0,0,0,0,0)]
@app.route("/")
def rootpage():
    temp = request.args.get("temp")
    humid = request.args.get("humid")
    piezo = request.args.get("piezo")
    beat_avg = request.args.get("beatAvg")
    spo2 = request.args.get("spo2")
    acc_x = request.args.get("acc_x")
    acc_y = request.args.get("acc_y")
    acc_z = request.args.get("acc_z")
    gyro_x = request.args.get("gyro_x")
    gyro_y = request.args.get("gyro_y")
    gyro_z = request.args.get("gyro_z")
    datawave = (datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                piezo, humid, temp, beat_avg, 
                acc_x, acc_y, acc_z, 
                gyro_x, gyro_y, gyro_z, spo2)
    if None not in datawave:
        persistent_vals.append(datawave)
        if len(persistent_vals) > 1000:
            persistent_vals.pop(0)

    # ========== MAKE GRAPHS HERE ==========
    piezodf = pd.DataFrame(data={'Time': [x[0] for x in persistent_vals], 'Reading': [float(x[1]) for x in persistent_vals]})
    fig = chart_appearance(px.line(piezodf, x="Time", y="Reading", title="Respiration Monitor"))
    fig.add_hline(y=700, line_width=3, line_dash="dash", line_color="red")
    piezoJSON = json.dumps(fig, cls=plotly.utils.PlotlyJSONEncoder)

    tempdf = pd.DataFrame(data={'Time': [x[0] for x in persistent_vals], 'Temperature (°C)': [float(x[3]) for x in persistent_vals]})
    fig = chart_appearance(px.line(tempdf, x="Time", y="Temperature (°C)", title="Ambient Temperature"))
    tempJSON = json.dumps(fig, cls=plotly.utils.PlotlyJSONEncoder)
    
    humiddf = pd.DataFrame(data={'Time': [x[0] for x in persistent_vals], 'Humidity %': [float(x[2]) for x in persistent_vals]})
    fig = chart_appearance(px.line(humiddf, x="Time", y="Humidity %", title="Ambient Humidity"))
    fig.add_hline(y=60, line_width=3, line_dash="dash", line_color="red")
    humidJSON = json.dumps(fig, cls=plotly.utils.PlotlyJSONEncoder)

    bpmdf = pd.DataFrame(data={'Time': [x[0] for x in persistent_vals], 'BPM': [float(x[4]) for x in persistent_vals]})
    fig = chart_appearance(px.line(bpmdf, x="Time", y="BPM", title="Heartrate Monitor"))
    bpmJSON = json.dumps(fig, cls=plotly.utils.PlotlyJSONEncoder)
    # ======================================

    return render_template('dashboard.html', 
    piezoJSON=piezoJSON,
    tempJSON=tempJSON,
    humidJSON=humidJSON,
    bpmJSON=bpmJSON,
    temp=persistent_vals[-1][3], humid=persistent_vals[-1][2], piezo=persistent_vals[-1][1], 
    beat_avg=persistent_vals[-1][4], acc_x=persistent_vals[-1][5], acc_y=persistent_vals[-1][6],
    acc_z=persistent_vals[-1][7], gyro_x=persistent_vals[-1][8], gyro_y = persistent_vals[-1][9], gyro_z = persistent_vals[-1][10], spo2=persistent_vals[-1][11])
from flask import Flask,render_template,jsonify
import random

app = Flask(__name__)


@app.route('/hello',methods = ['POST','GET'])
def index():
    return render_template('baidumap.html')

@app.route('/get/json_data',methods = ['POST','GET'])
def get_json_data():
    data = get_data()
    return jsonify(data)

def get_data():
    file_object = open(r'C:\Users\Administrator\Desktop\receive')
    readline_data = file_object.readlines()
    last=readline_data[-1]
    stringToSplit=last.split(' ')
    longitude=stringToSplit[0].split(':')[1]
    latitude=stringToSplit[2].split(':')[1]
    #print("longitude:",longitude)
    #print("latitude:",latitude)
    #random_data = random.uniform(0,0.0005)
    data = {'t':0,'s':0}
    #print(random_data)
    data['t'] = longitude
    data['s'] = latitude
    return data

if __name__ == '__main__':
    app.run(host = '0.0.0.0',port = 3090)

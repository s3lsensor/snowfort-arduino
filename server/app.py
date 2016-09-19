from flask import Flask, request, jsonify, session
import time, datetime
import collections
import os
from ctypes import *

app = Flask(__name__)

log_file = "log.txt"
# Generate a secret random key for the session
app.secret_key = os.urandom(24)

@app.route('/')
def main():
    return "Welcome!"

def str2num_unsigned(s):
    num = 0
    for c, i in zip(s, range(0,len(s),1)):
        num = num + (c_ulong(ord(c)).value << 8*i)

    return float(num)

def str2num(s):
    num = str2num_unsigned(s)

    if len(s) == 2 and num > (2**15-1):
        num = num - 2**16

    return float(num)

def dump_mpu6050(val):
    val_list = [];
    val_list.append(str2num_unsigned(val[:4]))
    val_list.append(str2num(val[4:6]))
    val_list.append(str2num(val[6:8]))
    val_list.append(str2num(val[8:10]))
    val_list.append(str2num(val[10:12]))
    val_list.append(str2num(val[12:14]))
    val_list.append(str2num(val[14:16]))
    val_list.append(str2num(val[16:18]))

    return val_list


@app.route('/data', methods=['POST'])
def upload_data():
    #request.get_data()
    # print request.headers
    # print "="*10
    #print request.stream.read()
    # print request.form
    # print dict(request.form)
    current_time = unicode(datetime.datetime.now())
    print request.headers
    data = request.get_data()
    data_list = data.split('&&')
    # print data_list

    num_value = data_list[1]
    RSSI = data_list[2]
    value_list = data_list[0].split('||')
    value_list = value_list[:-1]
    avg_value_len = sum(len(x) for x in value_list)/(len(value_list))
    print num_value, len(value_list), avg_value_len, RSSI

    val_list = []
    for val in value_list:
        val = val.replace("D=",'')
        val_list.append(dump_mpu6050(val))
        # print [str2num(v) for v in val[:6]]

    print val_list[0]


    # value = request.form.getlist("D")[0]
    # num_value = request.form.getlist("N")[0]
    # ip_addr = request.remote_addr
    # # print ip_addr, request.get_data()
    # print value
    # print ip_addr, num_value, len(value.split("||"))

    #print value.split(",")
    # data_list = value.split("|")
    # data_list = data_list[:-1]
    # #print num_value, len(data_list)
    # print num_value, sum(len(d) for d in data_list)

    # if len(data_list) != int(num_value):
    #     response = {}
    #     return jsonify(response), 422

    # with open(log_file, "a") as fp:
    #     for d in data_list:
    #         dd = d.split(",")
    #         xx = [current_time, ip_addr] + dd
    #         fp.write(",".join(xx)+"\n")

    response = {}

    return jsonify(response), 200

# @app.after_request
# def after(response):
#   # todo with response
#   print response.status
#   print response.headers
#   print response.get_data()
#   return response

@app.before_request
def make_session_permanent():
    session.permanent = True
    app.permanent_session_lifetime = datetime.timedelta(seconds=10)


if __name__ == "__main__":
    app.run(host="0.0.0.0", debug=True)

from flask import Flask, request, jsonify, session, render_template
from flask import send_from_directory
import time, datetime
import collections
import os
from ctypes import *
import logging
from logging import FileHandler
import struct, sys

from itertools import izip_longest



debug = True
NUM_VARIABLE = 5
PAYLOAD_LEN = 18

app = Flask(__name__, static_folder='static')

def remove_file(fn):
    try:
        os.remove(fn)
    except OSError:
        pass


log_file = "./data/data.csv"
raw_log_file = "./data/data_raw.txt"
network_log_file = "./data/network.csv"
AR_log_file = "./data/AR.csv"
AR_raw_log_file = "./data/AR_raw.txt"

remove_file(log_file)
remove_file(raw_log_file)
remove_file(network_log_file)
remove_file(AR_log_file)

# Generate a secret random key for the session
app.secret_key = os.urandom(24)

status_list = []
status_list_order = dict()
network_status = dict()

# @app.route('/js/<path:filename>')
# def serve_static(filename):
#     root_dir = os.path.dirname(os.getcwd())
#     return send_from_directory(os.path.join(root_dir, 'static', 'js'), filename)

@app.route('/')
def main():
    return render_template("index.html", status_list=status_list)

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
    current_time_dt = datetime.datetime.now()
    current_time = unicode(current_time_dt)
    content_length = request.headers['Content-Length']
    request_data = request.get_data()
    ip_addr = request.remote_addr


    # log raw data
    with open(raw_log_file, "a") as fp:
        fp.write(current_time+",")
        fp.write(ip_addr+",")
        fp.write(request_data+"\n\n")

    # split data by delimiter
    request_data_list = request_data.split('&&&')

    # check number of payloads
    if len(request_data_list) == NUM_VARIABLE:
        num_payload = request_data_list[1].replace("N=",'')
        RSSI = request_data_list[2].replace("R=",'')
        TX_counter = request_data_list[3].replace("I=",'')
        mac_addr = request_data_list[4].replace("M=",'')
        mac_addr = "f8:f0:05:f5:"+mac_addr[:2]+":"+mac_addr[2:]

        payload_list = request_data_list[0].split('||')
        payload_list = payload_list[:-1]
        avg_payload_len = sum(len(x) for x in payload_list)/(len(payload_list))

    elif len(request_data_list) > NUM_VARIABLE:
        print("I am here")
        # print request_data_list
        payload_list = request_data_list[0]
        for d in request_data_list[1:]:
            if d.startswith("N="):
                num_payload = d
                break
            else:
                payload_list += d

        for d in request_data_list[1:]:
            if d.startswith("R="):
                RSSI = d
                break
            else:
                num_payload += d

        for d in request_data_list[1:]:
            if d.startswith("I="):
                TX_counter = d
                break
            else:
                RSSI += d

        for d in request_data_list[1:]:
            if d.startswith("M="):
                mac_addr = d
                break
            else:
                TX_counter += d

        for d in request_data_list[1:]:
            if d.startswith("A="):
                AR_coeff = d
                break
            else:
                mac_addr += d

        payload_list = payload_list.split('||')
        payload_list = payload_list[:-1]

        num_payload = num_payload.replace("N=","")
        RSSI = RSSI.replace("R=","")
        TX_counter = TX_counter.replace("T=","")
        avg_payload_len = sum(len(x) for x in payload_list)/(len(payload_list))
        mac_addr = "f8:f0:05:f5:"+mac_addr[:2]+":"+mac_addr[2:]
    else:
        app.logger.error("{}: expect 5 variables in packet, find {} variables".format(ip_addr, len(request_data_list)))
        response = {"Incorrect packet variable": "True"}
        return jsonify(response), 422


    sensor_value_list = []
    time_vec = []
    time_raw_vec = []
    if len(payload_list) != int(num_payload):
        app.logger.error("{}:packet {}: expect {} values, find {} values".format(mac_addr,TX_counter,num_payload,str(len(payload_list))))

    val_idx = 0
    for payload in payload_list:
        if val_idx == 0:
            payload = payload.replace("D=",'')

        if len(payload) == PAYLOAD_LEN:
            DD = dump_mpu6050(payload)
            sensor_value_list.append(DD)
            time_vec.append(DD[0])
        else:
            print TX_counter, len(payload_list), val_idx, len(payload)
            print [str2num_unsigned(s) for s in payload]
            print request_data_list
            print payload_list
            app.logger.warning("{}:packet {}: expect {} elements, find {} elements".format(mac_addr,TX_counter,PAYLOAD_LEN,str(len(payload))))

        val_idx = val_idx + 1


    start_time = min(time_vec)
    end_time = max(time_vec)

    #print ip_addr, val_list[0]
    app.logger.info("system information: %s, %s, %s, %d, %s %s" % (ip_addr, mac_addr, num_payload, len(sensor_value_list), RSSI, TX_counter))
    print "%s, %s, %s, %d, %s %s" % (ip_addr, mac_addr, num_payload, len(sensor_value_list), RSSI, TX_counter)

    if ip_addr not in status_list_order:
        status_list_order[ip_addr] = len(status_list)
        # pack first directory
        item = dict()
        item["ip_addr"] = ip_addr
        item["RX_value"] = int(num_payload)
        item["RSSI"] = int(RSSI)
        item["last_TX_counter"] = int(TX_counter)
        item["total_TX_counter"] = 1
        item["total_miss_counter"] = 0
        item["missing_packet_counter"] = round(float(item["total_miss_counter"])/item["total_TX_counter"]*100.0,5)
        item["mac_addr"] = mac_addr
        time_diff = end_time-start_time
        if time_diff > 0:
            item["fs"] = round(float(num_payload)/(time_diff/1e3),5)

        item["repeat_TX"] = 0
        item["last_RX_time"] = current_time
        item["total_RX_samples"] = int(num_payload)
        status_list.append(item)
    else:
        item = status_list[status_list_order[ip_addr]]
        item["RX_value"] = int(num_payload)
        item["RSSI"] = int(RSSI)
        item["total_TX_counter"] = item["total_TX_counter"]+1
        packet_diff = int(TX_counter) - item["last_TX_counter"] - 1
        if packet_diff > 0:
            app.logger.warning("Packet missing %d, %d, %d" % (int(TX_counter), item["last_TX_counter"], packet_diff))
            item["total_miss_counter"] = item["total_miss_counter"] + packet_diff
        elif packet_diff == -1:
            item['repeat_TX'] = item['repeat_TX'] + 1
        else:
            item["total_RX_samples"] = item["total_RX_samples"] + int(num_payload)

        item["missing_packet_counter"] = round(float(item["total_miss_counter"])/item["total_TX_counter"]*100.0,5)
        item["last_TX_counter"] = int(TX_counter)
        time_diff = end_time-start_time
        if time_diff > 0:
            item["fs"] = round(float(num_payload)/(time_diff/1e3),5)

        item["last_RX_time"] = current_time

        status_list[status_list_order[ip_addr]] = item

    if ip_addr not in network_status:
        network_status[ip_addr] = [current_time_dt]
    else:
        temp_list = network_status[ip_addr]
        if len(temp_list) > 40:
            temp_list.pop(0)

        temp_list.append(current_time_dt)
        network_status[ip_addr] = temp_list


    item = status_list[status_list_order[ip_addr]]
    dt = max(network_status[ip_addr]) - min(network_status[ip_addr])
    item["RX_speed"] = dt.total_seconds()/len(network_status[ip_addr])
    status_list[status_list_order[ip_addr]] = item


    # if len(payload_list) != int(num_payload):
    #     app.logger.warning("Mismatch: %d, %d" % (len(payload_list), int(num_payload)))
    #     response = {"mismatch":"True"}
    #     return jsonify(response), 422

    prefix = [current_time, ip_addr, mac_addr, num_payload, RSSI, TX_counter, content_length]
    with open(network_log_file, "a") as fp:
        fp.write(",".join(prefix)+"\n")

    prefix = [current_time, mac_addr, TX_counter]
    with open(log_file, "a") as fp:
        for d in sensor_value_list:
            xx = prefix + [str(x) for x in d]
            fp.write(",".join(xx)+"\n")


    response = {}

    return jsonify(response), 200

def grouper(iterable, n, fillvalue=None):
    args = [iter(iterable)] * n
    return izip_longest(*args, fillvalue=fillvalue)

def dump_AR(AR_array):
    val_list = [];
    for val_tuple in grouper(AR_array, 4):
        val = "".join(val_tuple)
        val_float = struct.unpack('f', val)
        val_list.append(val_float)

    return val_list

@app.route('/AR', methods=['POST'])
def AR_coeff():
    current_time_dt = datetime.datetime.now()
    current_time = unicode(current_time_dt)
    content_length = request.headers['Content-Length']
    request_data = request.get_data()
    ip_addr = request.remote_addr

    with open(AR_raw_log_file,"a") as fp:
        fp.write(current_time+",")
        fp.write(ip_addr+",")
        fp.write(request_data+"\n\n")

    # split data by delimiter
    request_data_list = request_data.split('&&&')

    # check number of payloads
    if len(request_data_list) == 3:
        TX_counter = request_data_list[1].replace("I=",'')
        mac_addr = request_data_list[2].replace("M=",'')
        mac_addr = "f8:f0:05:f5:"+mac_addr[:2]+":"+mac_addr[2:]

        AR_data = request_data_list[0]
    else:
        response = {"AR_missing_match": "True"}
        return jsonify(status), 422

    if ip_addr not in status_list_order:
        status_list_order[ip_addr] = len(status_list)
        # pack first directory
        item = dict()
        item["ip_addr"] = ip_addr
        item["last_TX_counter"] = int(TX_counter)
        item["total_TX_counter"] = 1
        item["total_miss_counter"] = 0
        item["missing_packet_counter"] = round(float(item["total_miss_counter"])/item["total_TX_counter"]*100.0,5)
        item["mac_addr"] = mac_addr
        item["repeat_TX"] = 0
        item["last_RX_time"] = current_time
        status_list.append(item)
    else:
        item = status_list[status_list_order[ip_addr]]
        item["total_TX_counter"] = item["total_TX_counter"]+1
        packet_diff = int(TX_counter) - item["last_TX_counter"] - 1
        if packet_diff > 0:
            app.logger.warning("Packet missing %d, %d, %d" % (int(TX_counter), item["last_TX_counter"], packet_diff))
            item["total_miss_counter"] = item["total_miss_counter"] + packet_diff
        elif packet_diff == -1:
            item['repeat_TX'] = item['repeat_TX'] + 1


        item["missing_packet_counter"] = round(float(item["total_miss_counter"])/item["total_TX_counter"]*100.0,5)
        item["last_TX_counter"] = int(TX_counter)
        item["last_RX_time"] = current_time

        status_list[status_list_order[ip_addr]] = item

    if ip_addr not in network_status:
        network_status[ip_addr] = [current_time_dt]
    else:
        temp_list = network_status[ip_addr]
        if len(temp_list) > 40:
            temp_list.pop(0)

        temp_list.append(current_time_dt)
        network_status[ip_addr] = temp_list


    item = status_list[status_list_order[ip_addr]]
    dt = max(network_status[ip_addr]) - min(network_status[ip_addr])
    item["RX_speed"] = dt.total_seconds()/len(network_status[ip_addr])
    status_list[status_list_order[ip_addr]] = item

    # decode AR coeff
    AR_list = dump_AR(AR_data.replace("A=",''))

    prefix = [current_time, mac_addr, TX_counter]
    if len(AR_list) > 0:
        AR_order = len(AR_list)/6
        AR_str = [str(x[0]) for x in AR_list]
        with open(AR_log_file, "a") as fp:
            idx = 0
            for AR_val in grouper(AR_str, AR_order):
                AR_val = prefix + [str(idx)] + [x for x in AR_val]
                fp.write(",".join(AR_val)+"\n")
                idx += 1



    response = {}
    return jsonify(response), 200


@app.before_request
def make_session_permanent():
    session.permanent = True
    app.permanent_session_lifetime = datetime.timedelta(seconds=2)


if __name__ == "__main__":
    hdlr = FileHandler('./snowfort.log')
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    hdlr.setLevel(logging.DEBUG)
    hdlr.setFormatter(formatter)
    app.logger.addHandler(hdlr)

    ch = logging.StreamHandler(sys.stdout)
    ch.setLevel(logging.DEBUG)
    ch.setFormatter(formatter)
    app.logger.addHandler(ch)

    app.run(host="0.0.0.0", debug=debug)

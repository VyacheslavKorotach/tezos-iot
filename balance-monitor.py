from pytezos import pytezos

import os
import json
import time
import paho.mqtt.client as mqtt

topic_sub1 = 'tezos-iot/hackathon/device0001/state'
topic_pub1 = 'tezos-iot/hackathon/device0001/ctl'

tezos_endpoint = 'mainnet'

mqtt_host = 'korotach.com'
mqtt_user = 'igor'
mqtt_password = 'igor1315'
# device0001_account = 'tz1hs2Zf8uRhUxoTC4vwHL7Xk7k8KCfujHAY'
device0001_account = 'tz1Uuawvcr9HQDt8oWcrYdZkzgPqnA38FUD7'
# active_privat_key = os.environ['DEVICE_ACCOUNT_PRIVAT_KEY']
price = 888  # microtez
state = 'Start'
debug = True
goods_number = 0


def on_connect(mosq, obj, flags, rc):
    mqttc.subscribe(topic_sub1, 0)
    print("rc: " + str(rc))


def on_message(mosq, obj, msg):
    """
    get the status string from device
    {"recv_sequence": 32, "status": "OK"} or {"recv_sequence": 32, "status": "Error"}
    or {"status": "Restart"}
    """
    global state
    print(msg.topic + " " + str(msg.qos) + " " + str(msg.payload))
    json_string = ''
    d = {}
    try:
        json_string = msg.payload.decode('utf8')
    except UnicodeDecodeError:
        print("it was not a ascii-encoded unicode string")
    if debug: print('json_string = ', json_string)
    if json_string != '' and is_json(json_string):
        d = json.loads(json_string)
        if 'status' in d.keys():
            if d['status'].find('OK') != -1 \
                    and 'recv_sequence' in d.keys() and d['recv_sequence'] == goods_number:
                state = 'we successfully have gave goods out'
            elif d['status'].find('Error') != -1:
                state = 'We have received the Error code from device.'
            elif d['status'].find('Restart') != -1:
                state = 'Restart'
            elif d['status'].find('Empty') != -1:
                state = 'Crypto-device is empty.'
                pass
            elif d['status'].find('Ready') != -1:
                state = 'Crypto-device is ready.'
                pass
            elif d['status'].find('Busy') != -1:
                state = 'Crypto-device is busy.'
                pass
            elif d['status'].find('NO CONNECT') != -1:
                state = 'NO CONNECT'
                pass
            else:
                state = 'We have received a wrong message from device. Stop crypto-pleaser.'
        else:
            state = 'We have received a wrong message from device. Stop crypto-pleaser.'


#    if debug: print('state = ', state)


def on_publish(mosq, obj, mid):
    print("mid: " + str(mid))


def on_subscribe(mosq, obj, mid, granted_qos):
    print("Subscribed: " + str(mid) + " " + str(granted_qos))


def on_log(mosq, obj, level, string):
    print(string)


def get_tezos_balance(account):
    msg = pytezos.account(account)
    if 'balance' in msg.keys():
        return msg['balance']
    else:
        return 0  # fix to 0


def is_json(myjson):
    try:
        json_object = json.loads(myjson)
    except ValueError:
        return False
    return True


def give_pleasure(pleasure_time):
    tst_start = int(time.time())
    if debug: print('pleasure time is: ', pleasure_time)
    mqttc.publish(topic_pub1, '{"pleasure_time": ' + str(pleasure_time) + ', "tst": ' + str(tst_start) + '}')
    time.sleep(2)


mqttc = mqtt.Client()
# Assign event callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_publish = on_publish
mqttc.on_subscribe = on_subscribe
# mqttc.on_log = on_log
mqttc.username_pw_set(mqtt_user, password=mqtt_password)
# Connect
mqttc.connect(mqtt_host, 1883, 60)
# Continue the network loop
# mqttc.loop_forever()
mqttc.loop_start()
time.sleep(1)
#device_balance = 0
device_balance = get_tezos_balance(device0001_account)
old_balance = device_balance

while True:

    while (int(old_balance) + int(price)) >= int(device_balance):
        state = 'Waiting for transaction.'
        device_balance = get_tezos_balance(device0001_account)
        if debug: print('device balance is: ' + str(device_balance) + ' microtez. ' + state)
        time.sleep(5)
    give_pleasure(5)

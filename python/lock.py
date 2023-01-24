import os
import json
import requests
from dotenv import load_dotenv
import argparse   

import utils

parser = argparse.ArgumentParser(description='Switchbot Lock') 
parser.add_argument('-l', '--lock', action='store_true', help='lock specified device') 
parser.add_argument('-u', '--unlock', action='store_true', help='unlock specified device')
parser.add_argument('-t', '--toggle', action='store_true', help='toggle specified device')
parser.add_argument('-d', '--device', help='device id', required=True)
args = parser.parse_args()  

base_url = 'https://api.switch-bot.com'
load_dotenv()
# tokenとsecretを貼り付ける
token = os.getenv("TOKEN") # copy and paste from the SwitchBot app V6.14 or later
secret = os.getenv("secret") # copy and paste from the SwitchBot app V6.14 or later
headers = utils.make_request_header(token,secret)

def read_lock_from_json(deviceListJson='deviceList.json') -> dict:
    lock = {}
    f = open(deviceListJson,"r",encoding="utf-8")
    jsonfile = json.load(f)
    devices = jsonfile["body"]["deviceList"]

    for device in devices:
        if device["deviceType"] == "Smart Lock":
            device_lock = device
    return device_lock

def get_lock_status(deviceId: str):
    devices_url = base_url + "/v1.1/devices/" + deviceId + "/status"
    try:
        # ロックの状態を取得
        res = requests.get(devices_url, headers=headers)
        res.raise_for_status()
        return res.json()

    except requests.exceptions.RequestException as e:
        print('response error:',e)

def lock(deviceId: str):
    devices_url = base_url + "/v1.1/devices/" + deviceId + "/commands"
    data={
            "commandType": "command",
            "command": "lock",
            "parameter": "default",
        }
    try:
        # ロック
        res = requests.post(devices_url, headers=headers, json=data)
        res.raise_for_status()
        print(res.text)

    except requests.exceptions.RequestException as e:
        print('response error:',e)

def unlock(deviceId: str):
    devices_url = base_url + "/v1.1/devices/" + deviceId + "/commands"
    data={
            "commandType": "command",
            "command": "unlock",
            "parameter": "default",
        }
    try:
        # アンロック
        res = requests.post(devices_url, headers=headers, json=data)
        res.raise_for_status()
        print(res.text)
        
    except requests.exceptions.RequestException as e:
        print('response error:',e)

if __name__ == "__main__":
    if not (args.lock == True or args.unlock==True):
        args.toggle = True
    else:
        args.toggle = False

    deviceId = args.device

    # デバイス一覧を取得/更新
    if args.toggle:
        utils.get_device_list()
        device = read_lock_from_json()
        lock_state = get_lock_status(deviceId)["body"]["lockState"]
        if lock_state == "unlocked":
            lock(deviceId)
        elif lock_state == "locked":
            unlock(deviceId)
    elif args.lock:
        lock(deviceId)
    elif args.unlock:
        unlock(deviceId)

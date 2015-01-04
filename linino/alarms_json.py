import json
import sys

# filename = "/root/cloudClock/alarms.json"
filename = "alarms.json"

if len(sys.argv) < 2:
    print("Usage: %s [number|time|hour|minute|frequency|radio|add|remove]" % (sys.argv[0],))

if (sys.argv[1] == "number"):
	json_data=open(filename)
	data = json.load(json_data)
	print len(data["Alarms"])
	json_data.close()

if (sys.argv[1] == "time"):
    json_data=open(filename)
    data = json.load(json_data)
    print data["Alarms"][int(sys.argv[2])]["time"]
    json_data.close()

if (sys.argv[1] == "hour"):
    json_data=open(filename)
    data = json.load(json_data)
    time_str = data["Alarms"][int(sys.argv[2])]["time"]
    time_array = time_str.split(":");
    print time_array[0]
    json_data.close()

if (sys.argv[1] == "minute"):
    json_data=open(filename)
    data = json.load(json_data)
    time_str = data["Alarms"][int(sys.argv[2])]["time"]
    time_array = time_str.split(":");
    print time_array[1]
    json_data.close()

if (sys.argv[1] == "frequency"):
    json_data=open(filename)
    data = json.load(json_data)
    print data["Alarms"][int(sys.argv[2])]["frequency"]
    json_data.close()

if (sys.argv[1] == "radio"):
    json_data=open(filename)
    data = json.load(json_data)
    print data["Alarms"][int(sys.argv[2])]["radio"]
    json_data.close()

if (sys.argv[1] == "add"):
    json_data=open(filename, "r")
    data = json.load(json_data)
    json_data.close()
    json_data=open(filename, "w+")
    data["Alarms"].append({"time": sys.argv[2], "frequency": sys.argv[3], "radio": int(sys.argv[4])})
    json.dump(data, json_data)
    json_data.close()

if (sys.argv[1] == "remove"):
    json_data=open(filename, "r")
    data = json.load(json_data)
    json_data.close()
    json_data=open(filename, "w+")
    data["Alarms"].pop(int(sys.argv[2]))
    json.dump(data, json_data)
    json_data.close()
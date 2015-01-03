import json
import sys

# filename = "/root/cloudClock/radios.json"
filename = "radios.json"

if len(sys.argv) < 2:
    print("Usage: %s command" % (sys.argv[0],))

if (sys.argv[1] == "number"):
	json_data=open(filename)
	data = json.load(json_data)
	print len(data["Radios"])
	json_data.close()

if (sys.argv[1] == "name"):
    json_data=open(filename)
    data = json.load(json_data)
    print data["Radios"][int(sys.argv[2])]["Name"]
    json_data.close()

if (sys.argv[1] == "URL"):
    json_data=open(filename)
    data = json.load(json_data)
    print data["Radios"][int(sys.argv[2])]["URL"]
    json_data.close()

if (sys.argv[1] == "add"):
    json_data=open(filename, "r")
    data = json.load(json_data)
    json_data.close()
    json_data=open(filename, "w+")
    data["Radios"].append({"Name": sys.argv[2], "URL": sys.argv[3]})
    json.dump(data, json_data)
    json_data.close()

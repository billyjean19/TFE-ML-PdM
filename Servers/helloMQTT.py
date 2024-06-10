# Launch mosquitto broker: sudo systemctl restart mosquitto.service
# Check that mqtt exists on Internet port connections: netstat -at 
# adapt .config file in order to enable multpiple listeners on the local broker 
# see: https://mosquitto.org/man/mosquitto-conf-5.html 

# To run public server in command line: python helloMQTT.py
# @Return: a csv file containing all recordings made during the day


import paho.mqtt.client as mqtt
import csv
from datetime import datetime

mqtt_broker = "0.0.0.0" # Listen on all interfaces
mqtt_port = 1883        # Port to listen on (default for MQTT)

def on_connect(client, userdata, flags, rc, prop):
    print("Connected with result code :"+str(rc))
    client.subscribe("sensing_data_topic")

def on_message(client, userdata, msg):
    # print("Received message on topic "+ msg.topic+" : "+str(msg.payload))

    data = msg.payload.decode("utf-8")

    # Assuming data is comma-separated values
    data_list = data.split(',')
    
    # Generate CSV file name with current date
    current_date = datetime.now().strftime('%Y-%m-%d')
    csv_filename = f'data_MQTT_{current_date}.csv'
            
    # Define column names
    fieldnames = ['sensor_name', 'timestamp', 'acceleration_x', 'acceleration_y', 'acceleration_z', 'acceleration_magnitude']
    
    # Write data to CSV file with column names
    with open(csv_filename, 'a', newline='') as csvfile:
        csv_writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
                
        # Write header if file is newly created
        if csvfile.tell() == 0:
            csv_writer.writeheader()
                
        # Prepare data dictionary
        data_dict = {
            'sensor_name': data_list[0],
            'timestamp': data_list[1],
            'acceleration_x': data_list[2],
            'acceleration_y': data_list[3],
            'acceleration_z': data_list[4],
            'acceleration_magnitude': data_list[5]
        }
                
        # Write data to CSV file
        csv_writer.writerow(data_dict)


client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message

client.connect(mqtt_broker, mqtt_port, 60)

# Start the MQTT client loop
client.loop_forever()

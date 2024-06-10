# To run public server in cmd line: python helloUDP.py
# @Return: a csv file containing all recordings made during the day

import socket
import csv
from datetime import datetime

HOST = '0.0.0.0'  # Listen on all interfaces
PORT = 5000       # Port to listen on (same as ESP32 side)
DATA_SIZE = 256

def handle_udp():
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.bind((HOST, PORT))
        print("UDP server listening on port", PORT)
        while True:
            rawData, addr = s.recvfrom(DATA_SIZE)
            data = rawData.decode('utf-8')
            
            #print("Received message:", data, "from", addr)
            #print("Data list length:", len(data))

            # Assuming data is comma-separated values
            data_list = data.split(',')
            
            # Generate CSV file name with current date
            current_date = datetime.now().strftime('%Y-%m-%d')
            csv_filename = f'data_UDP_{current_date}.csv'
            
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

if __name__ == '__main__':
    # Start UDP server in a separate thread
    import threading
    udp_thread = threading.Thread(target=handle_udp)
    udp_thread.start()

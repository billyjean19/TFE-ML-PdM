# Implementation based on https://flask.palletsprojects.com/en/3.0.x/

# To run public server in command line: flask --app hello run --host=0.0.0.0 
# @Return: a csv file containing all recordings made during the day

from flask import Flask, request
import csv
from datetime import datetime

app = Flask(__name__)

# HTTP route
@app.route('/receive_data', methods=['POST', 'GET'])
def receive_data():
    if request.method == 'POST':
        data = request.get_data().decode('utf-8')
        
        # Assuming data is comma-separated values
        data_list = data.split(',')
        
        # Generate CSV file name with current date
        current_date = datetime.now().strftime('%Y-%m-%d')
        csv_filename = f'data_HTTP_{current_date}.csv'
        
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

        return 'Data received and written to CSV!'
    else:
        return 'Test OK!'

if __name__ == '__main__':
    app.run(debug=False)


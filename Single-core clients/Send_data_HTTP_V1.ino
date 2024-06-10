#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <HTTPClient.h>

/* uncomment to select the sensor to use (single mode or dual mode) */
#define  MPU5060 1
#define  ADXL345 2

/* uncomment to show performance logs */
//#define  PERFORMANCE 3

sensors_event_t event_mpu, event_adxl;
sensor_t sensor_mpu, sensor_adxl;

#ifdef MPU5060
Adafruit_MPU6050 mpu;
#endif

#ifdef ADXL345
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
#endif

// Wifi parameters
const char* ssid     = "XXXXX"; // Change this to your WiFi SSID
const char* password = "XXXXX"; // Change this to your WiFi password

// to find the url on your computer : flask --app hello run --host=0.0.0.0
const char *serverUrl = "http://XXXXXXXX:5000/receive_data";  

// Create an HTTP client object
HTTPClient http;

// Buffer for holding data to send
#define MAX_DATA_LENGTH 256
char dataBuffer[MAX_DATA_LENGTH];

// Batch variables
#define MAX_BATCH_SIZE 10  // Maximum number of data points to collect before sending batch
char batchData[MAX_BATCH_SIZE][MAX_DATA_LENGTH]; // Array to store batch data
uint8_t batchCount = 0; // Counter for batch size

void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(10);

  #ifdef MPU5060
  if (!mpu.begin()) {
    Serial.println("Failed to connect MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  Serial.println("");
  delay(100);
  #endif

  #ifdef ADXL345
  if(!accel.begin()) {
    Serial.println("Could not find a valid ADXL345 sensor");
    while (1) {
      delay(10);
    }
  }
  accel.setRange(ADXL345_RANGE_4_G);
  //accel.setDataRate(ADXL345_DATARATE_3200_HZ);
  Serial.println("ADXL345 Found!");
  Serial.println("");
  delay(100);
  #endif

  // Connect to WiFi network
  connectToWiFi();
}

void loop() {

  long int t1 = millis();
  
  #ifdef MPU5060
  mpu.getAccelerometerSensor()->getEvent(&event_mpu);
  mpu.getAccelerometerSensor()->getSensor(&sensor_mpu);

  // Read accelerometer data
  float x = event_mpu.acceleration.x;
  float y = event_mpu.acceleration.y;
  float z = event_mpu.acceleration.z;

  // Calculate total acceleration magnitude
  float accelerationMagnitude = sqrt(x * x + y * y + z * z);

  // Format data
  snprintf(dataBuffer, MAX_DATA_LENGTH, "%s,%lu,%.9f,%.9f,%.9f,%.9f", sensor_mpu.name, t1, x, y, z, accelerationMagnitude);

  // Add data to batch
  addDataToBatch(dataBuffer);
  #endif

  #ifdef ADXL345
  accel.getEvent(&event_adxl);
  accel.getSensor(&sensor_adxl);

  // Read accelerometer data
  float x2 = event_adxl.acceleration.x;
  float y2 = event_adxl.acceleration.y;
  float z2 = event_adxl.acceleration.z;

  // Calculate total acceleration magnitude
  float accelerationMagnitude2 = sqrt(x2 * x2 + y2 * y2 + z2 * z2);

  // Format data 
  snprintf(dataBuffer, MAX_DATA_LENGTH, "%s,%lu,%.9f,%.9f,%.9f,%.9f", sensor_adxl.name, t1, x2, y2, z2, accelerationMagnitude2);

  // Add data to batch
  addDataToBatch(dataBuffer);
  #endif

  long int t2 = micros();

  // Check if batch is full and send if needed
  if (batchCount >= MAX_BATCH_SIZE) {
    sendBatchData();
  }

  long int t3 = micros();

  #ifdef PERFORMANCE
  Serial.print("Time taken to read data: "); Serial.print((t2/1000-t1)); Serial.println(" milliseconds");
  Serial.print("Time taken to send data: "); Serial.print((t3-t2)/1000); Serial.println(" milliseconds");
  #endif
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

void addDataToBatch(const char *data) {
  
  if (batchCount < MAX_BATCH_SIZE) {
    // Add data to batch array
    strcpy(batchData[batchCount], data);
    batchCount++;
  }
}

void sendDataToServer(const char* dataToSend) {

  http.begin(serverUrl);
  http.addHeader("Content-Type", "text/plain");
  int httpResponseCode = http.POST(dataToSend);
  http.end();
}

void sendBatchData() {
  
  // Iterate through batch data and send each data point
  for (int i = 0; i < batchCount; i++) {
    sendDataToServer(batchData[i]);
  }
  // Reset batch count
  memset(batchData, 0, sizeof(batchData));
  batchCount = 0;
}


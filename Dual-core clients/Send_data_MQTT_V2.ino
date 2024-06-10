#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <PubSubClient.h>

/* uncomment to select the sensor to use (single mode or dual mode) */
#define  MPU5060 1
#define  ADXL345 2

/* uncomment to show performance logs */
#define  PERFORMANCE 3

sensors_event_t event_mpu, event_adxl;
sensor_t sensor_mpu, sensor_adxl;

#ifdef MPU6050
Adafruit_MPU6050 mpu;
#endif

#ifdef ADXL345
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
#endif

// Wifi parameters
const char* ssid     = "XXXXX"; // Change this to your WiFi SSID
const char* password = "XXXXX"; // Change this to your WiFi password

// IP address of MQTT Broker:
const char * mqttServer = "XXXXX";  // here at localhost
const int    mqttPort = 1883;

// The MQTT library class
WiFiClient espClient;
PubSubClient client(espClient);
char *publishTopic = "sensing_data_topic";

// Handle dual core programming
TaskHandle_t TaskRead;
TaskHandle_t TaskSend;

// Buffer for holding data to send
#define MAX_DATA_LENGTH 256 
char dataBuffer[MAX_DATA_LENGTH];

// Batch variables
#define MAX_BATCH_SIZE 1 //250 // Maximum number of data points to collect before sending batch
char batchData[MAX_BATCH_SIZE][MAX_DATA_LENGTH]; // Array to store batch data
uint8_t batchCount = 0; // Counter for batch size


void setup() {
  
  Serial.begin(115200);
  Serial.println();
  delay(10);

  #ifdef MPU6050   
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
  //accel.setDataRate(ADXL345_DATARATE_1600_HZ);
  Serial.println("ADXL345 Found!");
  Serial.println("");
  delay(100);
  #endif

  // Wifi connection
  connectToWiFi();

  // Initialize MQTT client
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  //create a task that executes the TaskReadSensor() function executed on core 1
  xTaskCreatePinnedToCore(TaskReadSensor, "TaskReadSensor", 10000, NULL, 1, &TaskRead, 1);
  
  //create a task that executes the TaskSendData() function executed on core 0
  xTaskCreatePinnedToCore(TaskSendData, "TaskSendData", 10000, NULL, 1, &TaskSend, 0);
}

void loop() {
  // nothing to do here, everything happens in the TaskReadSensor and TaskSendData functions
  delay(10);
}

void connectToWiFi() {

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    WiFi.mode(WIFI_STA);
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      client.subscribe("sensing_data_topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void addDataToBatch(const char *data) {
  
  if (batchCount < MAX_BATCH_SIZE) {
    // Add data to batch array
    strcpy(batchData[batchCount], data);
    batchCount++;
  }
}

void sendDataToServer(const char* dataToSend) {

  if (!client.connected()) {
    reconnect();
  }

  client.publish(publishTopic, dataToSend);
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

void TaskSendData(void* pvParameters) {

  for (;;) {
    long int t2 = micros();

    // Check if batch is full and send if needed
    if (batchCount >= MAX_BATCH_SIZE) {
      sendBatchData();
    }

    long int t3 = micros();

    #ifdef PERFORMANCE
    Serial.print("[Core ");Serial.print(xPortGetCoreID());Serial.print("] Time taken send data: "); Serial.print((t3-t2)/1000); Serial.println(" milliseconds");
    #endif
  }
}

void TaskReadSensor(void* pvParameters) {

  for (;;) {
    long int t1 = millis();

    #ifdef MPU6050
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
    
    #ifdef PERFORMANCE
    Serial.print("[Core ");Serial.print(xPortGetCoreID());Serial.print("] Time taken to read data: "); Serial.print((t2/1000-t1)); Serial.println(" milliseconds");
    #endif
  }
}





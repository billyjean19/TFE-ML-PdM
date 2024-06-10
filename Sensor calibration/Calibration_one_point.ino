/*

One-point Calibration routine

Sources of inspirations : 
  - https://descargas.cetronic.es/adxl345.pdf
  - https://learn.adafruit.com/calibrating-sensors/single-point-calibration  
  - https://github.com/blaisejarrett/Arduino-Lib.ADXL345/blob/master/examples/Calibrate/Calibrate.ino 

Method: use gravity vectors (+1g and -1g) as reference in order to proceed sensor calibration

Assumption: It assumes that the sensors have been soldered flat to the module and that the module is placed on a flat surface.

*/

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_Sensor.h>

/* uncomment to select the sensor to be calibrated (one at a time) */
//#define  MPU6050 1
#define  ADXL345 2

/* uncomment to select the axis to be calibrated (one at a time) */
#define  CALIBRATE_X 3
//#define  CALIBRATE_Y 4
//#define  CALIBRATE_Z 5

//sensors parameters
sensors_event_t event_mpu, event_adxl;
sensor_t sensor_mpu, sensor_adxl;
Adafruit_MPU6050 mpu;
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Calibration paramaters
float OffMinX = 0, OffMaxX = 0, OffMinY = 0, OffMaxY = 0, OffMinZ = 0, OffMaxZ = 0;
float X_offset = 0, Y_offset = 0, Z_offset = 0;
float numReadings = 200;
float gReference = 9.80665; // reference value for 1g
int calibrated = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(1000);

  Serial.println("");

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

  // Note: You should calibrate upon re-powering the sensor
  // No function implemented in the library that support automatic offset shifting
  calibrateMPU6050();
  #endif

  #ifdef ADXL345 
  if(!accel.begin()) {
    Serial.println("Could not find a valid ADXL345 sensor");
    while (1) {
      delay(10);
    }
  }
  accel.setRange(ADXL345_RANGE_4_G);
  accel.setDataRate(ADXL345_DATARATE_3200_HZ);
  Serial.println("ADXL345 Found!");
  Serial.println("");
  delay(100);

  // Note: You should calibrate upon re-powering the sensor
  // Uncomment if you want to compute calibration offsets
  calibrateADXL345();

  // Update with ADXL345 offset values from 6 positions (+1g and -1g for each axis)
  OffMinX = -0.63; 
  OffMaxX = -0.24;
  OffMinY = 0.0;
  OffMaxY = -0.08;
  OffMinZ = -0.24;
  OffMaxZ = -0.31;

  // Apply the offset in ADXL registers directly 
  setOffsetsADXL345((OffMinX+OffMaxX)/2,(OffMinY+OffMaxY)/2,(OffMinZ+OffMaxZ)/2);
  #endif

  // set to 1 to show calibrated values or 2 to compare with non calibrated values;
  calibrated = 0;

}

void loop() {
  // put your main code here, to run repeatedly:

  #ifdef ADXL345 
  accel.getEvent(&event_adxl);

  float X_out = event_adxl.acceleration.x;
  float Y_out = event_adxl.acceleration.y;
  float Z_out = event_adxl.acceleration.z;

  if(calibrated == 1){
    Serial.println("Calibrated ADXL345 values:");
    Serial.print("X: ");
    Serial.print(X_out);
    Serial.print(" Y: ");
    Serial.print(Y_out);
    Serial.print(" Z: ");
    Serial.println(Z_out);
  }
  else if (calibrated == 2){
    setOffsetsADXL345(0,0,0);
    Serial.println("Uncalibrated ADXL345 values:");
    Serial.print("X: ");
    Serial.print(X_out);
    Serial.print(" Y: ");
    Serial.print(Y_out);
    Serial.print(" Z: ");
    Serial.println(Z_out);
  }
  #endif

  #ifdef MPU6050 
  accel.getEvent(&event_mpu);

  // Update with offset values from 6 positions (+1g and -1g for each axis)
  OffMinX2 = -0.19; 
  OffMaxX2 = -0.32;
  OffMinY2 = -2.92;
  OffMaxY2 = -2.98;
  OffMinZ2 = -0.53;
  OffMaxZ2 = -0.82;

  float X_out2 = setOffsetsMPU6050(event_mpu.acceleration.x,(OffMinX2+OffMaxX2)/2);
  float Y_out2 = setOffsetsMPU6050(event_mpu.acceleration.y,(OffMinY2+OffMaxY2)/2);
  float Z_out2 = setOffsetsMPU6050(event_mpu.acceleration.z,(OffMinZ2+OffMaxZ2)/2);

  if(calibrated == 1){
    Serial.println("Calibrated MPU6050 values:");
    Serial.print("X: ");
    Serial.print(X_out2);
    Serial.print(" Y: ");
    Serial.print(Y_out2);
    Serial.print(" Z: ");
    Serial.println(Z_out2);
  }
  else if (calibrated == 2){
    Serial.println("Uncalibrated MPU6050 values:");
    Serial.print("X: ");
    Serial.print(X_out2);
    Serial.print(" Y: ");
    Serial.print(Y_out2);
    Serial.print(" Z: ");
    Serial.println(Z_out2);
  }
  #endif

  delay(3000);
}

int gatherAverage(sensors_event_t * sensor, float * xAv, float * yAv, float * zAv) {

    float xSum = 0, ySum = 0, zSum = 0;  // to gather average values

    for (int i = 0; i < numReadings; i++) {

      // Read accelerometer data
      float X_out = sensor->acceleration.x;
      float Y_out = sensor->acceleration.y;
      float Z_out = sensor->acceleration.z;
      
      xSum += X_out;
      ySum += Y_out;
      zSum += Z_out;

      Serial.print(".");

      delay(50);
    }

    Serial.println("");

    //average values
    *xAv = (xSum / numReadings);
    *yAv = (ySum / numReadings);
    *zAv = (zSum / numReadings);

    Serial.println("Average values : ");
    Serial.print("X average: ");
    Serial.print(*xAv);
    Serial.print(", Y average: ");
    Serial.print(*yAv);
    Serial.print(", Z average: ");
    Serial.println(*zAv);

    return 0;
}

void computeOffsets(float * xAv, float * yAv, float * zAv){
  
  #ifdef CALIBRATE_X
  if(*xAv>0){ 
    X_offset = gReference - *xAv;         //1g oriented
  } else { 
    X_offset = - gReference - *xAv;       //-1g oriented
  }
  Serial.print("X_offset= " );Serial.println(X_offset);
  #endif

  #ifdef CALIBRATE_Y
  if(*yAv>0){
    Y_offset = gReference - *yAv;         //1g oriented
  } else {
    Y_offset = - gReference - *yAv;       //-1g oriented
  }
  Serial.print("Y_offset= " );Serial.println(Y_offset);
  #endif

  #ifdef CALIBRATE_Z
  if(*zAv>0){ 
    Z_offset = gReference - *zAv;         //1g oriented
  } else {
    Z_offset = - gReference - *zAv;       //-1g oriented
  }
  Serial.print("Z_offset= " );Serial.println(Z_offset);
  #endif
}

void calibrateADXL345() {

  accel.getEvent(&event_adxl);

  // gather average values
  Serial.println("Gathering Average ADXL345...");
  float xAv, yAv, zAv;
  if (gatherAverage(&event_adxl,&xAv, &yAv, &zAv) != 0){
    return;
  }
 
  //use them to evaluate the calibration offsets
  computeOffsets(&xAv, &yAv, &zAv);

  delay(50);

}

void calibrateMPU6050() {

  mpu.getAccelerometerSensor()->getEvent(&event_mpu);

  // gather average values
  Serial.println("Gathering Average MPU6050...");
  float xAv, yAv, zAv;
  if (gatherAverage(&event_mpu, &xAv, &yAv, &zAv) != 0){
    return;
  }
 
  //use them to evaluate the calibration offsets
  computeOffsets(&xAv, &yAv, &zAv);

  delay(50);

}

void setOffsetsADXL345(float X_offset, float Y_offset, float Z_offset) {
  // Because the value placed in an offset register is additive, a negative value is placed into the register to eliminate 
  // a positive offset and vice versa for a negative offset
  accel.writeRegister(ADXL345_REG_OFSX,X_offset);
  accel.writeRegister(ADXL345_REG_OFSY,Y_offset);
  accel.writeRegister(ADXL345_REG_OFSZ,Z_offset);
}

float setOffsetsMPU6050(float rawValue, float offset) {
  if((offset>0 && rawValue>0) || (offset<0 && rawValue<0)) return rawValue - offset;
  else return rawValue + offset;
}



#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <esp_now.h>
#include <ahmedtos-project-1_inferencing.h>

// Constants
#define MAX_ACCEPTED_RANGE  2.0f        
#define SCALE_FACTOR 0.08


Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Error initializing MPU6050!");
    return;
  } else {
    Serial.println("MPU6050 initialized");
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);
}

void loop() {
  // Prepare buffer for sensor data
  float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };

  for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3) {
    uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    buffer[ix] = a.acceleration.x * SCALE_FACTOR;
    buffer[ix + 1] = a.acceleration.y * SCALE_FACTOR;
    buffer[ix + 2] = a.acceleration.z * SCALE_FACTOR;

    delayMicroseconds(next_tick - micros());
  }

  // Convert buffer to signal for classification
  signal_t signal;
  int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
  if (err != 0) {
    Serial.println("Failed to create signal from buffer");
    return;
  }

  // Run the classifier
  ei_impulse_result_t result = { 0 };
  err = run_classifier(&signal, &result, false);
  if (err != EI_IMPULSE_OK) {
    Serial.print("Failed to run classifier: ");
    Serial.println(err);
    return;
  }

//0 yaw
//1 idle
// 2 pitch

  // Serial.println("lable");
  
  if (result.classification[0].value > 0.7) {
        // Serial.println(result.classification[1].label); //vertical
        Serial.println("Yaw Movement Detected");
    }
   else if(result.classification[1].value > 0.7) {
        //Serial.println(result.classification[2].label);
        Serial.println("Sensor is Idle");
  }
  else if(result.classification[2].value > 0.7) {
        //Serial.println(result.classification[2].label);
      Serial.println("Pitch movement detected");
  }

  // Small delay to stabilize loop
  delay(100);
}

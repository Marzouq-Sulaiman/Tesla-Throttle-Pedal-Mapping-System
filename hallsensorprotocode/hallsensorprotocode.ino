#include <Wire.h>        
#include <math.h>
#include <cstdint>    
#include "SparkFun_TMAG5273_Arduino_Library.h" 


TMAG5273 sensor; 
// MACROS
#define cycles_per_avg_reading 20
#define SPEED_GRANULARITY 0.5
#define TORQUE_GRANULARITY 0.01
#define maxField 7.0; // mT value when pedal is fully pressed 
#define minField 1.0; // mT value when pedal is released
// Globals for torque 
int MODE = 0;  // 0 = Standard, 1 = Sport, 2 = Eco
int TORQUE_COMMAND;

// Note that values are upshifted by 150NM, or 15000 units (150 * Torque granularity)
// This is done so that a value of 0 represents our lowest possible value of torque (no negatives)
uint16_t upper_map[256];
uint16_t lower_map[256];
double sport_map[100];
double eco_map[100];

// Helper functions:
float pedal_percent_averaged();
void print_uint16_array(const char* name, const uint16_t* arr, size_t len);
void print_double_array(const char* name, const double* arr, size_t len);
void init_upper(void);
void init_lower(void);
void init_sport(void);
void init_eco(void);
int index_from_speed(int speed);
int torque_to_command(int torque);
uint16_t TorqueMap(double pedal_percentage, int speed);

// I2C default address
uint8_t i2cAddress = TMAG5273_I2C_ADDRESS_INITIAL;

//** CALIBRATION **//
bool pedalPressed = false;


void setup() 
{
  init_upper();
  init_lower();
  init_sport();
  init_eco();

  print_double_array("Sport Map", sport_map, 100);
  print_double_array("Eco Map", eco_map, 100);
  print_uint16_array("Upper Map", upper_map, 256);
  print_uint16_array("Lower Map", lower_map, 256);

  delay(60000);   // 60 seconds
  // Wire.begin();
   delay(2000);
  Serial.begin(115200);  
  Wire.begin(21, 22); // SDA & SCL pins respectively on ESP

  if (sensor.begin(i2cAddress, Wire) == 1) {
    Serial.println("SENSOR READY.");
  } else {
    Serial.println("SENSOR ERROR");
    while (1);
  }
  sensor.setTemperatureEn(false); // temperature not needed
}

void loop() 
{
  
  if(sensor.getMagneticChannel() != 0) {

  // Compute pedal percentage
  float pedal_percentage = pedal_percent_averaged();
  


  uint_16 Torque_CMD = TorqueMap(pedal_percentage)

  // Send Torque Command to Simulation
  // ****************** JACK + MARZOUK FILL IN HERE ******************

  // Delay
  delay(20);

  // Poll SIM for speed
  // ****************** JACK + MARZOUK FILL IN HERE ******************

   // currently sampling at 50 times/sec but update this if it hogs the MCU too much or results in unacceptably delayed pedal sensing  
  } else
   {
     //error with the magnetic channels
     Serial.println("CHANNEL ISSUE");
    //  while(1);
   }

}

float pedal_percent_averaged(){
  float average_mag_z;
  for (int i = 0; i < cycles_per_avg_reading; i++){
    average_mag_z += sensor.getZData(); // CHANGE AS NEEDED; assuming Z axis faces magnet
    delay(20);
  }
  average_mag_z /= cycles_per_avg_reading;
   // map to pedal position
  float average_percent = (average_mag_z - minField) / (maxField - minField); // Pedal position percent in decimal
  if (average_percent > 1.0) average_percent = 1.0; // Truncate to maximum of 100%
  if (average_percent < 0.0) average_percent = 0.0; // Truncate to minimum of 0%
  //**for debugging **//
  Serial.print("\nB field: ");
  Serial.print(average_mag_z, 3);
  Serial.print(" Position: ");
  Serial.print(average_percent, 4);
  return;
}

void print_uint16_array(const char* name, const uint16_t* arr, size_t len) {
  Serial.print(name);
  Serial.print(" = [");
  for (size_t i = 0; i < len; i++) {
    Serial.print(arr[i]);
    if (i < len - 1) Serial.print(", ");
  }
  Serial.println("]");
}

void print_double_array(const char* name, const double* arr, size_t len) {
  Serial.print(name);
  Serial.print(" = [");
  for (size_t i = 0; i < len; i++) {
    Serial.print(arr[i], 3);
    if (i < len - 1) Serial.print(", ");
  }
  Serial.println("]");
}

void init_sport() {
  double start_val_1 = 0;
  double end_val_1 = 0.35;
  int start_1 = 0;
  int end_1 = round(100 * 0.2);
  int steps_1 = end_1 - start_1;
  double step_size_1 = (start_val_1 - end_val_1) / steps_1;

  for (int i = start_1; i <= end_1; i++) {
    sport_map[i] = start_val_1 - (i - start_1) * step_size_1;
  }

  double start_val_2 = 0.36;
  double end_val_2 = 0.65;
  int start_2 = end_1 + 1;
  int end_2 = round(100 * 0.4);
  int steps_2 = end_2 - start_2;
  double step_size_2 = (start_val_2 - end_val_2) / steps_2;

  for (int i = start_2; i <= end_2; i++) {
    sport_map[i] = start_val_2 - (i - start_2) * step_size_2;
  }

  double start_val_3 = 0.66;
  double end_val_3 = 0.80;
  int start_3 = end_2 + 1;
  int end_3 = round(100 * 0.6);
  int steps_3 = end_3 - start_3;
  double step_size_3 = (start_val_3 - end_val_3) / steps_3;

  for (int i = start_3; i <= end_3; i++) {
    sport_map[i] = start_val_3 - (i - start_3) * step_size_3;
  }

  double start_val_4 = 0.81;
  double end_val_4 = 1;
  int start_4 = end_3 + 1;
  int end_4 = 99;
  int steps_4 = end_4 - start_4;
  double step_size_4 = (start_val_4 - end_val_4) / steps_4;

  for (int i = start_4; i <= end_4; i++) {
    sport_map[i] = start_val_4 - (i - start_4) * step_size_4;
  }
}

void init_eco() {
  double start_val_1 = 0;
  double end_val_1 = 0.2;
  int start_1 = 0;
  int end_1 = round(100 * 0.4);
  int steps_1 = end_1 - start_1;
  double step_size_1 = (start_val_1 - end_val_1) / steps_1;

  for (int i = start_1; i <= end_1; i++) {
    eco_map[i] = start_val_1 - (i - start_1) * step_size_1;
  }

  double start_val_2 = 0.21;
  double end_val_2 = 0.35;
  int start_2 = end_1 + 1;
  int end_2 = round(100 * 0.6);
  int steps_2 = end_2 - start_2;
  double step_size_2 = (start_val_2 - end_val_2) / steps_2;

  for (int i = start_2; i <= end_2; i++) {
    eco_map[i] = start_val_2 - (i - start_2) * step_size_2;
  }

  double start_val_3 = 0.36;
  double end_val_3 = 0.75;
  int start_3 = end_2 + 1;
  int end_3 = round(100 * 0.90);
  int steps_3 = end_3 - start_3;
  double step_size_3 = (start_val_3 - end_val_3) / steps_3;

  for (int i = start_3; i <= end_3; i++) {
    eco_map[i] = start_val_3 - (i - start_3) * step_size_3;
  }

  double start_val_4 = 0.76;
  double end_val_4 = 1;
  int start_4 = end_3 + 1;
  int end_4 = 99;
  int steps_4 = end_4 - start_4;
  double step_size_4 = (start_val_4 - end_val_4) / steps_4;

  for (int i = start_4; i <= end_4; i++) {
    eco_map[i] = start_val_4 - (i - start_4) * step_size_4;
  }
}

void init_upper() {
  for (int i = 0; i < 144; i++) {
    upper_map[i] = 0xFFFF;  // 65535 (Maximum Torque (505 NM))
  }

  int start = 144;
  int end   = 255;

  double start_val = 65535.0;  // Maximum Torque (505 NM)
  double end_val   = 46000.0;  // Torque at 125 km/h (310 NM)

  int steps = end - start;
  double step_size = (start_val - end_val) / steps;

  for (int i = start; i <= end; i++) {
    upper_map[i] = (uint16_t)(start_val - (i - start) * step_size);
  }
}

void init_lower() {
  // 0–15: linearly decrease from 15000 → 0
  double start_val_1 = 15000.0;  // 0 NM
  double end_val_1   = 0.0;      // -150 NM
  int start_1 = 0;
  int end_1   = 15;
  int steps_1 = end_1 - start_1;
  double step_1 = (start_val_1 - end_val_1) / steps_1;

  for (int i = start_1; i <= end_1; i++) {
    lower_map[i] = (uint16_t)(start_val_1 - (i - start_1) * step_1);
  }

  // 16–110: constant 0
  for (int i = 16; i <= 110; i++) {
    lower_map[i] = 0;
  }

  // 111–255: linearly increase from 0 → 7000
  double start_val_2 = 0.0;
  double end_val_2   = 7000.0;
  int start_2 = 111;
  int end_2   = 255;
  int steps_2 = end_2 - start_2;
  double step_2 = (end_val_2 - start_val_2) / steps_2;

  for (int i = start_2; i <= end_2; i++) {
    lower_map[i] = (uint16_t)(start_val_2 + (i - start_2) * step_2);
  }
}

int index_from_speed(int speed) {
  // Find x axis index by multiplying the speed number by the granularity
  // e.g. speed number = 4 -> corresponds to 2km/h if granularity is 0.5
  return speed * SPEED_GRANULARITY;
}

int torque_to_command(int torque) {
  // Fill when we find out what the dyno uses to represent torque
  return 1;
}

uint16_t TorqueMap(double pedal_percentage, int speed) {
  int speed_index = index_from_speed(speed);

  uint16_t upper = upper_map[speed_index];
  uint16_t lower = lower_map[speed_index];
  uint16_t difference = upper - lower;

  int mapped_torque;

  if (MODE == 0) {
    // Standard mode
    mapped_torque = (difference * pedal_percentage) + lower;

  } else if (MODE == 1) {
    // Sport Mode
    mapped_torque =
        (difference *
         sport_map[(int)(pedal_percentage * 100 - 1)]) + lower;

  } else {
    // Eco Mode
    mapped_torque =
        (difference *
         eco_map[(int)(pedal_percentage * 100 - 1)]) + lower;
  }

  return mapped_torque;
}

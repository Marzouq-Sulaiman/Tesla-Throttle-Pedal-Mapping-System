#include <math.h>
#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "log";

#define SPEED_GRANULARITY 0.5
#define TORQUE_GRANULARITY 0.01

int MODE = 0;  // 0 = Standard, 1 = Sport, 2 = Eco
int TORQUE_COMMAND;

uint16_t upper_map[256];
uint16_t lower_map[256];
double sport_map[100];
double eco_map[100];

void print_uint16_array(const char* name, const uint16_t* arr, size_t len) {
  printf("%s = [", name);
  for (size_t i = 0; i < len; i++) {
    printf("%u", arr[i]);
    if (i < len - 1) printf(", ");
  }
  printf("]\n");
}

void print_double_array(const char* name, const double* arr, size_t len) {
  printf("%s = [", name);
  for (size_t i = 0; i < len; i++) {
    printf("%.3f", arr[i]);
    if (i < len - 1) printf(", ");
  }
  printf("]\n");
}

void init_sport(void) {
  double start_val_1 = 0;
  double end_val_1 = 0.35;
  int start_1 = 0;
  int end_1 = (int)round(100 * 0.2);
  int steps_1 = end_1 - start_1;
  double step_size_1 = (start_val_1 - end_val_1) / steps_1;

  for (int i = start_1; i <= end_1; i++) {
    sport_map[i] = (start_val_1 - (i - start_1) * step_size_1);
  }

  double start_val_2 = 0.36;
  double end_val_2 = 0.65;
  int start_2 = end_1 + 1;
  int end_2 = (int)round(100 * 0.4);
  int steps_2 = end_2 - start_2;
  double step_size_2 = (start_val_2 - end_val_2) / steps_2;

  for (int i = start_2; i <= end_2; i++) {
    sport_map[i] = (start_val_2 - (i - start_2) * step_size_2);
  }

  double start_val_3 = 0.66;
  double end_val_3 = 0.80;
  int start_3 = end_2 + 1;
  int end_3 = (int)round(100 * 0.6);
  int steps_3 = end_3 - start_3;
  double step_size_3 = (start_val_3 - end_val_3) / steps_3;

  for (int i = start_3; i <= end_3; i++) {
    sport_map[i] = (start_val_3 - (i - start_3) * step_size_3);
  }

  double start_val_4 = 0.81;
  double end_val_4 = 1;
  int start_4 = end_3 + 1;
  int end_4 = 99;
  int steps_4 = end_4 - start_4;
  double step_size_4 = (start_val_4 - end_val_4) / steps_4;

  for (int i = start_4; i <= end_4; i++) {
    sport_map[i] = (start_val_4 - (i - start_4) * step_size_4);
  }
}

void init_eco(void) {
  double start_val_1 = 0;
  double end_val_1 = 0.2;
  int start_1 = 0;
  int end_1 = (int)round(100 * 0.4);
  int steps_1 = end_1 - start_1;
  double step_size_1 = (start_val_1 - end_val_1) / steps_1;

  for (int i = start_1; i <= end_1; i++) {
    eco_map[i] = (start_val_1 - (i - start_1) * step_size_1);
  }

  double start_val_2 = 0.21;
  double end_val_2 = 0.35;
  int start_2 = end_1 + 1;
  int end_2 = (int)round(100 * 0.6);
  int steps_2 = end_2 - start_2;
  double step_size_2 = (start_val_2 - end_val_2) / steps_2;

  for (int i = start_2; i <= end_2; i++) {
    eco_map[i] = (start_val_2 - (i - start_2) * step_size_2);
  }

  double start_val_3 = 0.36;
  double end_val_3 = 0.75;
  int start_3 = end_2 + 1;
  int end_3 = (int)round(100 * 0.90);
  int steps_3 = end_3 - start_3;
  double step_size_3 = (start_val_3 - end_val_3) / steps_3;

  for (int i = start_3; i <= end_3; i++) {
    eco_map[i] = (start_val_3 - (i - start_3) * step_size_3);
  }

  double start_val_4 = 0.76;
  double end_val_4 = 1;
  int start_4 = end_3 + 1;
  int end_4 = 99;
  int steps_4 = end_4 - start_4;
  double step_size_4 = (start_val_4 - end_val_4) / steps_4;

  for (int i = start_4; i <= end_4; i++) {
    eco_map[i] = (start_val_4 - (i - start_4) * step_size_4);
  }
}

void init_upper(void) {
  for (int i = 0; i < 144; i++) {
    upper_map[i] = 0xFFFF;  // 65535 (Maximum Torque (505 NM))
  }

  // Linearly decrease from 65535 to 46000 over indices 144 to 255 (inclusive)
  int start = 144;
  int end = 255;
  double start_val = 65535.0;  // Maximum Torque (505 NM)
  double end_val = 46000.0;    // Torque at 125km/h (310 NM)
  int steps = end - start;
  double step_size = (start_val - end_val) / steps;

  for (int i = start; i <= end; i++) {
    upper_map[i] = (uint16_t)(start_val - (i - start) * step_size);
  }
}

void init_lower(void) {
  // 0–15: linearly decrease from 15000 → 0
  double start_val_1 = 15000.0;  // 0 NM
  double end_val_1 = 0.0;        // -150 NM
  int start_1 = 0;
  int end_1 = 15;
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
  double end_val_2 = 7000.0;
  int start_2 = 111;
  int end_2 = 255;
  int steps_2 = end_2 - start_2;
  double step_2 = (end_val_2 - start_val_2) / steps_2;

  for (int i = start_2; i <= end_2; i++) {
    lower_map[i] = (uint16_t)(start_val_2 + (i - start_2) * step_2);
  }
}

// void print_uint16_array(const uint16_t* arr, size_t len, const char* label) {
//   ESP_LOGI(TAG, "---- %s ----", label ? label : "Array Contents");
//   ESP_LOGI(TAG, "Index |  Decimal  |   Hex");
//   ESP_LOGI(TAG, "---------------------------");

//   for (size_t i = 0; i < len; i++) {
//     ESP_LOGI(TAG, "%5u | %8u | 0x%04X", (unsigned)i, arr[i], arr[i]);
//   }

//   ESP_LOGI(TAG, "---------------------------");
// }

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
  // Pedal percentage should be a decimal number (i.e. 1 = 100%, 0.99 = 99%,
  // etc.)
  int speed_index = index_from_speed(speed);
  uint16_t upper = upper_map[speed_index];
  uint16_t lower = lower_map[speed_index];
  uint16_t difference = upper - lower;
  int mapped_torque;
  if (MODE == 0) {
    // Standard mode
    mapped_torque = ((double)difference * pedal_percentage) + lower;
  } else if (MODE == 1) {
    // Sport Mode
    mapped_torque = ((double)difference * pedal_percentage *
                     sport_map[(int)(pedal_percentage * 100 - 1)]);
  } else {
    // Eco mode
    mapped_torque = ((double)difference * pedal_percentage *
                     eco_map[(int)(pedal_percentage * 100 - 1)]);
  }
  return mapped_torque;
}

void app_main(void) {
  init_upper();
  init_lower();
  init_sport();
  init_eco();

  while (1) {
    print_double_array("Sport Map", sport_map, 100);
    print_double_array("Eco Map", eco_map, 100);
    print_uint16_array("Upper Map", upper_map, 256);
    print_uint16_array("Lower Map", lower_map, 256);

    vTaskDelay(60000 / portTICK_PERIOD_MS);
    // print_uint16_array(upper_map, 256, "Label");
    // vTaskDelay(5000 / portTICK_PERIOD_MS);
    // print_uint16_array(lower_map, 256, "Label");
  }
}
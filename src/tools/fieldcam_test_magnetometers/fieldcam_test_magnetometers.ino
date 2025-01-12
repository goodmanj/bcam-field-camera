#include <Arduino.h>
#include <Wire.h>

#include "TMAG5273.h"

#define N_I2C_INTS 2
#define TMAG1_SDA 14
#define TMAG1_SCL 15
#define TMAG0_SDA 8
#define TMAG0_SCL 9
TMAG5273 tmag5273_i2c0(&Wire);
TMAG5273 tmag5273_i2c1(&Wire1);

enum model_id {TMAG_A = 0x35, TMAG_B = 0x22, TMAG_C = 0x78, TMAG_D = 0x44};
//#define N_MODELS 3
//model_id model_id_list[N_MODELS] = {TMAG_A,TMAG_B,TMAG_C};
#define N_MODELS 3
model_id model_id_list[N_MODELS] = {TMAG_A, TMAG_B, TMAG_C};

#define N_POWER_PINS 4
int row_power_pin[N_POWER_PINS] = {13,10,26,16};
//int row_power_pin[N_POWER_PINS] = {10};
//int row_power_pin[N_POWER_PINS] = {13,26,16};

#define N_Y N_POWER_PINS
#define N_X N_I2C_INTS * N_MODELS

bool available[N_X][N_Y];  // True if sensor is available

// Decode x,y location of sensor into an I2C interface, default address, and new address.
void which_sensor(int x, int y, int *i2c_int, int *default_i2c, int *new_i2c) {
  if (x < N_MODELS) {
    *i2c_int = 0;
  } else {
    *i2c_int = 1;
    x = x - N_MODELS;
  }
  *default_i2c = model_id_list[x];
  *new_i2c = *default_i2c + y+1;
}

void setup_tmag(TMAG5273 *device) {
  device->configOperatingMode(TMAG5273_OPERATING_MODE_STANDBY);
  device->configReadMode(TMAG5273_READ_MODE_STANDARD);
  device->configMagRange(TMAG5273_MAG_RANGE_40MT);
  device->configLplnMode(TMAG5273_LOW_NOISE);
  device->configMagTempcoMode(TMAG5273_MAG_TEMPCO_NdBFe);
  device->configConvAvgMode(TMAG5273_CONV_AVG_32X);
  device->configTempChEnabled(true);
  device->initAll();
}

void setup() 
{
  int x,y;
  model_id model;
  int i2c_dev, default_i2c_addr, new_i2c_addr;
  TMAG5273 *tmag_object;
  byte error;

  Serial.begin(115200);
  while (!Serial); 
  Serial.println("Starting I2C0");
  Wire.setSDA(TMAG0_SDA);
  Wire.setSCL(TMAG0_SCL);
  Wire.begin();
  Wire.setClock(1000000);

  Serial.println("Starting I2C1");
  Wire1.setSDA(TMAG1_SDA);
  Wire1.setSCL(TMAG1_SCL);
  Wire1.begin();
  Wire1.setClock(1000000);
  
  for (y=0; y < N_Y; y++) {
    Serial.print("Activating power bus ");
    Serial.println(row_power_pin[y]);
    pinMode(row_power_pin[y],OUTPUT);
    digitalWrite(row_power_pin[y],HIGH);
    delay(1);
    Serial.println("Scanning I2C0");
    scanner(&Wire);
    Serial.println("Scanning I2C1");
    scanner(&Wire1);
    delay(1);
    for (x=0; x<N_X; x++) {
      which_sensor(x, y, &i2c_dev, &default_i2c_addr, &new_i2c_addr);

      Serial.print("Configuring sensor x=");
      Serial.print(x);
      Serial.print(", y=");
      Serial.print(y);
      Serial.print(" from I2C");
      Serial.print(i2c_dev);
      Serial.print(" 0x");
      Serial.print(default_i2c_addr,HEX);
      Serial.print(" to  0x");
      Serial.println(new_i2c_addr,HEX);
      if (i2c_dev == 0) {
        tmag_object = &tmag5273_i2c0;
      } else {
        tmag_object = &tmag5273_i2c1;
      }
      tmag_object->switchSensor(default_i2c_addr);
      tmag_object->modifyI2CAddress(new_i2c_addr); // change I2C address
      delay(1);
      if (i2c_dev == 0) {
        Wire.beginTransmission(new_i2c_addr);
        error = Wire.endTransmission();
      } else {
        Wire1.beginTransmission(new_i2c_addr);
        error = Wire1.endTransmission();
      }
      available[x][y] = (error == 0);
    }
    Serial.println("Scanning I2C0");
    scanner(&Wire);
    Serial.println("Scanning I2C1");
    scanner(&Wire1);
  }
  Serial.println("Sensor availability:");
  for (y=0; y < N_Y; y++) {
      for (x=0; x<N_X; x++) {
        Serial.print(available[x][y]?"+":"-");
      }
    Serial.println("");
  }
  setup_tmag(&tmag5273_i2c0);
  setup_tmag(&tmag5273_i2c1);
}

void scanner(TwoWire *wire) {
  byte error, address;
  int nDevices;

  Serial.print("I2C addresses: ");

  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    wire->beginTransmission(address);
    error = wire->endTransmission();

    if (error == 0)
    {
      Serial.print("0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.print(" ");

      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknown error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("\n");

}

void loop() 
{
  int x,y;
  model_id model;
  int i2c_dev, power_bus, default_i2c_addr, i2c_addr;
  TMAG5273 *tmag_object;

  float Bx, By, Bz, T;
  uint8_t res;

  int tick=millis();

  Serial.println("Z-component of field:");
  for (y = 0; y< N_Y; y++) {
    for (x = 0; x < N_X; x++) {
      if (available[x][y]) {
        which_sensor(x, y, &i2c_dev, &default_i2c_addr, &i2c_addr);
        if (i2c_dev == 0) {
          tmag_object = &tmag5273_i2c0;
        } else {
          tmag_object = &tmag5273_i2c1;
        }
        tmag_object->switchSensor(i2c_addr); 
        res = tmag_object->readMagneticField(&Bx, &By, &Bz, &T);
        Serial.print(Bz);
        Serial.print(" ");
      } else {
        Serial.print("---- ");
      }
    }
    Serial.println();
  }
  Serial.print("Time = ");
  Serial.print(millis()-tick);
  Serial.println("ms");
  Serial.println();
  delay(1000);
}

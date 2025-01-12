// Calibrate Field Camera: Calibrate magnetometers for field_camera.ino

// Magnetometers are laid out in a 2-d array, as follows:
// 0A1 0B1 0C1 1A1 1B1 1C1
// 0A2 0B2 0C2 1A2 1B2 1C2
// 0A3 0B3 0C3 1A3 1B3 1C3
// 0A4 0B4 0C4 1A4 1B4 1C4
//
// where the code indicates:
//    [I2C interface] [Magnetometer model] [Power bus]

#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include "TMAG5273.h" 
#include "fieldcam_graphics.h"
#include "LittleFS.h"

// ------------------------------------------------------
// ===== 1. Magnetic field sensor configuration
// ------------------------------------------------------

// Tmag5273 library assumes we're using the TMAG5273A1 with +/- 40 or 80 mT range.  Set this to true
// if you're using the TMAG52783A2 with +/- 133 or 266 mT range.
#define TMAG5273A2_RESCALE false

#define N_I2C_INTS 2 // # of I2C interfaces
#define TMAG1_SDA 14
#define TMAG1_SCL 15
#define TMAG0_SDA 8
#define TMAG0_SCL 9
TMAG5273 tmag5273_i2c0(&Wire);   // Declare one TMAG5273 object for each interface (not each device)
TMAG5273 tmag5273_i2c1(&Wire1);

// Default I2C addresses for TMAG5273 magnetometers, see 
// datasheet https://www.ti.com/lit/ds/symlink/tmag5273.pdf?ts=1686087260323
enum model_id {TMAG_A = 0x35, TMAG_B = 0x22, TMAG_C = 0x78, TMAG_D = 0x44};
#define N_MODELS 3
model_id model_id_list[N_MODELS] = {TMAG_A, TMAG_B, TMAG_C}; // Which models are in our array?

// ------------------------------------------------------
// ===== 2. TFT LCD configuration
// ------------------------------------------------------

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH,SCREEN_HEIGHT);  
TMAG5273 tmag5273(&Wire);          

// ------------------------------------------------------
// ===== 3. Magnetometer power configuration
// ------------------------------------------------------

#define N_POWER_PINS 4
int row_power_pin[N_POWER_PINS] = {13,10,26,16};
//int row_power_pin[N_POWER_PINS] = {10};
//int row_power_pin[N_POWER_PINS] = {13,26,16};

// ------------------------------------------------------
// ===== 3. Sensor / Screen Interaction configuration
// ------------------------------------------------------

// Total screen width: 73.44 mm, 480 px
// Total screen height: 49 mm, 320 px
// Sensor X positions relative to screen edge (mm): 4.53 16.43 28.43 40.43 52.53 64.18
// Sensor Y positions relative to screen edge (mm): 5.85 17.98 30.99 42.98
// 
#define N_SENSOR_ROWS N_POWER_PINS
#define N_SENSOR_COLUMNS N_I2C_INTS * N_MODELS
const int sensor_x[N_SENSOR_COLUMNS] = {30, 107, 186, 264, 343, 419};
const int sensor_y[N_SENSOR_ROWS] = {38,118,203,281};

// ------------------------------------------------------
// ===== 4. Graphics helper functions and classes
// ------------------------------------------------------

// Defined in fieldcam_graphics.cpp

// ------------------------------------------------------
// ===== 5. UI Settings and Button Handler
// ------------------------------------------------------

// Button info
const int button_width = 90;
const int button_height = 30;
const int button_top = SCREEN_HEIGHT*2/3;
const int button_center_x = SCREEN_WIDTH/2;

Button start_button = Button(&tft, "Start", button_center_x-button_width, button_top, 90, 30, TFT_BLACK, TFT_WHITE);

// ------------------------------------------------------
// ===== 7. Sensor array support functions
// ------------------------------------------------------

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

// Scan I2C addresses for debugging purposes
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

// Print calibration data to a file or serial port.  c_format=true gives output that can be pasted
// into C code.

void print_cal_data(Stream *s, float field[N_SENSOR_COLUMNS][N_SENSOR_ROWS]) {
  for (int x=0; x<N_SENSOR_COLUMNS; x++) {// Print out C++ array definition format
    for (int y=0; y < N_SENSOR_ROWS; y++) {
      s->print(field[x][y],4);
      if (y==N_SENSOR_ROWS-1) {
        s->println("");
      } else {
        s->print(", ");                  
      }
    }
  }
 }

// ------------------------------------------------------
// ===== 9. Setup
// ------------------------------------------------------

void setup() {
  int x,y;
  model_id model;
  int i2c_dev, default_i2c_addr, new_i2c_addr;
  TMAG5273 *tmag_object;

  // --- Magnetometer array setup --- 

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

  for (y=0; y < N_SENSOR_ROWS; y++) {
    Serial.print("Activating power bus ");
    Serial.println(row_power_pin[y]);
    pinMode(row_power_pin[y],OUTPUT);
    digitalWrite(row_power_pin[y],HIGH);
    delay(1);
    for (x=0; x<N_SENSOR_COLUMNS; x++) {
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
    }
    delay(1);
    Serial.println("Scanning I2C0");
    scanner(&Wire);
    Serial.println("Scanning I2C1");
    scanner(&Wire1);
  }
  setup_tmag(&tmag5273_i2c0);
  setup_tmag(&tmag5273_i2c1);


  // --- TFT setup --- 

  // Set up TFT, black screen medium font.
  tft.init(); // Not sure if both init() and begin() are needed or what the difference is
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0);
  tft.setTextFont(2);
  // set up touch calibration
  uint16_t calData[5] = { 196, 3683, 88, 3707, 1 };
  tft.setTouch(calData);

  // LittleFS setup
  LittleFS.begin();
}

// ------------------------------------------------------
// ===== 10. Loop
// ------------------------------------------------------

void loop() {
  float Bx_avg[N_SENSOR_COLUMNS][N_SENSOR_ROWS], By_avg[N_SENSOR_COLUMNS][N_SENSOR_ROWS], Bz_avg[N_SENSOR_COLUMNS][N_SENSOR_ROWS];
  int n_avgs = 0;
  bool integrating = false;
  int tick;
  int cal_phase = 0; // Which rotation step are we on?
  int next_cal_phase;
  int cal_phase_time = 5000; // How long to run each calibration phase, in milliseconds.

  int x,y;
  model_id model;
  int i2c_dev, power_bus, default_i2c_addr, i2c_addr;
  TMAG5273 *tmag_object;

  float Bx, By, Bz, T;
  uint8_t res;

  uint16_t touch_x, touch_y;

  // Zero averages
  for (y=0; y < N_SENSOR_ROWS; y++) {
    for (x=0; x<N_SENSOR_COLUMNS; x++) {
      Bx_avg[x][y] = 0.;
      By_avg[x][y] = 0.;
      Bz_avg[x][y] = 0.;
    }
  }

  start_button.draw();

  Serial.println("This program prints out the average value of each sensor");
  Serial.println("over an integration period and stores the data to the");
  Serial.println("Arduino's LittleFS file system.  Press START, then");
  Serial.println("rotate the field camera 90 degrees every time the screen");
  Serial.println("flashes.  After rotating through 360 degrees,");
  Serial.println("flip the field camera upside down and repeat.");
  tft.setCursor(0, 30,2);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);    tft.setTextFont(2);
  tft.println("This program prints out the average value of each sensor");
  tft.println("over an integration period and stores the data to the");
  tft.println("Arduino's LittleFS file system.  Press Start, then");
  tft.println("rotate the field camera 90 degrees every time the screen");
  tft.println("flashes.  After rotating through 360 degrees,");
  tft.println("flip the field camera upside down and repeat.");
  
  while (true) {
      if (tft.getTouch(&touch_x, &touch_y) && start_button.touched(touch_x,touch_y)) {
            tick = millis();
            cal_phase = 0;
            break;
        }
  }

  while (cal_phase < 8) {
    next_cal_phase = (millis() - tick)/(cal_phase_time);
    if (next_cal_phase > cal_phase) { // Flash screen and ask to rotate
      tft.fillScreen(TFT_WHITE);
      tft.setTextFont(4);
      tft.setCursor(tft.width()/3,tft.height()/3);
      tft.setTextColor(TFT_BLACK);
      if (next_cal_phase == 4) {
        tft.println("FLIP!");
        Serial.println("FLIP!");
      } else {
        tft.println("ROTATE 90");
        Serial.println("ROTATE 90");
      }
      delay(500);
      tft.fillScreen(TFT_BLACK);
      tft.setTextFont(2);
    }
    cal_phase = next_cal_phase;

    n_avgs = n_avgs + 1;
    for (y = 0; y< N_SENSOR_ROWS; y++) {
        for (x = 0; x < N_SENSOR_COLUMNS; x++) {
        which_sensor(x, y, &i2c_dev, &default_i2c_addr, &i2c_addr);
        if (i2c_dev == 0) {
            tmag_object = &tmag5273_i2c0;
        } else {
            tmag_object = &tmag5273_i2c1;
        }
        tmag_object->switchSensor(i2c_addr); 
        res = tmag_object->readMagneticField(&Bx, &By, &Bz, &T);
        if (TMAG5273A2_RESCALE) {
            Bx = Bx * 133/40;
            By = By * 133/40;
            Bz = Bz * 133/40;
        }
        Bx_avg[x][y] = Bx_avg[x][y] + Bx;
        By_avg[x][y] = By_avg[x][y] + By;
        Bz_avg[x][y] = Bz_avg[x][y] + Bz;
        }
    }
    tft.fillRect(0,0,tft.width(),16,TFT_BLACK);
    tft.setCursor(0,0);
    tft.setTextColor(TFT_WHITE);
    tft.print(float(millis()-tick)/1000.0);
    tft.println(" s");
    
  }

  // Take averages
  for (x=0; x<N_SENSOR_COLUMNS; x++) {
    for (y=0; y < N_SENSOR_ROWS; y++) {
      Bx_avg[x][y] = Bx_avg[x][y]/n_avgs;
      By_avg[x][y] = By_avg[x][y]/n_avgs;
      Bz_avg[x][y] = Bz_avg[x][y]/n_avgs;
    }
  }

  File f = LittleFS.open("tmag_cal.csv","w");

 // Print cal data to serial port and to LittleFS in CSV format

  Serial.println("Bx_cal_zero = ");
  print_cal_data(&Serial,Bx_avg);
  print_cal_data(&f,Bx_avg); 

  Serial.println("By_cal_zero = ");
  print_cal_data(&Serial,By_avg); 
  print_cal_data(&f,By_avg); 

  Serial.println("Bz_cal_zero = ");
  print_cal_data(&Serial,Bz_avg);
  print_cal_data(&f,Bz_avg); 
}

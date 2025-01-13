/*
    BMAG Magnetic Field Camera

  https://fabacademy.org/2023/labs/wheaton/students/jason-goodman/final-project/index.html

    Copyright (c) 2023-2025 Jason Goodman <jcgoodman@gmail.com>

    Creative Commons License CC BY-NC-SA
    https://creativecommons.org/licenses/by-nc-sa/4.0/

  You are free to:
  Share — copy and redistribute the material in any medium or format
  Adapt — remix, transform, and build upon the material

  Under the following terms:
  Attribution — You must give appropriate credit , provide a link to the license, and indicate if changes were made
  You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
  NonCommercial — You may not use the material for commercial purposes .
  ShareAlike — If you remix, transform, or build upon the material, you must distribute your contributions under the same license as the original.
  No additional restrictions — You may not apply legal terms or technological measures that legally restrict others from doing anything the license permits.
*/

// Field Camera: Map and display magnetic fields using an array of 3-d Hall effect magnetometers.

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

//uint16_t calData[5] = { 196, 3683, 88, 3707, 1 }; 
uint16_t calData[5] = { 274, 3637, 275, 3449, 1 };// Default touch calibration data

// ------------------------------------------------------
// ===== 3. Magnetometer power configuration
// ------------------------------------------------------

#define N_POWER_PINS 4
int row_power_pin[N_POWER_PINS] = {13,10,26,16};

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

// 
// UI Settings
const uint32_t ui_color = TFT_WHITE;  // Pale magenta
const int scale_arrow_pixels = 50; // Size of scale arrow in pixels
const int button_start_x = scale_arrow_pixels + 40;
const int n_scales = 4;
const float scales[] = {0.05,0.5,5,50}; // A vector arrow the size of the scale arrow is this many milliTeslas.
int selected_scale_index = 0; // Which of the field scales is selected?
bool show_xy = true; // Plot XY component of vectors?
bool show_z = true;  // Plot Z component of vectors?

// Button info
const int button_width = 30;
const int button_height = 30;
const int button_top = SCREEN_HEIGHT - button_height;
const int n_buttons = 7;
// Create an array of buttons
Button buttons[n_buttons] = {Button(&tft,"0.05",  button_start_x,button_top,button_width,button_height,TFT_BLACK,ui_color),
                             Button(&tft,"0.5", button_start_x+  button_width,button_top,button_width,button_height,TFT_BLACK,ui_color),
                             Button(&tft,"5",button_start_x+2*button_width,button_top,button_width,button_height,TFT_BLACK,ui_color),
                             Button(&tft,"50",button_start_x+3*button_width,button_top,button_width,button_height,TFT_BLACK,ui_color),
                             Button(&tft,"XY", button_start_x+6*button_width,button_top,button_width,button_height,TFT_BLACK,ui_color),
                             Button(&tft,"Z",  button_start_x+7*button_width,button_top,button_width,button_height,TFT_BLACK,ui_color),
                             Button(&tft,"Cal",  button_start_x+9*button_width,button_top,button_width,button_height,TFT_BLACK,ui_color)};

void handle_buttons(int touch_x, int touch_y) {
  // Screen was touched, did they tap a button?
  for (int i = 0; i< n_buttons; i++) {
    if (buttons[i].touched(touch_x,touch_y)) {
          if (i < n_scales) {// Scale button was pressed
            selected_scale_index = i;
          } else if (buttons[i].text == "XY") { // XY mode button was pressed, toggle
            show_xy = !show_xy;
          } else if (buttons[i].text == "Z")  { // Z mode button was pressed, toggle
            show_z = !show_z;
          } else if (buttons[i].text == "Cal") { // Cal  button was pressed, calibrate
            do_cal();
          }
    }
  }
}

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
//  device->configOperatingMode(TMAG5273_OPERATING_MODE_STANDBY);
  device->configOperatingMode(TMAG5273_OPERATING_MODE_MEASURE);  // Continuous measurement mode is faster.
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

uint8_t read_tmag_sensor(int x,int y,float *Bx_ptr,float *By_ptr,float *Bz_ptr) {
  uint8_t res;
  int i2c_dev,default_i2c_addr, i2c_addr;
  TMAG5273 *tmag_object;
  float T;

  which_sensor(x, y, &i2c_dev, &default_i2c_addr, &i2c_addr);
  if (i2c_dev == 0) {
      tmag_object = &tmag5273_i2c0;
  } else {
      tmag_object = &tmag5273_i2c1;
  }
  tmag_object->switchSensor(i2c_addr); 
  res = tmag_object->readMagneticField(Bx_ptr, By_ptr, Bz_ptr, &T);
  if (TMAG5273A2_RESCALE) {
      *Bx_ptr = *Bx_ptr * 133/40;
      *By_ptr = *By_ptr * 133/40;
      *Bz_ptr = *Bz_ptr * 133/40;
  }
  return res;
}

// ------------------------------------------------------
// ===== 8. Persistent Globals
// ------------------------------------------------------

int dx[N_SENSOR_COLUMNS][N_SENSOR_ROWS], dy[N_SENSOR_COLUMNS][N_SENSOR_ROWS], dz[N_SENSOR_COLUMNS][N_SENSOR_ROWS];
float Bx_smooth[N_SENSOR_COLUMNS][N_SENSOR_ROWS], By_smooth[N_SENSOR_COLUMNS][N_SENSOR_ROWS], Bz_smooth[N_SENSOR_COLUMNS][N_SENSOR_ROWS];
bool available[N_SENSOR_COLUMNS][N_SENSOR_ROWS];  // True if sensor is available

/* Calibration data */
float Bx_cal_zero[N_SENSOR_COLUMNS][N_SENSOR_ROWS];
float By_cal_zero[N_SENSOR_COLUMNS][N_SENSOR_ROWS];
float Bz_cal_zero[N_SENSOR_COLUMNS][N_SENSOR_ROWS];

// ------------------------------------------------------
// ===== 9. Calibration  Helper Functions
// ------------------------------------------------------

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

void read_cal_data(Stream *s, float field[N_SENSOR_COLUMNS][N_SENSOR_ROWS]) {
  int x,y;
  for (int x=0; x<N_SENSOR_COLUMNS; x++) {// Print out C++ array definition format
    for (int y=0; y < N_SENSOR_ROWS; y++) {
      if (s->available()) {
        field[x][y] = s->parseFloat();
      } else {
        field[x][y] = 0.;
      }
    }
  }
}

void read_calib_file() {
  File f = LittleFS.open("tmag_cal.csv","r");
  read_cal_data(&f, Bx_cal_zero);
  read_cal_data(&f, By_cal_zero);
  read_cal_data(&f, Bz_cal_zero);
  Serial.println("Bx_cal_zero = ");
  print_cal_data(&Serial,Bx_cal_zero);
  Serial.println("By_cal_zero = ");
  print_cal_data(&Serial,By_cal_zero);
  Serial.println("Bz_cal_zero = ");
  print_cal_data(&Serial,By_cal_zero);
}

void do_cal() {
  int n_avgs = 0;
  int tick;
  int cal_phase = 0; // Which rotation step are we on?
  int next_cal_phase;
  int cal_phase_time = 5000; // How long to run each calibration phase, in milliseconds.

  int x,y;

  float Bx, By, Bz;
  uint8_t res;
  // Button info
  const int button_width = 90;
  const int button_height = 30;
  const int button_top = SCREEN_HEIGHT*2/3;
  const int button_center_x = SCREEN_WIDTH/2;

  Button start_button = Button(&tft, "Start", button_center_x-button_width, button_top, 90, 30, TFT_BLACK, TFT_WHITE);
  Button cancel_button = Button(&tft, "Cancel",button_center_x+button_width, button_top, 90, 30, TFT_BLACK, TFT_WHITE);

  uint16_t touch_x, touch_y;

  Serial.println("To calibrate the sensors, move the field camera away");
  Serial.println("from metal objects and press START.  Rotate the");
  Serial.println("field camera 90 degrees every time the screen");
  Serial.println("flashes.  After rotating through 360 degrees,");
  Serial.println("flip the field camera upside down and repeat.");
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 30,2);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);    tft.setTextFont(2);
  tft.println("To calibrate the sensors, move the field camera away");
  tft.println("from metal objects and press START.  Rotate the");
  tft.println("field camera 90 degrees every time the screen");
  tft.println("flashes.  After rotating through 360 degrees,");
  tft.println("flip the field camera upside down and repeat.");
  start_button.draw();
  cancel_button.draw();
  
  while (true) {
    if (tft.getTouch(&touch_x, &touch_y)) {
      if (start_button.touched(touch_x,touch_y)) {
        tick = millis();
        cal_phase = 0;
        break;
      } else {
          tft.fillScreen(TFT_BLACK);
          return;  // Cancel if user clicks anywhere but "Start".
      }
    }
  }

  // Zero averages
  for (y=0; y < N_SENSOR_ROWS; y++) {
    for (x=0; x<N_SENSOR_COLUMNS; x++) {
      Bx_cal_zero[x][y] = 0.;
      By_cal_zero[x][y] = 0.;
      Bz_cal_zero[x][y] = 0.;
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
      delay(1000);
      tft.fillScreen(TFT_BLACK);
      tft.setTextFont(2);
    }
    cal_phase = next_cal_phase;

    n_avgs = n_avgs + 1;
    for (y = 0; y< N_SENSOR_ROWS; y++) {
        for (x = 0; x < N_SENSOR_COLUMNS; x++) {
          res =  read_tmag_sensor(x,y,&Bx,&By,&Bz);
          Bx_cal_zero[x][y] += Bx;
          By_cal_zero[x][y] += By;
          Bz_cal_zero[x][y] += Bz;
        }
    }
    tft.fillRect(0,0,tft.width()/3,16,TFT_BLACK);
    tft.setCursor(0,0);
    tft.setTextColor(TFT_WHITE);
    tft.print(float(millis()-tick)/1000.0);
    tft.println(" s");    
  }

  // Calculate average from sum
  for (x=0; x<N_SENSOR_COLUMNS; x++) {
    for (y=0; y < N_SENSOR_ROWS; y++) {
      Bx_cal_zero[x][y] = Bx_cal_zero[x][y]/n_avgs;
      By_cal_zero[x][y] = By_cal_zero[x][y]/n_avgs;
      Bz_cal_zero[x][y] = Bz_cal_zero[x][y]/n_avgs;
    }
  }

 // Print cal data to serial port and to LittleFS in CSV format

  File f = LittleFS.open("tmag_cal.csv","w");

  Serial.println("Bx_cal_zero = ");
  print_cal_data(&Serial,Bx_cal_zero);
  print_cal_data(&f,Bx_cal_zero); 

  Serial.println("By_cal_zero = ");
  print_cal_data(&Serial,By_cal_zero); 
  print_cal_data(&f,By_cal_zero); 

  Serial.println("Bz_cal_zero = ");
  print_cal_data(&Serial,Bz_cal_zero);
  print_cal_data(&f,Bz_cal_zero); 
}

// ------------------------------------------------------
// ===== 10. Setup
// ------------------------------------------------------

void setup() {
  int x,y;
  model_id model;
  int i2c_dev, default_i2c_addr, new_i2c_addr;
  byte error;

  Serial.begin(115200);
  delay(1000); // Wait a bit for serial

  // LittleFS startup
  LittleFS.begin();

  // --- Magnetometer array setup --- 

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
        tmag5273_i2c0.switchSensor(default_i2c_addr);
        tmag5273_i2c0.modifyI2CAddress(new_i2c_addr); // change I2C address
        delay(1);
        Wire.beginTransmission(new_i2c_addr);
        error = Wire.endTransmission();
      } else {
        tmag5273_i2c1.switchSensor(default_i2c_addr);
        tmag5273_i2c1.modifyI2CAddress(new_i2c_addr); // change I2C address
        delay(1);
        Wire1.beginTransmission(new_i2c_addr);
        error = Wire1.endTransmission();
      }
      available[x][y] = (error == 0);
    }
  }
  Serial.println("Sensor availability:");
  for (y=0; y < N_SENSOR_ROWS; y++) {
      for (x=0; x<N_SENSOR_COLUMNS; x++) {
        Serial.print(available[x][y]?"+":"-");
      }
    Serial.println("");
  }
  setup_tmag(&tmag5273_i2c0);
  setup_tmag(&tmag5273_i2c1);

  read_calib_file();
  
  // --- TFT setup --- 

  // Set up TFT, black screen medium font.
  tft.init(); // Not sure if both init() and begin() are needed or what the difference is
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0);
  tft.setTextFont(2);
  // set up touch calibration
  tft.setTouch(calData);

  // Zero persistent variables
  for (y=0; y < N_SENSOR_ROWS; y++) {
    for (x=0; x<N_SENSOR_COLUMNS; x++) {
      dx[x][y] = 0;
      dy[x][y] = 0;
      dz[x][y] = 0;
      Bx_smooth[x][y] = 0.;
      By_smooth[x][y] = 0.;
      Bz_smooth[x][y] = 0.;
    }
  }
}

// ------------------------------------------------------
// ===== 11. Loop
// ------------------------------------------------------

void loop() {
  const int smooth_level = 2;  // Strength of autoregressive smoothing filter
  int x,y;

  float Bx, By, Bz, T;
  uint8_t res;

  uint16_t touch_x, touch_y;

  int tick;
  
  tick = millis();

  for (y = 0; y< N_SENSOR_ROWS; y++) {
    for (x = 0; x < N_SENSOR_COLUMNS; x++) {
      if (available[x][y]) {
        res = read_tmag_sensor(x,y,&Bx,&By,&Bz);
      } else {
        Bx = 0;
        By = 0;
        Bz = 0;
      }

      Bx = Bx - Bx_cal_zero[x][y];
      By = By - By_cal_zero[x][y];
      Bz = Bz - Bz_cal_zero[x][y];

    // Apply autoregressive smoother.  Division only by 2 to avoid floating-point arithmetic.
    for (int i=0; i<smooth_level; i++) {
      Bx = (Bx + Bx_smooth[x][y])/2;
      By = (By + By_smooth[x][y])/2;
      Bz = (Bz + Bz_smooth[x][y])/2;
    }
    Bx_smooth[x][y] = Bx;
    By_smooth[x][y] = By;
    Bz_smooth[x][y] = Bz;

    // Clear old graphics
    draw_arrow(&tft, sensor_x[x], sensor_y[y], dx[x][y], dy[x][y], TFT_BLACK);
    draw_z_arrow(&tft, sensor_x[x], sensor_y[y], dz[x][y], TFT_BLACK);

    // Scale field for display, rotate into screen coodinate system.
    // Sensor +X is screen +Y, Sensor +Y is screen -X, Sensor +Z is screen -Z
    // See Fig 7-1 of TMAG5273 datasheet.

    dx[x][y] = -scale_arrow_pixels * By_smooth[x][y] / scales[selected_scale_index];
    dy[x][y] = scale_arrow_pixels * Bx_smooth[x][y] / scales[selected_scale_index];
    dz[x][y] = -scale_arrow_pixels * Bz_smooth[x][y]/(2*scales[selected_scale_index]);

    // Draw arrows
    if (show_xy) {
      draw_arrow(&tft, sensor_x[x], sensor_y[y], dx[x][y], dy[x][y], TFT_MAGENTA | TFT_DARKGREEN); // Pale magenta arrows
    }
    if (show_z) {
      if (dz[x][y] > 0) {
        draw_z_arrow(&tft, sensor_x[x], sensor_y[y], dz[x][y], TFT_SKYBLUE);
      } else {
        draw_z_arrow(&tft, sensor_x[x], sensor_y[y], dz[x][y], TFT_PINK);
      }
    }

    Serial.print(Bz);
    Serial.print(" ");
    }
    Serial.println();
  }

  // Draw scale arrow
  draw_arrow(&tft, scale_arrow_pixels/2, button_top+button_width/2, scale_arrow_pixels, 0, ui_color);
  draw_z_arrow(&tft, scale_arrow_pixels/2, button_top+button_width/2, scale_arrow_pixels/2, ui_color);

  // Draw buttons
  for (int i = 0; i < n_buttons; i++) {
    if (i < n_scales) { // Draw scale buttons, highlighting the selected scale
      buttons[i].selected = (i == selected_scale_index); // Highlight only the selected scale button
      buttons[i].draw();
    }
    if (buttons[i].text == "XY") { // Draw XY button, highlighting if XY vectors are shown
      buttons[i].selected = show_xy;
      buttons[i].draw();
    }
    if (buttons[i].text == "Z") { // Draw Z button, highlighting if Z vector is shown
      buttons[i].selected = show_z;
      buttons[i].draw();
    }
    if (buttons[i].text == "Cal") { // Draw Cal button
      buttons[i].selected = 0;
      buttons[i].draw();
    }
  }

  // Draw text labels 
  tft.setTextColor(ui_color);
  tft.drawString("=",button_start_x - 7,button_top+button_width/2); 
  tft.drawString("mT",button_start_x+4.5*button_width,button_top+button_width/2); 
  
  // Look for touch
  
  tft.fillCircle(touch_x, touch_y, 2, TFT_BLACK);
  if (tft.getTouch(&touch_x, &touch_y)) {
    handle_buttons(touch_x,touch_y);
    tft.fillCircle(touch_x, touch_y, 2, TFT_WHITE);
  }
  Serial.print("Time = ");
  Serial.print(millis()-tick);
  Serial.println("ms");
  Serial.println();
}

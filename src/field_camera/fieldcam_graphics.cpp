
// ------------------------------------------------------
// ===== 4. Graphics helper functions and classes
// ------------------------------------------------------

#include <Arduino.h>
#include "fieldcam_graphics.h"
Button::Button(TFT_eSPI *settft, String settext, int setx, int sety,int setdx, int setdy,
        uint32_t setbgcolor, uint32_t setfgcolor,bool setselected) {
      this->tft = settft;
      this->text = settext;
      this->x = setx;
      this->y = sety;
      this->dx = setdx;
      this->dy = setdy;
      this->bgcolor = setbgcolor;
      this->fgcolor = setfgcolor;
      this->selected = setselected;
      this->last_touch = millis();
    }

    // Draw a button at position x,y with width, length dx,dy, containing text, with a chosen foreground and background color.
void Button::draw() {
      uint32_t color1,color2;
      if (this->selected) {// reverse colors to highlight
      color1 = this->fgcolor;
      color2 = this->bgcolor;
      } else { 
        color1 = this->bgcolor;
        color2 = this->fgcolor;
      }
      tft->fillRect(this->x, this->y, this->dx, this->dy, color1);
      tft->drawRect(this->x,this->y,this->dx,this->dy,color2);
      tft->setTextDatum(MC_DATUM);
      tft->setTextColor(color2);
      tft->setTextFont(2);
      tft->drawString(this->text,this->x+this->dx/2, this->y+this->dy/2);
    }

    // Is this button being touched?  Return true if (x,y) are inside this button 
    // and it hasn't been tapped recently (avoid double-taps).
bool Button::touched(int touch_x, int touch_y) {
      if ((touch_x >= this->x) && (touch_x < (this->x+this->dx)) && 
        (touch_y >= this->y) && (touch_y < (this->y+this->dy)) &&
        ((millis()-200 > this->last_touch))) {
          this->last_touch = millis();
          return true;
        } else {
          return false;
        }
    }

// Draw a dot or cross to indicate the vertical component of the field
void draw_z_arrow(TFT_eSPI *tft, int x, int y, int z, uint32_t color) {
    int rcos45 = (z*71/100)/2; // Diagonal x,ydistance for cross
    if (z>0) {//Draw a dot
      tft->drawCircle(x,y,z/2,color);
      tft->fillCircle(x,y,z/10,color);
    } else { // Draw a cross
      tft->drawCircle(x,y,-z/2,color);
      tft->drawLine(x+rcos45,y+rcos45,x-rcos45,y-rcos45,color);
      tft->drawLine(x+rcos45,y-rcos45,x-rcos45,y+rcos45,color);
    }
}

// Draw an arrow starting from position (x,y), with length (dx, dy).
// to indicate the horizontal component of the field
void draw_arrow(TFT_eSPI *tft,int x, int y, int dx, int dy, uint32_t color) {

  // Unit vector shape.  Each array element is one line.
  int n_lines = 3;
  int unit_startx[] = {0,100,100};
  int unit_starty[] = {0,0,0};
  int unit_endx[] =   {100,85,85};
  int unit_endy[] =   {0,10,-10};

  // Scaled/rotated shape
  int startx[] = {0,0,0};
  int starty[] = {0,0,0};
  int endx[] =   {0,0,0};
  int endy[] =   {0,0,0};

  for(int i = 0; i<n_lines; i++) {  // Rotate and scale unit vector shape to final coordinates
    startx[i] = x+(unit_startx[i]*dx - unit_starty[i]*dy)/100;
    endx[i] =   x+(unit_endx[i]*dx   - unit_endy[i]*dy  )/100;
    starty[i] = y+(unit_startx[i]*dy + unit_starty[i]*dx)/100;
    endy[i] =   y+(unit_endx[i]*dy   + unit_endy[i]*dx  )/100;
    tft->drawLine(startx[i], starty[i], endx[i], endy[i],  color); // Draw the line
  }  
}

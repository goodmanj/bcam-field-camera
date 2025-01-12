
// ------------------------------------------------------
// ===== 4. Graphics helper functions and classes
// ------------------------------------------------------

#ifndef FIELDCAM_GRAPHICS
#define FIELDCAM_GRAPHICS
#include <TFT_eSPI.h>

// An onscreen button class for the TFT display
class Button {
  private:
    int x,y,dx,dy;
    uint32_t bgcolor,fgcolor;
    int last_touch;
    TFT_eSPI *tft;
  public:
    bool selected;  // Expose these as public because I'm too lazy to create access functions
    String text;
    // Constructor
    Button(TFT_eSPI *settft, String settext, int setx, int sety,int setdx=20, int setdy=20,
        uint32_t setbgcolor=TFT_BLACK, uint32_t setfgcolor=TFT_WHITE,bool setselected=true);

    // Draw a button at position x,y with width, length dx,dy, containing text, with a chosen foreground and background color.
    void draw();

    // Is this button being touched?  Return true if (x,y) are inside this button 
    // and it hasn't been tapped recently (avoid double-taps).
    bool touched(int touch_x, int touch_y);
};

// Draw a dot or cross to indicate the vertical component of the field
void draw_z_arrow(TFT_eSPI *tft, int x, int y, int z, uint32_t color);

// Draw an arrow starting from position (x,y), with length (dx, dy).
// to indicate the horizontal component of the field
void draw_arrow(TFT_eSPI *tft, int x, int y, int dx, int dy, uint32_t color);

#endif

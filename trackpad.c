/**
 * Hunter Adams (vha3@cornell.edu)
 * 
 * Uses resistive touchscreen to draw to VGA.
 * 
 * https://vanhunteradams.com/Pico/VGA/Trackpad.html
 * 
 *
 */

#include "vga16_graphics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "image_buffer.h"
#include "classify.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "ssd1306.h"

// Attached to resistive touchscreen
// #define XMINUS 6
// #define YPLUS 7
// #define XPLUS 8
// #define YMINUS 9

#define XMINUS 8
#define YPLUS 7
#define XPLUS 6
#define YMINUS 9
#define STATE_BUTTON 15
#define DRAW_BUTTON 14
#define BUTTON_COOLDOWN 100000

// Setup for reading the y coordinate
// Y+ and Y- set to input (high impedance)
// X+ and X- set to output
void setupY(void) {

    // direction
    gpio_set_dir(XMINUS, GPIO_OUT) ;
    gpio_set_dir(XPLUS, GPIO_OUT) ;
    gpio_set_dir(YPLUS, GPIO_IN) ;
    gpio_set_dir(YMINUS, GPIO_IN) ;

    // values
    gpio_put(XMINUS, 0);
    gpio_put(XPLUS, 1);

}

// Setup for reading the x coordinate
// X+ and X- set to input (high impedance)
// Y+ and Y- set to output
void setupX(void) {

    // direction
    gpio_set_dir(XMINUS, GPIO_IN) ;
    gpio_set_dir(XPLUS, GPIO_IN) ;
    gpio_set_dir(YPLUS, GPIO_OUT) ;
    gpio_set_dir(YMINUS, GPIO_OUT) ;

    // values
    gpio_put(YMINUS, 0);
    gpio_put(YPLUS, 1);

}

volatile uint8_t img[28][28] = {0};
volatile bool    frame_ready = false;    
volatile bool    clear_img = false;

// Selects between measuring x/y coord
volatile int chooser;

// Raw ADC measurements
volatile uint adc_x_raw ;
volatile uint adc_y_raw ;

// X/Y buffers for filtering
volatile uint xarray[16] ;
volatile uint yarray[16] ;

// Accumulation and output of filter
volatile float xout ;
volatile float yout ;
volatile int xret ;
volatile int yret ;

int ix ;
int iy ;

// Pointer to index in sample buffer
unsigned char xpointer = 0 ;
unsigned char ypointer = 0 ;

// Single-core state machine
typedef enum { DRAWING, CLASSIFY } AppState;
volatile AppState app_state = DRAWING;

typedef enum { NODRAW, DRAW } DrawState;
volatile DrawState draw_state = NODRAW;

static volatile uint32_t last_app_press = 0;
static volatile uint32_t last_draw_press = 0;

static void button_irq_handler(uint gpio, uint32_t events)
{
    uint32_t now = time_us_32();

    if (gpio == STATE_BUTTON) {
        if (now - last_app_press > BUTTON_COOLDOWN) {
            last_app_press = now;
            app_state = (app_state == DRAWING) ? CLASSIFY : DRAWING;
        }
    }
    if (gpio == DRAW_BUTTON) {
        if (now - last_draw_press > BUTTON_COOLDOWN) {
            last_draw_press = now;
            draw_state = (draw_state == NODRAW) ? DRAW : NODRAW;
        }
    }
}

// Timer ISR
bool repeating_timer_callback(struct repeating_timer *t) {
    // Read X coord
    if (chooser == 1) {
        setupX();
        adc_select_input(0);
        adc_read();
        adc_read();
        adc_x_raw = adc_read();

        xarray[xpointer & 0x0F] = adc_x_raw ;

        xout = 0 ;
        xret = 0 ;
        for (int i=0; i<16; i++) {
            xout += xarray[i] ;
        }
        xret = ((uint)(xout))>>4 ;

        xpointer += 1 ;

        chooser = 0 ;
    }
    // Read Y coord
    else {
        setupY() ;
        adc_select_input(1);
        adc_read();
        adc_read();
        adc_y_raw = adc_read();

        yarray[ypointer & 0x0F] = adc_y_raw ;

        yout = 0 ;
        yret = 0 ;
        for (int i=0; i<16; i++) {
            yout += yarray[i] ;
        }
        yret = ((uint)(yout))>>4 ;

        ypointer += 1 ;

        chooser = 1 ;
    }

    // Scale and draw to VGA
    if ((xret<2200) && (xret>1600) && (yret<3400) && (yret>450)) {
        drawPixel(640-(((xret - 1600) * (640)) / (2200 - 1600)),
            (((yret - 450) * (480)) / (3400 - 450)), WHITE) ;
    }
}

void clear_touchpad() {
    if (clear_img) {
        for (size_t y = 0; y < 28; ++y) {
            for (size_t x = 0; x < 28; ++x) {
                img[y][x] = 0;
            }
        }
        clear_img = false;
    }
}

// void read_touchpad() {
//     if ((xret < 2200) && (xret > 1600) && (yret < 3400) && (yret > 450)) {
//         // map into [0..27]
//         int ix = 28 - (((xret - 1600) * 28) / (2200 - 1600));
//         if (ix < 0)   ix = 0;
//         if (ix > 27)  ix = 27;
//         int iy =      ((yret - 450) * 28) / (3400 - 450);
//         if (iy < 0)   iy = 0;
//         if (iy > 27)  iy = 27;
    
//         printf("%d, %d\t", ix, iy);
    
//         // draw a 2x2 square centered at (ix,iy)
//         for (int dy = 0; dy <= 1; dy++) {
//             int y = iy + dy;
//             if (y < 0 || y >= 28) continue;
//             for (int dx = 0; dx <= 1; dx++) {
//                 int x = ix + dx;
//                 if (x < 0 || x >= 28) continue;
//                 if (dx == 0 && dy == 0) { img[y][x] = 255; } 
//                 else if (img[y][x] != 255) { img[y][x] = 255; }
//             }
//         }
//     }
//     printf("ARRAY\n");
//     for (int i = 0; i < 28; i++) {
//         for (int j = 0; j < 28; j++) {
//             printf("%d ", img[i][j]);
//         }
//         printf("\n");
//     }
//     printf("\n");
// }

/* assumes img[28][28] is global or passed in */
/* 90° CW: (orig_x, orig_y)  →  (new_x = 27‑orig_y , new_y = orig_x) */

#define N 28       // side length of the image

#define N 28        // image side length

// void read_touchpad(void)
// {
//     if ((xret < 2200) && (xret > 1600) &&
//         (yret < 3400) && (yret > 450))
//     {
//         /* map raw touch into 0‥27 */
//         int ix = 28 - (((xret - 1600) * N) / (2200 - 1600));
//         if (ix < 0) ix = 0;
//         if (ix > 27) ix = 27;

//         int iy =     ((yret - 450) * N) / (3400 - 450);
//         if (iy < 0) iy = 0;
//         if (iy > 27) iy = 27;

//         printf("%d, %d\t", ix, iy);

//         /* draw a 2×2 square — store it already rotated 90° CW */
//         for (int dy = -1; dy <= 1; dy++) {
//             int y_orig = iy + dy;             // original Y
//             if (y_orig >= N) continue;

//             for (int dx = -1; dx <= 1; dx++) {
//                 int x_orig = ix + dx;         // original X
//                 if (x_orig >= N) continue;

//                 /* correct clockwise mapping */
//                 int y_rot = N - 1 - x_orig;   // row index in img
//                 int x_rot = y_orig;           // column index in img

//                 if (dx == 0 && dy == 0) {
//                     img[y_rot][x_rot] = 255;
//                 } else if (img[y_rot][x_rot] != 255) {
//                     img[y_rot][x_rot] = 255;
//                 }
//             }
//         }
//     }

//     /* debug dump */
//     printf("ARRAY (90° CW)\n");
//     for (int i = 0; i < N; ++i) {
//         for (int j = 0; j < N; ++j) {
//             printf("%d ", img[i][j]);
//         }
//         printf("\n");
//     }
//     printf("\n");
// }


//SAMPLE READ TOUCH PAD-----------------------------------------







//SAMPLE READ TOUCH PAD END --------------------------------------------






static void oled_init(void)
{
    i2c_init(i2c_default, 400 * 1000);          // 400 kHz
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    SSD1306_Init();        // from the library
    SSD1306_Clear();
    SSD1306_UpdateScreen();
}

static void oled_show_number(uint32_t value)
{
    char buf[8];                           // 7 digits + NUL
    snprintf(buf, sizeof(buf), "%07u", value);

    SSD1306_GotoXY(0, 0);                  // X-col, Y-row (pixels)
    SSD1306_Puts(buf, &Font_7x10, 1);      // built-in 7 × 10 font
    SSD1306_UpdateScreen();
}


int main() {

    // Initialize stdio
    stdio_init_all();

    // Initialize ADC
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    gpio_init(26); // X+ (ADC input)
    gpio_set_dir(26, GPIO_IN);
    gpio_disable_pulls(26); // Important!
    gpio_init(STATE_BUTTON);
    gpio_set_dir(STATE_BUTTON, GPIO_IN);
    gpio_pull_up(STATE_BUTTON);
    gpio_init(DRAW_BUTTON);
    gpio_set_dir(DRAW_BUTTON, GPIO_IN);
    gpio_pull_up(DRAW_BUTTON);
    adc_gpio_init(26);  
    adc_gpio_init(27);

    // Initialize the VGA screen
    initVGA() ;

    // Initialize GPIO
    gpio_init(XMINUS) ;
    gpio_init(YPLUS) ;
    gpio_init(XPLUS) ;
    gpio_init(YMINUS) ;

        /* NEW: OLED init + quick test */
        oled_init();
        oled_show_number(1234567);
        sleep_ms(2000);
        SSD1306_Clear();
        SSD1306_UpdateScreen();

    // Repeating timer struct
    struct repeating_timer timer;

    // Negative delay so means we will call repeating_timer_callback, and call it again
    // 25us (40kHz) later regardless of how long the callback took to execute
    add_repeating_timer_us(-4000, repeating_timer_callback, NULL, &timer);

    gpio_set_irq_enabled_with_callback(STATE_BUTTON, GPIO_IRQ_EDGE_FALL, true, &button_irq_handler);

    gpio_set_irq_enabled(DRAW_BUTTON, GPIO_IRQ_EDGE_FALL, true);

    cnn_setup();

    while(1) {
        switch (app_state) {
            case DRAWING:
                if (draw_state == DRAW) {
                    clear_touchpad();
                    frame_ready = false;
                    read_touchpad();
                }
                else if (draw_state == NODRAW) {
                    frame_ready = true;
                    clear_img = true;
                }
                break;
            case CLASSIFY:
                cnn_run_if_frame_ready();
                int best = return_digit();
                oled_show_number(best);
                app_state = DRAWING;
                break;
        }
    }

}

//MAIN TESTING SECTION------------------------------------------------------------------------------------------------








//MAIN TESTING SECTION------------------------------------------------------------------------------------------------










// uint32_t last_app_button_time = 0;
// uint32_t last_draw_button_time = 0;

// bool debounce_app_state_button() {
//     if (gpio_get(STATE_BUTTON) == 0) {  
//       uint32_t current_time = time_us_32();
//       if (current_time - last_app_button_time > BUTTON_COOLDOWN*5){
//         if (gpio_get(STATE_BUTTON) == 0) {  
//           last_app_button_time = current_time;
//           return true;
//         }
//       }
//     }
//     return false;
// }
  
// //Function to update the state machine
// void update_app_state() {
//     if (debounce_app_state_button()) {
//         app_state = (app_state == DRAWING) ? CLASSIFY : DRAWING;
//     }
// }

// bool debounce_draw_state_button() {
//     if (gpio_get(DRAW_BUTTON) == 0) {  
//       uint32_t current_time = time_us_32();
//       if (current_time - last_draw_button_time > BUTTON_COOLDOWN*5){
//         if (gpio_get(DRAW_BUTTON) == 0) {  
//           last_draw_button_time = current_time;
//           return true;
//         }
//       }
//     }
//     return false;
// }

// void update_draw_state() {
//     if (debounce_draw_state_button()) {
//         draw_state = (draw_state == NODRAW) ? DRAW : NODRAW;
//     }
// }

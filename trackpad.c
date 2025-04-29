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
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"

// Attached to resistive touchscreen
// #define XMINUS 6
// #define YPLUS 7
// #define XPLUS 8
// #define YMINUS 9

#define XMINUS 8
#define YPLUS 7
#define XPLUS 6
#define YMINUS 9

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

static uint8_t img[28][28] = {0};

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


int main() {

    // Initialize stdio
    stdio_init_all();

    // Initialize ADC
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    gpio_init(26); // X+ (ADC input)
    gpio_set_dir(26, GPIO_IN);
    gpio_disable_pulls(26); // Important!
    adc_gpio_init(26);  
    adc_gpio_init(27);

    // Initialize the VGA screen
    initVGA() ;

    // Initialize GPIO
    gpio_init(XMINUS) ;
    gpio_init(YPLUS) ;
    gpio_init(XPLUS) ;
    gpio_init(YMINUS) ;

    // Repeating timer struct
    struct repeating_timer timer;

    // Negative delay so means we will call repeating_timer_callback, and call it again
    // 25us (40kHz) later regardless of how long the callback took to execute
    add_repeating_timer_us(-4000, repeating_timer_callback, NULL, &timer);

    while(1) {
        //printf("x = %d, y = %d\n", adc_x_raw, adc_y_raw);
        // if ((xret<2200) && (xret>1600) && (yret<3400) && (yret>450)) {
        //     ix = 28 - (((xret - 1600) * 28) / (2200 - 1600));
        //     if (ix < 0)   ix = 0;
        //     if (ix > 27)  ix = 27;
        //     iy =      ((yret - 450) * 28) / (3400 - 450);
        //     if (iy < 0)   iy = 0;
        //     if (iy > 27)  iy = 27;
        //     printf("%d, %d\t", ix, iy);
        //     img[iy][ix] = 1;
        // }

        if ((xret < 2200) && (xret > 1600) && (yret < 3400) && (yret > 450)) {
            // map into [0..27]
            int ix = 28 - (((xret - 1600) * 28) / (2200 - 1600));
            if (ix < 0)   ix = 0;
            if (ix > 27)  ix = 27;
            int iy =      ((yret - 450) * 28) / (3400 - 450);
            if (iy < 0)   iy = 0;
            if (iy > 27)  iy = 27;
        
            printf("%d, %d\t", ix, iy);
        
            // draw a 3×3 square centered at (ix,iy)
            for (int dy = -1; dy <= 0; dy++) {
                int y = iy + dy;
                if (y < 0 || y >= 28) continue;
                for (int dx = 0; dx <= 1; dx++) {
                    int x = ix + dx;
                    if (x < 0 || x >= 28) continue;
                    img[y][x] = 1;
                }
            }
        }

        printf("ARRAY\n");
        for (int i = 0; i < 28; i++) {
            for (int j = 0; j < 28; j++) {
                printf("%d ", img[i][j]);
            }
            printf("\n");
        }
        printf("\n");
        //sleep_ms(10000);
    }

}

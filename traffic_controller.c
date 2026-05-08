#include "LPC17xx.h"

// ------------------------------------
// PIN DEFINITIONS
// ------------------------------------

// Lane 1 LEDs
#define L1_RED      (1 << 0)   // P1.0
#define L1_YELLOW   (1 << 1)   // P1.1
#define L1_GREEN    (1 << 4)   // P1.4

// Lane 2 LEDs
#define L2_RED      (1 << 8)   // P1.8
#define L2_YELLOW   (1 << 9)   // P1.9
#define L2_GREEN    (1 << 10)  // P1.10

// Lane 3 LEDs
#define L3_RED      (1 << 14)  // P1.14
#define L3_YELLOW   (1 << 15)  // P1.15
#define L3_GREEN    (1 << 16)  // P1.16

// Lane 4 LEDs
#define L4_RED      (1 << 17)  // P1.17
#define L4_YELLOW   (1 << 18)  // P1.18
#define L4_GREEN    (1 << 19)  // P1.19

// IR Sensors (Active LOW assumed)
#define IR1         (1 << 0)   // P0.0
#define IR2         (1 << 1)   // P0.1
#define IR3         (1 << 4)   // P0.4
#define IR4         (1 << 5)   // P0.5

// ALL LEDs
#define ALL_LEDS    (L1_RED|L1_YELLOW|L1_GREEN|L2_RED|L2_YELLOW|L2_GREEN|L3_RED|L3_YELLOW|L3_GREEN|L4_RED|L4_YELLOW|L4_GREEN)
#define ALL_RED     (L1_RED|L2_RED|L3_RED|L4_RED)
#define ALL_YELLOW  (L1_YELLOW|L2_YELLOW|L3_YELLOW|L4_YELLOW)
#define ALL_GREEN   (L1_GREEN|L2_GREEN|L3_GREEN|L4_GREEN)

// ------------------------------------
// DELAY
// ------------------------------------
void delay_ms(uint32_t ms) {
    uint32_t i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 6000; j++);
}

// ------------------------------------
// ALL RED
// ------------------------------------
void allRed(void) {
    LPC_GPIO1->FIOSET = ALL_RED;
    LPC_GPIO1->FIOCLR = ALL_YELLOW | ALL_GREEN;
}

// ------------------------------------
// READ SENSOR (ACTIVE LOW)
// ------------------------------------
int readIR(int lane) {
    switch(lane) {
        case 0: return (LPC_GPIO0->FIOPIN & IR1) ? 0 : 1;
        case 1: return (LPC_GPIO0->FIOPIN & IR2) ? 0 : 1;
        case 2: return (LPC_GPIO0->FIOPIN & IR3) ? 0 : 1;
        case 3: return (LPC_GPIO0->FIOPIN & IR4) ? 0 : 1;
        default: return 0;
    }
}

// ------------------------------------
// PRIORITY SELECTION
// ------------------------------------
int getPriorityLane(int currentLane) {
    int i;
    for (i = 1; i <= 4; i++) {
        int next = (currentLane + i) % 4;
        if (readIR(next)) return next;
    }
    return (currentLane + 1) % 4;
}

// ------------------------------------
// SERVE LANE
// ------------------------------------
void serveLane(int lane) {

    uint32_t red, yellow, green;

    switch(lane) {
        case 0: red=L1_RED; yellow=L1_YELLOW; green=L1_GREEN; break;
        case 1: red=L2_RED; yellow=L2_YELLOW; green=L2_GREEN; break;
        case 2: red=L3_RED; yellow=L3_YELLOW; green=L3_GREEN; break;
        case 3: red=L4_RED; yellow=L4_YELLOW; green=L4_GREEN; break;
        default: return;
    }

    int detected = readIR(lane);
    int greenTime = detected ? 8000 : 3000;

    // All red
    allRed();
    delay_ms(500);

    // Yellow (prepare)
    LPC_GPIO1->FIOCLR = red;
    LPC_GPIO1->FIOSET = yellow;
    delay_ms(2000);

    // Green
    LPC_GPIO1->FIOCLR = yellow;
    LPC_GPIO1->FIOSET = green;
    delay_ms(greenTime);

    // Yellow (stop)
    LPC_GPIO1->FIOCLR = green;
    LPC_GPIO1->FIOSET = yellow;
    delay_ms(2000);

    // Back to red
    LPC_GPIO1->FIOCLR = yellow;
    LPC_GPIO1->FIOSET = red;
    delay_ms(500);
}

// ------------------------------------
// MAIN
// ------------------------------------
int main(void) {

    // ------------------------------------
    // FORCE GPIO MODE (CRITICAL FIX)
    // ------------------------------------

    // Port 1
    LPC_PINCON->PINSEL2 &= ~((3<<0)|(3<<2)|(3<<8));             // P1.0,1,4
    LPC_PINCON->PINSEL3 &= ~((3<<16)|(3<<18)|(3<<20));          // P1.8,9,10
    LPC_PINCON->PINSEL3 &= ~((3<<28)|(3<<30));                  // P1.14,15
    LPC_PINCON->PINSEL4 &= ~((3<<0)|(3<<2)|(3<<4)|(3<<6));      // P1.16–19

    // Port 0 (IR sensors)
    LPC_PINCON->PINSEL0 &= ~((3<<0)|(3<<2)|(3<<8)|(3<<10));     // P0.0,1,4,5

    // ------------------------------------
    // GPIO DIRECTION
    // ------------------------------------

    LPC_GPIO1->FIODIR |= ALL_LEDS;             // LEDs as output
    LPC_GPIO0->FIODIR &= ~(IR1|IR2|IR3|IR4);   // Sensors as input

    // Pull-up resistors enabled (default = 00)
    LPC_PINCON->PINMODE0 &= ~((3<<0)|(3<<2)|(3<<8)|(3<<10));

    // ------------------------------------
    // INITIAL STATE
    // ------------------------------------

    allRed();
    delay_ms(2000);

    int currentLane = 0;

    while(1) {
        serveLane(currentLane);
        currentLane = getPriorityLane(currentLane);
    }
}

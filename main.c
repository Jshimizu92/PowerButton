#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"

#define OFF 0
#define ON 1

#define BTN_PINS GPIO_PIN_3
#define LED_PINS GPIO_PIN_5

#define TOGGLE_COUNT 500
#define DEBOUNCE_DELAY 10
#define BUTTONS_NOT_PRESSED 8

void debounce_buttons(void);
void disable_interrupts(void);
void enable_interrupts(void);
void wait_for_interrupts(void);
void Button_Handler(void);

volatile unsigned long count = 0; // Every 12.5ns*(period); for timer-interrupt
volatile unsigned long timeCount = 0; // Every 1 ms; for debouncing

volatile unsigned long ledState = 32;
volatile unsigned long lastDebounceTime = 0;
volatile unsigned long In, Out;

_Bool areButtonsToggledOff = 1;

void setup(void) {
    // Setting Clock to 40MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    //Enabling Port B
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //Setting LED pins to Output
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, LED_PINS);

    // Set SW1 GPIO as inputs
    GPIODirModeSet(GPIO_PORTB_BASE, BTN_PINS , GPIO_DIR_MODE_IN);

    //
    GPIOIntRegister(GPIO_PORTB_BASE, Button_Handler);

    // Enable internal pull up resistors
    GPIOPadConfigSet(GPIO_PORTB_BASE, BTN_PINS , GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOPadConfigSet(GPIO_PORTB_BASE, LED_PINS , GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
}

/* main */
int main(void){
  setup();

  enable_interrupts();

  while(1){                   // interrupts every 1ms
      GPIOPinWrite(GPIO_PORTB_BASE, LED_PINS, ledState);
      wait_for_interrupts();
      debounce_buttons();
  }
}

/* Disable interrupts by setting the I bit in the PRIMASK system register */
void disable_interrupts(void) {
    __asm("    CPSID  I\n"
          "    BX     LR");
}

/* Enable interrupts by clearing the I bit in the PRIMASK system register */
void enable_interrupts(void) {
    __asm("    CPSIE  I\n"
          "    BX     LR");
}

/* Enter low-power mode while waiting for interrupts */
void wait_for_interrupts(void) {
    __asm("    WFI\n"
          "    BX     LR");
}

/* Interrupt GPIO_PORTB when button Pressed */
void Button_Handler(void){
    GPIOIntClear(GPIO_PORTB_BASE, BTN_PINS);

    _Bool wasAButtonPressed = (GPIOPinRead(GPIO_PORTB_BASE, BTN_PINS) != BUTTONS_NOT_PRESSED);
    if(wasAButtonPressed) {
        lastDebounceTime = timeCount;
    }
}

// Helper function for Button Reading
void debounce_buttons(void){
    timeCount++;

    //TODO(Rebecca): Can this logic be moved to Button_Handler? If not, why?
    _Bool hasDebounced = timeCount > lastDebounceTime + DEBOUNCE_DELAY;
    if (hasDebounced) {
        _Bool wasAButtonPressed = (GPIOPinRead(GPIO_PORTB_BASE, BTN_PINS) != BUTTONS_NOT_PRESSED);
        if(wasAButtonPressed) {
            // Checking if button has already been toggled during a button press
            if(areButtonsToggledOff) {
                ledState = 0;
                areButtonsToggledOff = 0;
            }
        } else {
            areButtonsToggledOff = 1;
        }
    }
}



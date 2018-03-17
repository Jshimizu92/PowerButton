// Rebecca Dun
// Jacob Shimizu

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
#define LED_ON 32

// Period = (80 MHz) / (Desired Frequency Hz) - 1
#define DEBOUNCE_PERIOD (800000 - 1)

void debounce_buttons(void);
void wait_for_interrupts(void);
void button_Handler(void);
void debounce_Button(void);

volatile unsigned long count = 0; // Every 12.5ns*(period); for timer-interrupt
volatile unsigned long timeCount = 0; // Every 1 ms; for debouncing

volatile unsigned long ledState = 0;
volatile unsigned long lastDebounceTime = 0;

_Bool areButtonsToggledOff = 1;

void portB_Init(void) {
        // Setting Clock to 40MHz
       SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

       // Enabling Port B
       SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

       // Setting LED pins to Output
       GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, LED_PINS);

       // Set SW1 GPIO as inputs
       GPIODirModeSet(GPIO_PORTB_BASE, BTN_PINS , GPIO_DIR_MODE_IN);

       // Register, configure and enable the Button Interrupt handler
       GPIOIntRegister(GPIO_PORTB_BASE, button_Handler);
       GPIOIntTypeSet(GPIO_PORTB_BASE, BTN_PINS, GPIO_FALLING_EDGE);
       GPIOIntEnable(GPIO_PORTB_BASE,  BTN_PINS);

       // Enable internal pull up resistors
       GPIOPadConfigSet(GPIO_PORTB_BASE, BTN_PINS , GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
       GPIOPadConfigSet(GPIO_PORTB_BASE, LED_PINS , GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
}

void timer_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_ONE_SHOT);

    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();

    TimerEnable(TIMER0_BASE, TIMER_A);
}

void setup(void) {
    portB_Init();
    timer_Init();
}

int main(void) {
  setup();

  while(1){                   // interrupts every 1ms
      GPIOPinWrite(GPIO_PORTB_BASE, LED_PINS, ledState);
      wait_for_interrupts();
  }
}

void on_Button_Pushed() {
    ledState ^= LED_ON;
    GPIOPinWrite(GPIO_PORTB_BASE, LED_PINS, ledState);
}

/* Enter low-power mode while waiting for interrupts */
void wait_for_interrupts(void) {
    __asm("    WFI\n"
          "    BX     LR");
}

/* Interrupt GPIO_PORTB when button pressed */
void button_Handler(void){
    // Clear interrupt flag
    GPIOIntClear(GPIO_PORTB_BASE, BTN_PINS);

    // Check that portB isn't in default state ie. button is pushed.
    int buttonValue = GPIOPinRead(GPIO_PORTB_BASE, BTN_PINS);
    _Bool wasAButtonPressed = (buttonValue != BUTTONS_NOT_PRESSED);
    if(wasAButtonPressed) {

        // Reload and enable the debounce timer
        TimerLoadSet(TIMER0_BASE, TIMER_A, DEBOUNCE_PERIOD);
        TimerEnable(TIMER0_BASE, TIMER_A);
    }
}

/* Interrupt for handling button debounce */
void debounce_Handler(void) {
    // Clear Timer Interrupt Flag
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // Check if the button is still pressed
    _Bool wasAButtonPressed = (GPIOPinRead(GPIO_PORTB_BASE, BTN_PINS) != BUTTONS_NOT_PRESSED);
    if(wasAButtonPressed) {
        on_Button_Pushed();
    }
}

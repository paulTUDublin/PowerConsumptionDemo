/* Demonstrates the use of an interrupt on a GPIO port
GPIO interrupts are routed through the EXTI (Extended 
Inerrupts and Event Controler ).  This makes it a little
more complicated that an interrupt which is managed directly by
the NVIC (Nested Vector Interrupt Controller) 
This example uses a button on PB4 to generate an interrupt.  
When the button is pressed, PB4 is pulled low.
PB4 is routed through the EXTI4 interrupt */
#include "eeng1030_lib.h"
void setup(void);
void delay(volatile uint32_t dly);
int main()
{
    setup();
    
    // Un comment for low power modes e.g. LPRun mode
    // The same function can be used for LPSleep as this only sets LPR  
    // enterLPRun();           // Enter LPRun

    while(1)
    {        
        // Uncomment during Run and LPRun demonstrations 
        // GPIOB->ODR &= ~(1 << 3);
        // GPIOA->ODR ^= (1 << 0);
        // delay(1000000);
        
        // Uncomment for Sleep and LPSleep
        // GPIOB->ODR &= ~(1 << 3);
        // GPIOA->ODR ^= (1 << 0);
        // __WFI();
        // GPIOA->ODR ^= (1 << 0);
        // delay(1000000);

        // Uncomment for Stop Mode 0
        GPIOB->ODR &= ~(1 << 3);
        GPIOA->ODR ^= (1 << 0);
        enterStopMode0(); 
        GPIOA->ODR ^= (1 << 0);
        delay(100000);

            
    }
}
void setup()
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 1);    // enable GPIOA and GPIOB
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;    // enable PWR
    
    // Initialise clock source
    initClocks();                           // Set PLL clockL: 80 MHz clock
    // initMSIClock(5);                        // Set MSI clock: 2 MHz

    // Configure GPIO
    pinMode(GPIOB,3,1);
    pinMode(GPIOB,4,0);
    enablePullUp(GPIOB,4);
    pinMode(GPIOA,0,1);
    
    // Configure external interrupts
    RCC->APB2ENR |= (1 << 0);           // enable SYSCFG
    SYSCFG->EXTICR[1] &= ~(7 << 0);     // clear perhaps previously set bits
    SYSCFG->EXTICR[1] |= (1 << 0);      // map EXTI2 interrupt to PB4
    EXTI->FTSR1 |= (1 << 4);            // select falling edge trigger for PB4 input
    EXTI->IMR1 |= (1 << 4);             // enable PB4 interrupt
    NVIC->ISER[0] |= (1 << 10);         // IRQ 10 maps to EXTI4
    __enable_irq();

    // Enable MCO which is AF0 on  PA8
    pinMode(GPIOA,8,2);                     // Alternate function mode         
    selectAlternateFunction(GPIOA,8,0);     // AF0 for MCO on PA8

    // Reference manual recommends setting prescaler before enabling MCO output
    RCC->CFGR &= ~(0xf << 24);              // Clear previous bits and diable MCO
    RCC->CFGR &= ~(RCC_CFGR_MCOPRE);        // Clear prescaler bits
    RCC->CFGR |= (0b000 << 28);             // Set MCO prescaler (1: 000,2,4,8,16: 100)
    RCC->CFGR |= (1 << 24);                 // Select SYSCLK as MCO output
    RCC->CFGR &= ~(1 << 15);                // Set wakeup clok to be MSI
    
}
void delay(volatile uint32_t dly)
{
    while (dly--);
}
void EXTI4_IRQHandler()
{
    GPIOB->BSRR = (1 << 3); // set PB3 to turn on LED
    EXTI->PR1 = (1 << 4);   // clear interrupt pending flag
}
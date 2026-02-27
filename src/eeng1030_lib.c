#include <stm32l432xx.h>
#include <stdint.h>
void initClocks()
{
	// Initialize the clock system to a higher speed.
	// At boot time, the clock is derived from the MSI clock 
	// which defaults to 4MHz.  Will set it to 80MHz
	// See chapter 6 of the reference manual (RM0393)
	    RCC->CR &= ~(1 << 24); // Make sure PLL is off
	
	// PLL Input clock = MSI so BIT1 = 1, BIT 0 = 0
	// PLLM = Divisor for input clock : set = 1 so BIT6,5,4 = 0
	// PLL-VCO speed = PLL_N x PLL Input clock
	// This must be < 344MHz
	// PLL Input clock = 4MHz from MSI
	// PLL_N can range from 8 to 86.  
	// Will use 80 for PLL_N as 80 * 4 = 320MHz
	// Put value 80 into bits 14:8 (being sure to clear bits as necessary)
	// PLLSAI3 : Serial audio interface : not using leave BIT16 = 0
	// PLLP : Must pick a value that divides 320MHz down to <= 80MHz
	// If BIT17 = 1 then divisor is 17; 320/17 = 18.82MHz : ok (PLLP used by SAI)
	// PLLQEN : Don't need this so set BIT20 = 0
	// PLLQ : Must divide 320 down to value <=80MHz.  
	// Set BIT22,21 to 1 to get a divisor of 8 : ok
	// PLLREN : This enables the PLLCLK output of the PLL
	// I think we need this so set to 1. BIT24 = 1 
	// PLLR : Pick a value that divides 320 down to <= 80MHz
	// Choose 4 to give an 80MHz output.  
	// BIT26 = 0; BIT25 = 1
	// All other bits reserved and zero at reset
	    RCC->PLLCFGR = (1 << 25) + (1 << 24) + (1 << 22) + (1 << 21) + (1 << 17) + (80 << 8) + (1 << 0);	
	    RCC->CR |= (1 << 24); // Turn PLL on
	    while( (RCC->CR & (1 << 25))== 0); // Wait for PLL to be ready
	// configure flash for 4 wait states (required at 80MHz)
	    FLASH->ACR &= ~((1 << 2)+ (1 << 1) + (1 << 0));
	    FLASH->ACR |= (1 << 2); 
	    RCC->CFGR |= (1 << 1)+(1 << 0); // Select PLL as system clock
}
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber)
{
	Port->PUPDR = Port->PUPDR &~(3u << BitNumber*2); // clear pull-up resistor bits
	Port->PUPDR = Port->PUPDR | (1u << BitNumber*2); // set pull-up bit
}
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
	/*
        Modes : 00 = input
                01 = output
                10 = special function
                11 = analog mode
	*/
	uint32_t mode_value = Port->MODER;
	Mode = Mode << (2 * BitNumber);
	mode_value = mode_value & ~(3u << (BitNumber * 2));
	mode_value = mode_value | Mode;
	Port->MODER = mode_value;
}
void selectAlternateFunction (GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t AF)
{
    // The alternative function control is spread across two 32 bit registers AFR[0] and AFR[1]
    // There are 4 bits for each port bit.
    if (BitNumber < 8)
    {
        Port->AFR[0] &= ~(0x0f << (4*BitNumber));
        Port->AFR[0] |= (AF << (4*BitNumber));
    }
    else
    {
        BitNumber = BitNumber - 8;
        Port->AFR[1] &= ~(0x0f << (4*BitNumber));
        Port->AFR[1] |= (AF << (4*BitNumber));
    }
}
void initMSIClock(uint32_t MSIClockRange)
{

    // The purpose of this function is to configure the multi-speed internal (MSI) clock and configure its as the SysClock

    // Enable internal oscillator for MSI
    RCC->CR |= (1 << 0);            // Enable MSI (multi-speed internal oscillator)
    while(!(RCC->CR & (1 << 1)));   // Wait till MSI ready bit is set
    
    // Configure MSI clock to 1 MHz
    // MSIRANGE values: 0:100 kHz, 1:200 kHz, 2:400 kHz, 3:800 kHz, 
    //                  4:1 MHz, 5:2 MHz, 6:4 MHz (reset value), 7:8 MHz,
    //                  8:16 MHz, 9:32 MHz, 10:32 MHz, 11:48 MHz, 
    RCC->CR |= (1 << 3);                // Allows MSI clock range to set by MSIRANGe bits in CR register
    RCC->CR &= ~(0b1111 << 4);          // Clear the MSIRANGE bits [7:4]
    RCC->CR |= (MSIClockRange << 4);    // Set MSIRANGE to mode MSIClockRange

    // Select MSI as system clock
    RCC->CFGR &= ~(0b11 << 0);      // Clear system clock switch bits
    RCC->CFGR |= (0b00 << 0);       // Set system switch bits (SW bits, 00: MSI as system clock)

    while( (RCC->CFGR & (0b11 << 2)) != 0); // Wait until MSI is set as system clock
    
    // Update system clock varibale
    SystemCoreClockUpdate();
    
     
}
void enterLPRun(void)
{
    // Section 5.3 of the reference manual (RM0394) outlines the 7 low-power modes
    // Optional step is to power down FLASH->ACR RUN_PD bit
    // Optional: Set Bit 13 of FLASH_ACR (write protected see ref manual 3.7.1)
    // FLASH->PDKEYR = 0x04152637;
    // FLASH->PDKEYR = 0xFAFBFCFD; 
    // FLASH->ACR |= (1 << 13); 

	// Select voltage scale 2 (low-power run compatible)
	PWR->CR1 &= ~PWR_CR1_VOS;
	PWR->CR1 = PWR->CR1 + (1 << 9);   // VOS = Range 2, need to set this correctly 
	
	// // Wait until voltage scaling ready
	while (PWR->SR2 & PWR_SR2_VOSF);
	
    // Force regulator into low-power mode LPR, system clock to be <= 2 MHz
    PWR->CR1 |= (1 << 14);
	while((PWR->SR2 & (1 << 9)) == 0);     // Wait until REGLPF = 1 i.e. regulator is in main mode 
}
void exitLPRun(void)
{
    // Section 5.3 of the reference manual (RM0394) outlines the 7 low-power modes
    PWR->CR1 &= ~(1 << 14);         // Force regulator into main mode LPR
    while(PWR->SR2 & (1 << 9));     // Wait until REGLPF = 0 i.e. regulator is in main mode
    
}
void enterStopMode0()
{
	PWR->CR1 &= ~(0b111 << 0); 	// Clear LPMS bits
    PWR->CR1 |= (0b000 << 0);	// Set LPMS bits for Stop Mode 0
	SCB->SCR |= (1 << 2);		// Set SLEEPDEEP bit in System Control Block in the core
	__WFI();
	SCB->SCR &= ~(1 << 2);		// Clear SLEEPDEEP bit in System Control Block in the core
}
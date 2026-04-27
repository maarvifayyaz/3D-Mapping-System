#include <stdint.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"

#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"

#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable

#define MAXRETRIES              5           // number of receive attempts before giving up
void PortN_Init_Project(void);
void PortF_Init_Project(void);

void SetAdditionalStatus_On(void);
void SetAdditionalStatus_Off(void);

void SetMeasurement_On(void);
void SetMeasurement_Off(void);

void PulseUARTTx(void);

void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           // activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;           // activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){}; // ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           // 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             // 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             // 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;           // 7) disable analog functionality on PB2,3

                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                       // 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       // 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
//    I2C0_MTPR_R = 0x3B;                                         // 8) configure for 100 kbps clock
       
}

//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}

//XSHUT     This pin is an active-low shutdown input;
// the board pulls it up to VDD to enable the sensor by default.
// Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    //FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
   
}

//int numSteps = 256; // 45 degress per ToF capture

void PortL_Init(void){   // STEPPER MOTOR
//Use PortL pins (PL0-PL3) for output
SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R10; // activate clock for Port L
while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R10) == 0){}; // allow time for clock to stabilize
GPIO_PORTL_DIR_R |= 0x0F;         // configure Port L pins (PL0-PL3) as output
  GPIO_PORTL_AFSEL_R &= ~0x0F;     // disable alt funct on Port L pins (PL0-PL3)
  GPIO_PORTL_DEN_R |= 0x0F;         // enable digital I/O on Port L pins (PL0-PL3)
// configure Port L as GPIO
  GPIO_PORTL_AMSEL_R &= ~0x0F;     // disable analog functionality on Port L pins (PL0-PL3)
return;
}

void PortJ_Init(void){   // PJ0 = onboard pushbutton
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8;              // activate clock for Port J
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R8) == 0){};   // wait until Port J is ready

    GPIO_PORTJ_DIR_R &= ~0x01;      // make PJ0 input
    GPIO_PORTJ_AFSEL_R &= ~0x01;    // disable alternate function on PJ0
    GPIO_PORTJ_DEN_R |= 0x01;       // enable digital I/O on PJ0
    GPIO_PORTJ_PUR_R |= 0x01;       // enable weak pull-up resistor on PJ0
    GPIO_PORTJ_AMSEL_R &= ~0x01;    // disable analog functionality on PJ0
}

void WaitForButtonPress(void){
    while((GPIO_PORTJ_DATA_R & 0x01) != 0){
    }

    SysTick_Wait10ms(2);   // debounce

    while((GPIO_PORTJ_DATA_R & 0x01) == 0){
    }

    SysTick_Wait10ms(2);   // debounce
}

void PortN_Init_Project(void){   // PN0 and PN1
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R12) == 0){}

    GPIO_PORTN_DIR_R |= 0x03;       // PN0, PN1 output
    GPIO_PORTN_AFSEL_R &= ~0x03;
    GPIO_PORTN_DEN_R |= 0x03;
    GPIO_PORTN_AMSEL_R &= ~0x03;

    GPIO_PORTN_DATA_R &= ~0x03;     // both off initially
}

void PortF_Init_Project(void){   // PF4
                  
            SYSCTL_RCGCGPIO_R |= 0x20;                 // 1) enable clock for Port F
    while((SYSCTL_PRGPIO_R & 0x20)==0){};     // wait until ready

    GPIO_PORTF_LOCK_R = 0x4C4F434B;           // 2) unlock PF0 (needed)
    GPIO_PORTF_CR_R |= 0x11;                  // allow changes to PF0 and PF4

    GPIO_PORTF_DIR_R |= 0x11;                 // 3) PF0 and PF4 = outputs
    GPIO_PORTF_DEN_R |= 0x11;                 // 4) digital enable PF0 and PF4
                  
            GPIO_PORTF_DATA_R &= ~0x11;     // PF4 off initially
}

void SetAdditionalStatus_On(void){   // PF4
    GPIO_PORTF_DATA_R |= 0x10;
}

void SetAdditionalStatus_Off(void){
    GPIO_PORTF_DATA_R &= ~0x10;
}

void SetMeasurement_On(void){        // PN0
    GPIO_PORTN_DATA_R |= 0x01;
}

void SetMeasurement_Off(void){
    GPIO_PORTN_DATA_R &= ~0x01;
}

void PulseUARTTx(void){              // PN1
    GPIO_PORTN_DATA_R |= 0x02;
    SysTick_Wait10ms(1);
    GPIO_PORTN_DATA_R &= ~0x02;
}




//*********************************************************************************************************
//*********************************************************************************************************
//*********** MAIN Function *****************************************************************
//*********************************************************************************************************
//*********************************************************************************************************
uint16_t dev = 0x29; //address of the ToF sensor as an I2C slave peripheral
int status=0;

int main(void) {
  uint8_t byteData, sensorState=0, myByteArray[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} , i=0;
  uint16_t wordData;
  uint16_t Distance;
  uint16_t SignalRate;
  uint16_t AmbientRate;
  uint16_t SpadNum;
  uint8_t RangeStatus;
  uint8_t dataReady = 0;

//initialize
PortN_Init_Project();
PortF_Init_Project();
PLL_Init();
SysTick_Init();
onboardLEDs_Init();
I2C_Init();
UART_Init();
PortL_Init(); // NEW
PortJ_Init();// button press

// hello world!
UART_printf("Program Begins\r\n");
int mynumber = 1;
sprintf(printf_buffer,"2DX ToF Program Studio Code %d\r\n",mynumber);
UART_printf(printf_buffer);
PulseUARTTx();


/* Those basic I2C read functions can be used to check your own I2C functions */
status = VL53L1X_GetSensorId(dev, &wordData);

sprintf(printf_buffer,"(Model_ID, Module_Type)=0x%x\r\n",wordData);
UART_printf(printf_buffer);
PulseUARTTx();

// 1 Wait for device booted
while(sensorState==0){
status = VL53L1X_BootState(dev, &sensorState);
SysTick_Wait10ms(10);
  }
//FlashAllLEDs();
UART_printf("ToF Chip Booted!\r\n Please Wait...\r\n");
PulseUARTTx();   // PN1 pulse

status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/

  /* 2 Initialize the sensor with the default setting  */
  status = VL53L1X_SensorInit(dev);
Status_Check("SensorInit", status);

SetAdditionalStatus_On();   // PN0 = waiting

UART_printf("Press PJ0 to start scan...\r\n"); // added new
PulseUARTTx();   // PN1 pulse
WaitForButtonPress();

SetAdditionalStatus_Off();  // leave waiting state

UART_printf("Button pressed. Starting scan...\r\n");
PulseUARTTx();   // PN1 pulse


  /* 3 Optional functions to be used to change the main ranging parameters according the application requirements to get the best ranging performances */
//  status = VL53L1X_SetDistanceMode(dev, 2); /* 1=short, 2=long */
//  status = VL53L1X_SetTimingBudgetInMs(dev, 100); /* in ms possible values [20, 50, 100, 200, 500] */
//  status = VL53L1X_SetInterMeasurementInMs(dev, 200); /* in ms, IM must be > = TB */


    // 4 What is missing -- refer to API flow chart
  status = VL53L1X_StartRanging(dev);

UART_printf("\r\nCW Measurements\r\n");
PulseUARTTx();

// ----- PF0 BUS SPEED TEST -----
for(int k = 0; k < 10; k++){
    GPIO_PORTF_DATA_R ^= 0x01;   // PF0
    SysTick_Wait10ms(50);
}

for(int i = 0; i < 32; i++) {

    dataReady = 0;

    // wait until ToF data is ready
    while(dataReady == 0){
        status = VL53L1X_CheckForDataReady(dev, &dataReady);
        VL53L1_WaitMs(dev, 5);
    }

    dataReady = 0;

    // rotate motor 45 degrees
    for(int j = 0; j < 16; j++){
        GPIO_PORTL_DATA_R = 0b00000011;
        SysTick_Wait2ms(1);

        GPIO_PORTL_DATA_R = 0b00000110;
        SysTick_Wait2ms(1);

        GPIO_PORTL_DATA_R = 0b00001100;
        SysTick_Wait2ms(1);

        GPIO_PORTL_DATA_R = 0b00001001;
        SysTick_Wait2ms(1);
    }

    SysTick_Wait10ms(20);   // let motor settle

    // read ToF data
    SetMeasurement_On();

    status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
    status = VL53L1X_GetDistance(dev, &Distance);
    status = VL53L1X_GetSignalRate(dev, &SignalRate);
    status = VL53L1X_GetAmbientRate(dev, &AmbientRate);
    status = VL53L1X_GetSpadNb(dev, &SpadNum);

    SetMeasurement_Off();

    status = VL53L1X_ClearInterrupt(dev);

    // send data over UART
    sprintf(printf_buffer, "%d,%u,%u,%u,%u,%u\r\n",
            i + 1, Distance, RangeStatus, SignalRate, AmbientRate, SpadNum);
    UART_printf(printf_buffer);
    PulseUARTTx();

    SysTick_Wait10ms(50);
}

// return motor to home after full scan
SysTick_Wait10ms(20);

for(int j = 0; j < 512; j++){
    GPIO_PORTL_DATA_R = 0b00000011;
    SysTick_Wait2ms(1);

    GPIO_PORTL_DATA_R = 0b00001001;
    SysTick_Wait2ms(1);

    GPIO_PORTL_DATA_R = 0b00001100;
    SysTick_Wait2ms(1);

    GPIO_PORTL_DATA_R = 0b00000110;
    SysTick_Wait2ms(1);
}

SysTick_Wait10ms(20);

VL53L1X_StopRanging(dev);

UART_printf("Scan complete.\r\n");
PulseUARTTx();

SetAdditionalStatus_On();   // PF4 back ON = done/idle

while(1) {}

}
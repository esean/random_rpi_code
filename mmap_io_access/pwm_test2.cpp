#include "easy_pio.h"

int main(int argc,char* argv[]) {

#if 1
    pioInit();
    pwmInit();
    printf("SETPWM\n");
    setPWM(40.0,50.0);
    printf("sleep\n");
    usleep(1000000);
    printf("stop\n");
    pwmStop();

#else

    PWMCTL=0; // Turn off PWM.

    CM_PWMCTL=(CM_PWMCTL&~0x10)|0x5a000000; // Turn off enable flag.
    while(CM_PWMCTL&0x80); // Wait for busy flag to turn off.
    CM_PWMDIV=0x5a000000|(5<<12); // Configure divider.
    CM_PWMCTL=0x5a000206; // Source=PLLD (500 MHz), 1-stage MASH.
    CM_PWMCTL=0x5a000216; // Enable clock.
    while(!(CM_PWMCTL&0x80)); // Wait for busy flag to turn on.

    PWMRNG1=100;
    PWMDAT1=10;

    PWMCTL=0x0081; // Channel 1 M/S mode, no FIFO, PWM mode, enabled.
#endif
    return 0;
}

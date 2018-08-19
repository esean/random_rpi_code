#include <cstdint>
#include <cstdio>

#if 1
#define PWMCTL (*(volatile uint32_t *)0x2020c000)
#define PWMSTA (*(volatile uint32_t *)0x2020c004)
#define PWMDMAC (*(volatile uint32_t *)0x2020c008)
#define PWMRNG1 (*(volatile uint32_t *)0x2020c010)
#define PWMDAT1 (*(volatile uint32_t *)0x2020c014)
#define PWMFIF1 (*(volatile uint32_t *)0x2020c018)
#define PWMRNG2 (*(volatile uint32_t *)0x2020c020)
#define PWMDAT2 (*(volatile uint32_t *)0x2020c024)

#define CM_PWMCTL (*(volatile uint32_t *)0x201010a0)
#define CM_PWMDIV (*(volatile uint32_t *)0x201010a4)
#endif

int main(int argc,char* argv[]) {
	printf("1");
    PWMCTL=0; // Turn off PWM.
#if 0
printf("1");

    CM_PWMCTL=(CM_PWMCTL&~0x10)|0x5a000000; // Turn off enable flag.
	printf("1");
    while(CM_PWMCTL&0x80); // Wait for busy flag to turn off.
	printf("1");
    CM_PWMDIV=0x5a000000|(5<<12); // Configure divider.
	printf("1");
    CM_PWMCTL=0x5a000206; // Source=PLLD (500 MHz), 1-stage MASH.
	printf("1");
    CM_PWMCTL=0x5a000216; // Enable clock.
	printf("1");
    while(!(CM_PWMCTL&0x80)); // Wait for busy flag to turn on.
	printf("1");

    PWMRNG1=100;
	printf("1");
    PWMDAT1=10;
	printf("1");

    PWMCTL=0x0081; // Channel 1 M/S mode, no FIFO, PWM mode, enabled.
	printf("1");
#endif
    return 0;
}

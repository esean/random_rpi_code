/*
Compile and link

gcc -o nanopulse nanopulse.c

Run example (10000 10 nanosecond pulses with 2000 nano second gap)

sudo ./nanopulse 10 10000 2000
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>


#define CLK_BASE  0x20101000
#define GPIO_BASE 0x20200000
#define PWM_BASE  0x2020C000

#define CLK_LEN   0xA8
#define GPIO_LEN  0xB4
#define PWM_LEN   0x28

#define PWM_CTL      0
#define PWM_STA      1
#define PWM_RNG1     4
#define PWM_FIFO     6

#define PWM_CTL_CLRF1 (1<<6)
#define PWM_CTL_USEF1 (1<<5)
#define PWM_CTL_MODE1 (1<<1)
#define PWM_CTL_PWEN1 (1<<0)

#define PWM_STA_EMPT1 (1<<1)

#define CLK_PASSWD  (0x5A<<24)

#define CLK_CTL_MASH(x)((x)<<9)
#define CLK_CTL_BUSY    (1 <<7)
#define CLK_CTL_KILL    (1 <<5)
#define CLK_CTL_ENAB    (1 <<4)
#define CLK_CTL_SRC(x) ((x)<<0)

#define CLK_CTL_SRC_PLLD 6  /* 500.0 MHz */

#define CLK_DIV_DIVI(x) ((x)<<12)
#define CLK_DIV_DIVF(x) ((x)<< 0)

#define CLK_PWMCTL 40
#define CLK_PWMDIV 41

#define MAX_BITS 224

typedef struct
{
   unsigned divider;
   unsigned bits;
} pwm_clock_cfg_t;

unsigned base_nano[]={4, 8, 10, 20, 40, 80, 100, 200, 250, 500, 1000};

static volatile uint32_t  *clkReg  = MAP_FAILED;
static volatile uint32_t  *gpioReg = MAP_FAILED;
static volatile uint32_t  *pwmReg  = MAP_FAILED;

static void mynanosleep(unsigned nanos)
{
   struct timespec ts, tr;

   ts.tv_sec = 0;
   ts.tv_nsec = nanos;
   while (nanosleep(&ts, &tr))
   {
      ts = tr;
   }
}

int gpioSetMode(unsigned gpio, unsigned mode)
{
   int reg, shift;

   reg   =  gpio/10;
   shift = (gpio%10) * 3;

   gpioReg[reg] = (gpioReg[reg] & ~(7<<shift)) | (mode<<shift);

   return 0;
}

int gpioGetMode(unsigned gpio)
{
   int reg, shift;

   reg   =  gpio/10;
   shift = (gpio%10) * 3;

   return (*(gpioReg + reg) >> shift) & 7;
}

static void initPWM(unsigned divider)
{
   /* reset PWM clock */
   clkReg[CLK_PWMCTL] = CLK_PASSWD | CLK_CTL_KILL;

   mynanosleep(10000);

   /* set PWM clock source as 500 MHz PLLD */
   clkReg[CLK_PWMCTL] = CLK_PASSWD | CLK_CTL_SRC(CLK_CTL_SRC_PLLD);

   mynanosleep(10000);

   /* set PWM clock divider */
   clkReg[CLK_PWMDIV] = CLK_PASSWD | CLK_DIV_DIVI(divider) | CLK_DIV_DIVF(0);

   mynanosleep(10000);

   /* enable PWM clock */
   clkReg[CLK_PWMCTL] =
      CLK_PASSWD | CLK_CTL_ENAB | CLK_CTL_SRC(CLK_CTL_SRC_PLLD);

   mynanosleep(100000);

   /* reset PWM */
   pwmReg[PWM_CTL] = 0;

   /* clear PWM status bits */
   pwmReg[PWM_STA] = -1;

   mynanosleep(10000);
}

static void sendPulse(unsigned bits)
{
   int i;
   uint32_t word;

   if      (bits == 0)       bits = 1;
   else if (bits > MAX_BITS) bits = MAX_BITS;

   /* clear PWM fifo */

   pwmReg[PWM_CTL] = PWM_CTL_CLRF1;

   mynanosleep(10000);

   while (bits >= 32)
   {
      pwmReg[PWM_FIFO] = -1;
      bits -= 32;
   }

   if (bits)
   {
      word = 0;

      for (i=0; i<bits; i++) word |= (1<<(31-i));

      pwmReg[PWM_FIFO] = word;
   }

   pwmReg[PWM_FIFO] = 0;

   /* enable PWM for serialised data from fifo */
   pwmReg[PWM_CTL] = PWM_CTL_USEF1 | PWM_CTL_MODE1 | PWM_CTL_PWEN1;
}


static uint32_t * mapMem(int fd, unsigned base, unsigned len)
{
   return mmap
   (
      0,
      len,
      PROT_READ|PROT_WRITE|PROT_EXEC,
      MAP_SHARED|MAP_LOCKED,
      fd,
      base
   );
}

pwm_clock_cfg_t getDivBits(unsigned nano)
{
   pwm_clock_cfg_t cfg;

   unsigned i, base, bits, err, bestErr, bestBase, bestBits;

   bestErr = -1;

   for (i=0; i<sizeof(base_nano)/sizeof(unsigned);i++)
   {
      bits = nano / base_nano[i];

      if (bits > MAX_BITS) bits = MAX_BITS;

      err = nano - (bits * base_nano[i]);

      if (err < bestErr)
      {
         bestErr = err;
         bestBase = base_nano[i];
         bestBits = bits;
      }
   }

   cfg.divider = bestBase / 2;
   cfg.bits = bestBits;

   return cfg;
}

int main(int argc, char *argv[])
{
   int fd, i, gpio, mode;
   pwm_clock_cfg_t cfg;

   int nanos=1000, pulses=100, gap=5000;

   fd = open("/dev/mem", O_RDWR | O_SYNC);

   if (fd<0)
   {
      printf("need to run as root, e.g. sudo %s\n", argv[0]);
      exit(1);
   }

   gpioReg = mapMem(fd, GPIO_BASE, GPIO_LEN);
   pwmReg  = mapMem(fd, PWM_BASE,  PWM_LEN);
   clkReg  = mapMem(fd, CLK_BASE,  CLK_LEN);

   close(fd);

   if (argc > 1) nanos  = atoi(argv[1]);
   if (argc > 2) pulses = atoi(argv[2]);
   if (argc > 3) gap    = atoi(argv[3]);

   if      (nanos < 4)      nanos = 4;
   else if (nanos > 224000) nanos = 224000;

   if (pulses < 1) pulses = 1;

   if (gap < 0) gap = 0;

   cfg = getDivBits(nanos);

   printf("%d pulses of %d nanos with gap of %d nanos (div=%d bits=%d)\n",
      pulses, cfg.divider * 2 * cfg.bits, gap, cfg.divider, cfg.bits);

   mode = gpioGetMode(18); /* save original mode */

   gpioSetMode(18, 2); /* set to ALT5, PWM1 */

   initPWM(cfg.divider);

   for (i=0; i< pulses; i++)
   {
      sendPulse(cfg.bits);

      mynanosleep(nanos + gap);
   }

   gpioSetMode(18, mode); /* restore original mode */
}


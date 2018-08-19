#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

double freq = 50.0;
double duty = 0.50;

int main (int argc, char* argv[])
{
  printf("USAGE: [freq(50)] [duty(0.5)]\n");
  if (argc >= 3) {
    freq = atof(argv[1]);
    duty = atof(argv[2]);
  }
  if (wiringPiSetupGpio() == -1)
    exit (1) ;

  pinMode(18,PWM_OUTPUT);
  pwmSetMode(PWM_MODE_MS);

  double range = freq/duty;
  double clock = (19.2E+6 / freq) / range;
  if (range>4095) range=4095;
  if (clock>4095) clock=4095;
  printf("Freq=%d Duty=%d %%, CLK=%d RANGE=%d -> freq=%d\n",(int)freq,(int)(duty*100.0),(int)clock,(int)range,(int)((19.2E+6/(int)clock)/(int)range));

#if 1
  pwmSetClock((int)clock);
  pwmSetRange ((int)range);
  pwmWrite (18, (int)(duty*100.0));
#else
  pwmSetClock(1920);
  pwmSetRange (200);
  pwmWrite (18, 50);
#endif

//  for (;;) delay (1000) ;
}

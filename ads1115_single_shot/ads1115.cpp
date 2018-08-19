
#include <stdio.h>
#include <sys/types.h> // open
#include <sys/stat.h>  // open
#include <sys/ioctl.h>
#include <fcntl.h>     // open
#include <unistd.h>    // read/write usleep
#include <stdlib.h>    // exit
#include <inttypes.h>  // uint8_t, etc
#include <linux/i2c-dev.h> // I2C bus definitions
#include <iostream>
#include <cstdlib>
#include "ads1115.h"

// todo: support continuous mode - should be faster to set mode=cont and then do one-shot reads

ads1115::ads1115(int i2c_addr)
    : fd(-1), addr(i2c_addr) {
} 

// returns -1 on failure, 0 success
int ads1115::dev_open() {
  // open device on /dev/i2c-1 
  // the default on Raspberry Pi B
  if ((fd = open("/dev/i2c-1", O_RDWR)) < 0) {
    printf("Error:ads1115::dev_open() Couldn't open I2C bus! %d\n", fd);
    return -1;
  }

  // connect to ads1115 as i2c slave
  if (ioctl(fd, I2C_SLAVE, addr) < 0) {
    printf("Error:ads1115::dev_open() Couldn't find I2C device on address 0x%X!\n",addr);
    return -1;
  }
  return 0;
}

// returns -1 on failure, 0 success
int ads1115::dev_single_shot(double* meas, ads1115::INPUT_MUX input_mux, ads1115::PGA input_pga) {
  int16_t val;
  uint8_t writeBuf[3];
  uint8_t readBuf[2];
  double myfloat;
  double div;
  switch (input_pga) {
        case ads1115::PGA::PGA0_6144: div = 6.144; break;
        case ads1115::PGA::PGA1_4096: div = 4.096; break;
        case ads1115::PGA::PGA2_2048: div = 2.048; break;
        case ads1115::PGA::PGA3_1024: div = 1.024; break;
        case ads1115::PGA::PGA4_0512: div = 0.512; break;
        case ads1115::PGA::PGA5_0256:
        case ads1115::PGA::PGA6_0256:
        case ads1115::PGA::PGA7_0256: div = 6.144; break;
  }
  const double VPS = div / 32768.0; //volts per step

  // set config register and start conversion
  // ANC0 and GND, 4.096v, 128s/s
  writeBuf[0] = 1;    // config register is 1
  // (1<<7) = 'OS' - start a single conversion
  // (1<<0) = 'MODE' - single-shot mode or power-down (default)
  writeBuf[1] = (1<<7) | ((input_mux&7)<<4) | ((input_pga&7)<<1) | (1<<0);
  //writeBuf[1] = 0b11000011; // bit 15-8 0xD3
  // bit 15 flag bit for single shot
  // Bits 14-12 input selection:
  // 100 ANC0; 101 ANC1; 110 ANC2; 111 ANC3
  // Bits 11-9 Amp gain. Default to 010 here 001 P19
  // Bit 8 Operational mode of the ADS1115.
  // 0 : Continuous conversion mode
  // 1 : Power-down single-shot mode (default)

  writeBuf[2] = 0b10000101; // bits 7-0  0x85
  // Bits 7-5 data rate default to 100 for 128SPS
  // Bits 4-0  comparator functions see spec sheet.

  // begin conversion
  if (write(fd, writeBuf, 3) != 3) {
    perror("Write to register 1");
    return -1;
  }

  // wait for conversion complete
  // checking bit 15
  do {
    if (read(fd, writeBuf, 2) != 2) {
      perror("Read conversion");
      return -1;
    }
  }
  while ((writeBuf[0] & 0x80) == 0);

  // read conversion register
  // write register pointer first
  readBuf[0] = 0;   // conversion register is 0
  if (write(fd, readBuf, 1) != 1) {
    perror("Write register select");
    return -1;
  }

  // read 2 bytes
  if (read(fd, readBuf, 2) != 2) {
    perror("Read conversion");
    return -1;
  }

  // convert display results
  val = (readBuf[0] << 8)&0xFF00 | (readBuf[1]&0x00FF);
  if (val < 0)   val = 0;
  myfloat = val * VPS; // convert to voltage
  if (meas) *meas = myfloat;

  return 0;
}
ads1115::~ads1115() {
    if (fd>0) close(fd);
}

//------------------------------------------------

int main(int argc, char* argv[]) {
    printf("USAGE: %s {[MUX] [PGA]}\n",argv[0]);

    ads1115::INPUT_MUX in_mux = ads1115::INPUT_MUX::P_AIN0_N_GND;
    ads1115::PGA in_pga = ads1115::PGA::PGA2_2048;
    if (argc>1) in_mux = static_cast<ads1115::INPUT_MUX>(atoi(argv[1]));
    if (argc>2) in_pga = static_cast<ads1115::PGA>(atoi(argv[2]));

    ads1115 t(ads1115::ADS1115_I2C_ADDR);
    if (t.dev_open() < 0) {
        printf("ERROR: open\n");
        exit(1);
    }
    double meas;
    //while (1) {
    if (t.dev_single_shot(&meas,in_mux,in_pga) < 0) {
        printf("ERROR: read\n");
        exit(1);
    }
    printf("ads1115 meas = %f\n",meas);
    //}

    exit(0);
}

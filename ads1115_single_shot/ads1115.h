#ifndef ads1115_hpp
#define ads1115_hpp

class ads1115 {

public:
    static const int ADS1115_I2C_ADDR = 0x48;

    enum INPUT_MUX {
        P_AIN0_N_AIN1 = 0,  // default
        P_AIN0_N_AIN3 = 1,
        P_AIN1_N_AIN3 = 2,
        P_AIN2_N_AIN3 = 3,
        P_AIN0_N_GND  = 4,
        P_AIN1_N_GND  = 5,
        P_AIN2_N_GND  = 6,
        P_AIN3_N_GND  = 7,
    };

    enum PGA {
        PGA0_6144 = 0,
        PGA1_4096 = 1,
        PGA2_2048 = 2,  // default
        PGA3_1024 = 3,
        PGA4_0512 = 4,
        PGA5_0256 = 5,
        PGA6_0256 = 6,
        PGA7_0256 = 7,
    };

    ads1115(int i2c_addr);
    ads1115();
    ~ads1115();

    int dev_open();
    int dev_single_shot(double* meas, INPUT_MUX input_mux = P_AIN0_N_AIN1, PGA input_pga = PGA2_2048);

private:
    //INPUT_MUX in_mux;
    //PGA in_pga;

    int fd;
    int addr; 
};

#endif

/*
 * MPU6050 connection demo, based on Intel "Hello World" example for NiosII. Adapted by Tim Gilmour.
 *
 *  This code uses the Intel I2C controller... for another option of OpenCores core, see
 *	https://community.intel.com/t5/FPGA-Intellectual-Property/I2C-OpenCores-not-working/m-p/708254
 *	  (For a different approach that uses VHDL for the I2C state machine, see
 *     https://github.com/danomora/mpu6050-vhdl/
 *
 *	See also https://github.com/alex-mous/MPU6050-C-CPP-Library-for-Raspberry-Pi/blob/master/MPU6050.cpp for example
 *	also https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
 *	and https://cdn.sparkfun.com/datasheets/Sensors/Accelerometers/RM-MPU-6000A.pdf (register map)
 *  and see the ug_embedded_ip pdf for Intel FPGA Avalon I2C (Host) Core API documentation and example code
 *
 *  Tips: put this Eclipse project and BSP project onto the C drive, not a network drive...
 */

#include <sys/alt_stdio.h>
#include <stdio.h>
#include "altera_avalon_pio_regs.h"
#include <unistd.h>
#include <system.h>
#include <stdlib.h>
#include <string.h>
#include "altera_avalon_i2c.h"
#include "io.h"


// testing C++ compilation in Nios Eclipse
class testclass {
    public:
        testclass() { }
        //void initialize();
};


int main() {
	ALT_AVALON_I2C_DEV_t *i2c_dev; //pointer to instance structure
	ALT_AVALON_I2C_STATUS_CODE status;
    ALT_AVALON_I2C_MASTER_CONFIG_t cfg;
	alt_u8 txbuffer[0x200];
	alt_u8 rxbuffer[0x200];
	char in, out;
	int16_t X_accel, Y_accel, Z_accel, temperature, X_gyro, Y_gyro, Z_gyro;

	testclass tc; // for debugging only


	//get a pointer to the Avalon I2C Host Controller instance
	i2c_dev = alt_avalon_i2c_open("/dev/i2c_0");
	if (NULL == i2c_dev) {
		printf("Error: Cannot find /dev/i2c_0\n");
		return 1;
	} else {
		printf("Opened /dev/i2c_0 \n");
	}

	printf("Configuring MPU6050...");

	alt_avalon_i2c_master_config_get(i2c_dev, &cfg);
	// need to change the following line in the altera_avalon_i2c.h if you want to use 400 kHz:
	//  #define ALT_AVALON_I2C_DIFF_LCNT_HCNT 30 // 60 for 100kHz, 15 for 400 kHz, 30 for 200 kHz
	alt_avalon_i2c_master_config_speed_set(i2c_dev, &cfg, 200000);
	alt_avalon_i2c_master_config_set(i2c_dev, &cfg);

    alt_avalon_i2c_master_target_set(i2c_dev, 0x68); //set the address of the device (MPU6050 has address 0x68 or 0x69 depending on ADDRESS pin)

	usleep(1000);
	txbuffer[0] = 0x6b;  txbuffer[1] = 0x00; // power management: turn off sleep mode
	status = alt_avalon_i2c_master_tx(i2c_dev, txbuffer, 2, ALT_AVALON_I2C_NO_INTERRUPTS);

	usleep(1000);
	txbuffer[0] = 0x1a;  txbuffer[1] = 0x03; // frequency config
	status = alt_avalon_i2c_master_tx(i2c_dev, txbuffer, 2, ALT_AVALON_I2C_NO_INTERRUPTS);

	usleep(1000);
	txbuffer[0] = 0x19;  txbuffer[1] = 0x04; // sample rate
	status = alt_avalon_i2c_master_tx(i2c_dev, txbuffer, 2, ALT_AVALON_I2C_NO_INTERRUPTS);

	usleep(1000);
	txbuffer[0] = 0x1b;  txbuffer[1] = 0x00; // gyro config
	status = alt_avalon_i2c_master_tx(i2c_dev, txbuffer, 2, ALT_AVALON_I2C_NO_INTERRUPTS);

	usleep(1000);
	txbuffer[0] = 0x1c;  txbuffer[1] = 0x00; // accel config
	status = alt_avalon_i2c_master_tx(i2c_dev, txbuffer, 2, ALT_AVALON_I2C_NO_INTERRUPTS);

    printf("finished.\n");
	usleep(5000);

	while (1)
	{

		in = IORD_ALTERA_AVALON_PIO_DATA(SWITCHES_BASE); // for debugging only
		out = in;
		IOWR_ALTERA_AVALON_PIO_DATA(LEDS_BASE, out);

		//Read back the data into rxbuffer
		//This command sends the register address, then does a restart and receives the data.
		txbuffer[0] = 0x3B; // read accel_xout_H, accel_xout_L, accel_yout_H, etc
		status = alt_avalon_i2c_master_tx_rx(i2c_dev, txbuffer, 1, rxbuffer, 14, ALT_AVALON_I2C_NO_INTERRUPTS);
		if (status != ALT_AVALON_I2C_SUCCESS) {
			printf("Error after alt_avalon_i2c_master_tx_rx: %d \n", status);
		} else {
			//printf("%02X %02X %02X %02X %02X %02X \n", rxbuffer[0], rxbuffer[1], rxbuffer[2], rxbuffer[3], rxbuffer[4], rxbuffer[5] );

			X_accel = rxbuffer[0] << 8 | rxbuffer[1];
			Y_accel = rxbuffer[2] << 8 | rxbuffer[3];
			Z_accel = rxbuffer[4] << 8 | rxbuffer[5];
			temperature = rxbuffer[6] << 8 | rxbuffer[7]; // broken into separate steps for debugging,
			temperature = ~temperature + 1;               // only using a small amount of the precision, could extract more if needed
			temperature = 37 - (temperature / 340);       // see the datasheet & register map
			X_gyro = rxbuffer[8] << 8 | rxbuffer[9];
			Y_gyro = rxbuffer[10] << 8 | rxbuffer[11];
			Z_gyro = rxbuffer[12] << 8 | rxbuffer[13];
			printf("%d,%d,%d,%d,%d,%d,%d\n", X_accel, Y_accel, Z_accel, temperature, X_gyro, Y_gyro, Z_gyro);
		}

		usleep(100000);
	}

	return 0;
}

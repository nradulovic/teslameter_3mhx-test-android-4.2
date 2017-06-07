package com.teslameter.nr.teslameter;

import java.io.IOException;
import java.util.Locale;

/**
 * Created by nenad on 6/7/17.
 */

public class ADT7410 {
    private static final int CHIP_I2C_ID = 0x48;
    private static final int ADT7410_REG_TEMP_MSB = 0x00;
    private static final int ADT7410_REG_TEMP_LSB = 0x01;
    private static final int ADT7410_REG_STATUS = 0x02;
    private static final int ADT7410_REG_CONFIGURATION = 0x03;
    private static final int ADT7410_REG_ID = 0x0b;
    private static final int ADT7410_REG_RESET = 0x2f;
    private static final int REG_STATUS_NRDY = (0x1 << 7);
    private static final int REG_CONFIGURATION_RESOLUTION = (0x1 << 7);
    private I2cClient i2cClient;

    ADT7410(int bus_id, int chip_id) throws IOException {

        if ((chip_id != 0) && (chip_id != 1)) {
            throw new IllegalArgumentException(String.format(Locale.getDefault(),
                    "Argument chip_id = %d is out of range: [0, 1]", chip_id));
        }
        this.i2cClient = new I2cClient(bus_id, this.CHIP_I2C_ID);

        try {
            /* Reset the chip */
            this.i2cClient.i2cWriteReg(this.ADT7410_REG_RESET, 0x11);
            /* Enable 16-bit mode */
            this.i2cClient.i2cWriteReg(this.ADT7410_REG_CONFIGURATION,
                    this.REG_CONFIGURATION_RESOLUTION);
        } catch (IOException e) {
            throw e;
        }
    }

    public int readRawValue() throws IOException {
        int status = this.i2cClient.i2cReadReg(this.ADT7410_REG_STATUS);

        if ((status & REG_STATUS_NRDY) == REG_STATUS_NRDY) {
            throw new IllegalStateException("Sample not ready");
        }

        int [] buf = this.i2cClient.i2cReadBuf(this.ADT7410_REG_TEMP_MSB, 2);

        return (buf[0] * 256 + buf[1]);
    }

    public float getTemp(int rawValue) {
        return ((float)(rawValue / 128.0));
    }
}

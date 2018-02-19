#ifndef __RTS_IO_I2C_H__
#define __RTS_IO_I2C_H__

/**
 *enum the i2c bus number
 *
 *this enum defined i2c number
 */
enum i2c_bus_nr {
	I2c0	= 0,	/**<i2c-0*/
	I2C_NUM_MAX	/**<number of i2c bus*/
};

/**
 *i2c read data
 *
 *@param i2c_nr		the i2c bus number
 *@param slave_addr	the address of slave device
 *@param count		the length you wangt to read
 *@param buf		pointer to the storage buffer
 *@return 0: success, <0: fail
 */
int rts_io_i2c_read(int i2c_nr, int slave_addr, int count, uint8_t *buf);

/**
 *i2c write data
 *
 *@param i2c_nr		the i2c bus number
 *@param slave_addr	the address of slave device
 *@param count		the length you wangt to write
 *@param buf		pointer to the transfer buffer
 *@return 0: success, <0: fail
 */
int rts_io_i2c_write(int i2c_nr, int slave_addr, int count, uint8_t *buf);

/**
 *read value of some register
 *
 *@param i2c_nr		the i2c bus number
 *@param slave_addr	the address of slave device
 *@param reg		the register you wangt to read
 *@param val		pointer to the storage buffer
 *@return 0: success, <0: fail
 */
int rts_io_i2c_read_reg(int i2c_nr, int slave_addr, int reg, uint8_t *val);

/**
 *write value to some register
 *
 *@param i2c_nr		the i2c bus number
 *@param slave_addr	the address of slave device
 *@param reg		the register you wangt to write
 *@param val		the value you want to write
 *@return 0: success, <0: fail
 */
int rts_io_i2c_write_reg(int i2c_nr, int slave_addr, int reg, uint8_t val);

/**
 *set bits of some register
 *
 *@param i2c_nr		the i2c bus number
 *@param slave_addr	the address of slave device
 *@param reg		the register you wangt to set
 *@param bit_mask	bitmask for bits you want to set
 *@return 0: success, <0: fail
 */
int rts_io_i2c_set_bits(int i2c_nr, int slave_addr, int reg, uint8_t bit_mask);

/**
 *clear bits of some register
 *
 *@param i2c_nr		the i2c bus number
 *@param slave_addr	the address of slave device
 *@param reg		the register you wangt to write
 *@param bit_mask	bitmask for bits you want to clear
 *@return 0: success, <0: fail
 */
int rts_io_i2c_clr_bits(int i2c_nr, int slave_addr, int reg, uint8_t bit_mask);

/**
 *multi read
 *
 *@param i2c_nr		the i2c bus number
 *@param slave_addr	the address of slave device
 *@param reg		the first register you wangt to write
 *@param count		the length you want to read
 *@param val		pointer to the storage buffer
 *@return 0: success, <0: fail
 */
int rts_io_i2c_bulk_read(int i2c_nr, int slave_addr, int reg, int count, uint8_t *val);

/**
 *multi write
 *
 *@param i2c_nr		the i2c bus number
 *@param slave_addr	the address of slave device
 *@param reg		the first register you wangt to write
 *@param count		the length you want to read
 *@param val		pointer to the transfer buffer
 *@return 0: success, <0: fail
 */
int rts_io_i2c_bulk_write(int i2c_nr, int slave_addr, int reg, int count, uint8_t *val);

/**
 *write bits of some register
 *
 *@param i2c_nr		the i2c bus number
 *@param slave_addr	the address of slave device
 *@param reg		the first register you wangt to write
 *@param val		the value you want to write
 *@param mask		bitmask for bits you want to write
 *@return 0: success, <0: fail
 */
int rts_io_i2c_update_bits(int i2c_nr, int slave_addr, int reg, uint8_t val, uint8_t mask);
#endif

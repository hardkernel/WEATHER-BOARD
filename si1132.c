#include "si1132.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

int si1132_begin(const char *device)
{
	int status = 0;

	si1132Fd = open(device, O_RDWR);
	if (si1132Fd < 0) {
		printf("ERROR: open failed\n");
		return -1;
	}

	status = ioctl(si1132Fd, I2C_SLAVE, Si1132_ADDR);
	if (status < 0) {
		printf("ERROR: ioctl error\n");
		close(si1132Fd);
		return -1;
	}

	if (read8(Si1132_REG_PARTID) != 0x32) {
		printf("ERROR: read failed the PART ID\n");
		return -1;
	}

	initialize();
}

void initialize(void)
{
	reset();

	write8(Si1132_REG_UCOEF0, 0x29);
	write8(Si1132_REG_UCOEF1, 0x89);
	write8(Si1132_REG_UCOEF2, 0x02);
	write8(Si1132_REG_UCOEF3, 0x00);

	writeParam(Si1132_PARAM_CHLIST, Si1132_PARAM_CHLIST_ENUV |
			Si1132_PARAM_CHLIST_ENAUX | Si1132_PARAM_CHLIST_ENALSIR |
				Si1132_PARAM_CHLIST_ENALSVIS);
	write8(Si1132_REG_INTCFG, Si1132_REG_INTCFG_INTOE);
	write8(Si1132_REG_IRQEN, Si1132_REG_IRQEN_ALSEVERYSAMPLE);

	writeParam(Si1132_PARAM_ALSIRADCMUX, Si1132_PARAM_ADCMUX_SMALLIR);
	usleep(10000);
	// fastest clocks, clock div 1
	writeParam(Si1132_PARAM_ALSIRADCGAIN, 2);
	usleep(10000);
	// take 511 clocks to measure
	writeParam(Si1132_PARAM_ALSIRADCCOUNTER, Si1132_PARAM_ADCCOUNTER_511CLK);
	// in high range mode
	writeParam(Si1132_PARAM_ALSIRADCMISC, Si1132_PARAM_ALSIRADCMISC_RANGE);
	usleep(10000);
	// fastest clocks
	writeParam(Si1132_PARAM_ALSVISADCGAIN, 3);
	usleep(10000);
	// take 511 clocks to measure
	writeParam(Si1132_PARAM_ALSVISADCCOUNTER, Si1132_PARAM_ADCCOUNTER_511CLK);
	//in high range mode (not normal signal)
	writeParam(Si1132_PARAM_ALSVISADCMISC, Si1132_PARAM_ALSVISADCMISC_VISRANGE);
	usleep(10000);

	write8(Si1132_REG_MEASRATE0, 0xFF);
	write8(Si1132_REG_COMMAND, Si1132_ALS_AUTO);
}

void reset()
{
	write8(Si1132_REG_MEASRATE0, 0);
	write8(Si1132_REG_MEASRATE1, 0);
	write8(Si1132_REG_IRQEN, 0);
	write8(Si1132_REG_IRQMODE1, 0);
	write8(Si1132_REG_IRQMODE2, 0);
	write8(Si1132_REG_INTCFG, 0);
	write8(Si1132_REG_IRQSTAT, 0xFF);

	write8(Si1132_REG_COMMAND, Si1132_RESET);
	usleep(10000);
	write8(Si1132_REG_HWKEY, 0x17);

	usleep(10000);
}

float readVisible()
{
	usleep(10000);
	return read16(0x22) - 250;
}

float readIR()
{
	usleep(10000);
	return read16(0x24) - 250;
}

float readUV()
{
	usleep(10000);
	return read16(0x2c);
}

unsigned char read8(unsigned char reg)
{
	unsigned char ret;
	write(si1132Fd, &reg, 1);
	read(si1132Fd, &ret, 1);
	return ret;
}

unsigned short read16(unsigned char reg)
{
	unsigned char rbuf[2];
	write(si1132Fd, &reg, 1);
	read(si1132Fd, rbuf, 2);

	return (unsigned short)(rbuf[0] | rbuf[1] << 8);
}

void write8(unsigned char reg, unsigned char val)
{
	unsigned char wbuf[2];
	wbuf[0] = reg;
	wbuf[1] = val;
	write(si1132Fd, wbuf, 2);
}

void writeParam(unsigned char param, unsigned char val)
{
	write8(Si1132_REG_PARAMWR, val);
	write8(Si1132_REG_COMMAND, param | Si1132_PARAM_SET);
}

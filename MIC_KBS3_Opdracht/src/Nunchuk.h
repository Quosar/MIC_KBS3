/*
 * nunchuk library
 */
#ifndef NUNCHUK_H_
#define NUNCHUK_H_

#include <inttypes.h>

// don't encode
#define ENCODED         0
#define IDLEN		4 // bytes

struct ncState {
	uint8_t		joy_x_axis;
	uint8_t		joy_y_axis;
	uint16_t	accel_x_axis;
	uint16_t	accel_y_axis;
	uint16_t	accel_z_axis;
	uint8_t		z_button;
	uint8_t		c_button;
};

struct ncCal {
	uint16_t	x0;
	uint16_t	y0;
	uint16_t	z0;
	uint16_t	x1;
	uint16_t	y1;
	uint16_t	z1;
	uint8_t		xmin;
	uint8_t		xmax;
	uint8_t		xcenter;
	uint8_t		ymin;
	uint8_t		ymax;
	uint8_t		ycenter;
	uint16_t	chksum;
};

class NunChuk {
	public:
		NunChuk();
		bool		begin(uint8_t address);
		bool		getState(uint8_t address);
		ncState		state;
		bool		getCalibration(uint8_t address);
		ncCal		cal;
		char		id[2*IDLEN+3]; // '0xAABBCCDD\0'
		uint8_t		read(uint8_t address, uint8_t offset, uint8_t len);
		static uint8_t	buffer[];

	private:
		bool	_getId(uint8_t address);
		uint8_t _read(uint8_t address, uint8_t offset, uint8_t len);
		uint8_t _decode(uint8_t b);
};

// Nunchuk globaly declared in Nunchuk.cpp
extern NunChuk Nunchuk;

#endif

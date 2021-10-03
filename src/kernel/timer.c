#include "stdlib.h"
#include "timer.h"

//-----------------------------------------------
//	Controller Registers
//-----------------------------------------------

#define		I86_PIT_REG_COUNTER0		0x40
#define		I86_PIT_REG_COUNTER1		0x41
#define		I86_PIT_REG_COUNTER2		0x42
#define		I86_PIT_REG_COMMAND			0x43

// Global Tick count
static volatile uint32 _pit_ticks = 0;

void inc_pit_ticks() {
    _pit_ticks++;
}

// returns current tick count
uint32 get_tick_count () {
	return _pit_ticks;
}

// send command to pit
void i86_pit_send_command (uint8 cmd) {
	outportb(I86_PIT_REG_COMMAND, cmd);
}

// send data to a counter
void i86_pit_send_data (uint16 data, uint8 counter) {
	uint8 port = counter==I86_PIT_OCW_COUNTER_0
        ? I86_PIT_REG_COUNTER0 
        : ((counter==I86_PIT_OCW_COUNTER_1) 
            ? I86_PIT_REG_COUNTER1 
            : I86_PIT_REG_COUNTER2);
	outportb(port, (uint8)data);
}

// read data from counter
uint8 i86_pit_read_data (uint16 counter) {
	uint8 port= (counter==I86_PIT_OCW_COUNTER_0) ? I86_PIT_REG_COUNTER0 :
		((counter==I86_PIT_OCW_COUNTER_1) ? I86_PIT_REG_COUNTER1 : I86_PIT_REG_COUNTER2);
    uint8 result;
	inportb(port, result);
	return result;
}

// starts a counter
void i86_pit_start_counter (uint32 freq, uint8 counter, uint8 mode) {
	if (freq == 0)
		return;

	uint16 divisor = (uint16)(1193181 / (uint16)freq);

	// send operational command
	uint8 ocw=0;
	ocw = (ocw & ~I86_PIT_OCW_MASK_MODE) | mode;
	ocw = (ocw & ~I86_PIT_OCW_MASK_RL) | I86_PIT_OCW_RL_DATA;
	ocw = (ocw & ~I86_PIT_OCW_MASK_COUNTER) | counter;
	i86_pit_send_command (ocw);

	// set frequency rate
	i86_pit_send_data (divisor & 0xff, 0);
	i86_pit_send_data ((divisor >> 8) & 0xff, 0);

	// reset tick count
	_pit_ticks = 0;
}

// sleeps a little bit
void sleep(int ms) {

	int ticks = ms + get_tick_count();
	while (ticks > get_tick_count())
		;
}
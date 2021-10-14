#include <stdlib.h>
#include <floppy.h>

/*
**	Controller I/O Ports.
*/
enum FLPYDSK_IO {
	FLPYDSK_DOR		=	0x3f2,
	FLPYDSK_MSR		=	0x3f4,
	FLPYDSK_FIFO	=	0x3f5,
	FLPYDSK_CTRL	=	0x3f7
};

/**
*	Bits 0-4 of command byte.
*/
enum FLPYDSK_CMD {
	FDC_CMD_READ_TRACK	=	2,
	FDC_CMD_SPECIFY		=	3,
	FDC_CMD_CHECK_STAT	=	4,
	FDC_CMD_WRITE_SECT	=	5,
	FDC_CMD_READ_SECT	=	6,
	FDC_CMD_CALIBRATE	=	7,
	FDC_CMD_CHECK_INT	=	8,
	FDC_CMD_FORMAT_TRACK=	0xd,
	FDC_CMD_SEEK		=	0xf
};

/**
*	Additional command masks. Can be masked with above commands
*/
enum FLPYDSK_CMD_EXT {
	FDC_CMD_EXT_SKIP		=	0x20,	//00100000
	FDC_CMD_EXT_DENSITY		=	0x40,	//01000000
	FDC_CMD_EXT_MULTITRACK	=	0x80	//10000000
};

/*
**	Digital Output Register
*/
enum FLPYDSK_DOR_MASK {
	FLPYDSK_DOR_MASK_DRIVE0			=	0,	//00000000	= here for completeness sake
	FLPYDSK_DOR_MASK_DRIVE1			=	1,	//00000001
	FLPYDSK_DOR_MASK_DRIVE2			=	2,	//00000010
	FLPYDSK_DOR_MASK_DRIVE3			=	3,	//00000011
	FLPYDSK_DOR_MASK_RESET			=	4,	//00000100
	FLPYDSK_DOR_MASK_DMA			=	8,	//00001000
	FLPYDSK_DOR_MASK_DRIVE0_MOTOR	=	16,	//00010000
	FLPYDSK_DOR_MASK_DRIVE1_MOTOR	=	32,	//00100000
	FLPYDSK_DOR_MASK_DRIVE2_MOTOR	=	64,	//01000000
	FLPYDSK_DOR_MASK_DRIVE3_MOTOR	=	128	//10000000
};

/**
*	Main Status Register
*/
enum FLPYDSK_MSR_MASK {
	FLPYDSK_MSR_MASK_DRIVE1_POS_MODE	=	1,	//00000001
	FLPYDSK_MSR_MASK_DRIVE2_POS_MODE	=	2,	//00000010
	FLPYDSK_MSR_MASK_DRIVE3_POS_MODE	=	4,	//00000100
	FLPYDSK_MSR_MASK_DRIVE4_POS_MODE	=	8,	//00001000
	FLPYDSK_MSR_MASK_BUSY				=	16,	//00010000
	FLPYDSK_MSR_MASK_DMA				=	32,	//00100000
	FLPYDSK_MSR_MASK_DATAIO				=	64, //01000000
	FLPYDSK_MSR_MASK_DATAREG			=	128	//10000000
};

/**
*	Controller Status Port 0
*/
enum FLPYDSK_ST0_MASK {
	FLPYDSK_ST0_MASK_DRIVE0		=	0,		//00000000	=	for completness sake
	FLPYDSK_ST0_MASK_DRIVE1		=	1,		//00000001
	FLPYDSK_ST0_MASK_DRIVE2		=	2,		//00000010
	FLPYDSK_ST0_MASK_DRIVE3		=	3,		//00000011
	FLPYDSK_ST0_MASK_HEADACTIVE	=	4,		//00000100
	FLPYDSK_ST0_MASK_NOTREADY	=	8,		//00001000
	FLPYDSK_ST0_MASK_UNITCHECK	=	16,		//00010000
	FLPYDSK_ST0_MASK_SEEKEND	=	32,		//00100000
	FLPYDSK_ST0_MASK_INTCODE	=	64		//11000000
};

/*
** LPYDSK_ST0_MASK_INTCODE types
*/
enum FLPYDSK_ST0_INTCODE_TYP {
	FLPYDSK_ST0_TYP_NORMAL		=	0,
	FLPYDSK_ST0_TYP_ABNORMAL_ERR=	1,
	FLPYDSK_ST0_TYP_INVALID_ERR	=	2,
	FLPYDSK_ST0_TYP_NOTREADY	=	3
};

/**
*	GAP 3 sizes
*/
enum FLPYDSK_GAP3_LENGTH {
	FLPYDSK_GAP3_LENGTH_STD = 42,
	FLPYDSK_GAP3_LENGTH_5_14= 32,
	FLPYDSK_GAP3_LENGTH_3_5= 27
};

/*
**	Formula: 2^sector_number * 128, where ^ denotes "to the power of"
*/
enum FLPYDSK_SECTOR_DTL {
	FLPYDSK_SECTOR_DTL_128	=	0,
	FLPYDSK_SECTOR_DTL_256	=	1,
	FLPYDSK_SECTOR_DTL_512	=	2,
	FLPYDSK_SECTOR_DTL_1024	=	4
};

// floppy irq
const int FLOPPY_IRQ = 6;

// sectors per track
const int FLPY_SECTORS_PER_TRACK = 18;

// You can change this as needed. It must be below 16MB and in identity mapped memory!
// todo: use physical memory manager
int DMA_BUFFER = 0x20000;

// FDC uses DMA channel 2
const uint8 FDC_DMA_CHANNEL = 2;

// used to wait in miliseconds
extern void sleep(int);

//  current working drive. Defaults to 0 which should be fine on most systems
static uint8 current_drive = 0;

// set when IRQ fires
static volatile bool irq_received = false;

bool dma_initialize_floppy(uint8* buffer, unsigned length) {
	//todo: calc buffer address and extended page address register
	
   union {
      uint8 byte[4]; //Lo[0], Mid[1], Hi[2]
      unsigned long l;
   } a, c;

   a.l = (unsigned) buffer;
   c.l = (unsigned) length - 1;

   //Check for buffer issues
   if ((a.l >> 24) || (c.l >> 16) || (((a.l & 0xffff) + c.l) >> 16)) {
      return false;
   }

   dma_reset(1);
   dma_mask_channel(FDC_DMA_CHANNEL); //Mask channel 2

   dma_reset_flipflop(FDC_DMA_CHANNEL); //Flipflop reset on DMA 1
   
   dma_set_read(FDC_DMA_CHANNEL); //set the DMA for read transfer

   dma_set_address(FDC_DMA_CHANNEL, a.byte[0], a.byte[1]); //Buffer address
   
   dma_set_external_page_register(2, a.byte[2]); // Page number
  
   dma_set_count(FDC_DMA_CHANNEL, c.byte[0], c.byte[1]); //Set count
   
   dma_unmask_all(1); //Unmask channel 2

   return true;
}

// sets DMA base address
void flpydsk_set_dma (int addr) {
	DMA_BUFFER = addr;
}

// return fdc status
uint8 flpydsk_read_status () {
	uint8 result;
	inportb(FLPYDSK_MSR, result);
	// just return main status register
	return result;
}

// write to the fdc dor
void flpydsk_write_dor(uint8 val) {
	// write the digital output register
	outportb(FLPYDSK_DOR, val);
}

// send command byte to fdc
void flpydsk_send_command(uint8 cmd) {
	// wait until data register is ready. We send commands to the data register
	for (int i = 0; i < 500; i++ )
		if ( flpydsk_read_status () & FLPYDSK_MSR_MASK_DATAREG ) {
			outportb (FLPYDSK_FIFO, cmd);
			return;
		}	
}

// get data from fdc
uint8 flpydsk_read_data () {
	// same as above function but returns data register for reading
	for (int i = 0; i < 500; i++ )
		if (flpydsk_read_status () & FLPYDSK_MSR_MASK_DATAREG ) {
			uint8 result;
			inportb(FLPYDSK_FIFO, result);
			return result;
		}

	return 0;
}

// write to the configuation control register
void flpydsk_write_ccr (uint8 val) {
	// write the configuration control
	outportb(FLPYDSK_CTRL, val);
}

// wait for irq to fire
void flpydsk_wait_irq () {
	// wait for irq to fire
	while (irq_received == false) ;
	
	irq_received = false;
}

// floppy disk irq handler
void i86_flpy_irq() {
	irq_received = true;
}

// check interrupt status command
void flpydsk_check_int(uint32* st0, uint32* cyl) {
	flpydsk_send_command(FDC_CMD_CHECK_INT);

	*st0 = flpydsk_read_data();
	*cyl = flpydsk_read_data();
}

// turns the current floppy drives motor on/off
void flpydsk_control_motor(bool b) {
	// sanity check: invalid drive
	if (current_drive > 3)
		return;

	uint8 motor = 0;

	// select the correct mask based on current drive
	switch (current_drive) {
		case 0:
			motor = FLPYDSK_DOR_MASK_DRIVE0_MOTOR;
			break;
		case 1:
			motor = FLPYDSK_DOR_MASK_DRIVE1_MOTOR;
			break;
		case 2:
			motor = FLPYDSK_DOR_MASK_DRIVE2_MOTOR;
			break;
		case 3:
			motor = FLPYDSK_DOR_MASK_DRIVE3_MOTOR;
			break;
	}

	// turn on or off the motor of that drive
	if (b)
		flpydsk_write_dor((uint8)(current_drive | motor | FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA));
	else
		flpydsk_write_dor(FLPYDSK_DOR_MASK_RESET);

	// in all cases; wait a little bit for the motor to spin up/turn off
	sleep(20);
}

// configure drive
void flpydsk_drive_data(uint8 stepr, uint8 loadt, uint8 unloadt, bool dma) {
	uint8 data = 0;

	// send command
	flpydsk_send_command(FDC_CMD_SPECIFY);
	data = ((stepr & 0xf) << 4) | (unloadt & 0xf);
	flpydsk_send_command(data);
	data = ((loadt << 1) | ((dma) ? 0 : 1));
	flpydsk_send_command(data);
}

// calibrates the drive
int flpydsk_calibrate(uint8 drive) {
	uint32 st0, cyl;

	if (drive >= 4)
		return -2;

	// turn on the motor
	flpydsk_control_motor(true);

	for (int i = 0; i < 10; i++) {
		// send command
		flpydsk_send_command(FDC_CMD_CALIBRATE);
		flpydsk_send_command(drive);
		flpydsk_wait_irq();
		flpydsk_check_int(&st0, &cyl);

		// did we fine cylinder 0? if so, we are done
		if (!cyl) {
			flpydsk_control_motor (false);
			return 0;
		}
	}

	flpydsk_control_motor(false);
	return -1;
}

// disable controller
void flpydsk_disable_controller () {
	flpydsk_write_dor(0);
}

// enable controller
void flpydsk_enable_controller () {
	flpydsk_write_dor(FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA);
}

void flpydsk_reset() {
	uint32 st0, cyl;

	// reset the controller
	flpydsk_disable_controller();
	flpydsk_enable_controller();
	flpydsk_wait_irq();

	// send CHECK_INT/SENSE INTERRUPT command to all drives
	for (int i = 0; i < 4; i++)
		flpydsk_check_int(&st0,&cyl);
}

// read a sector
void flpydsk_read_sector_imp(uint8 head, uint8 track, uint8 sector) {
	uint32 st0, cyl;

	// initialize DMA
	dma_initialize_floppy((uint8*)DMA_BUFFER, 512);

	// read in a sector
	flpydsk_send_command (
				FDC_CMD_READ_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
	flpydsk_send_command(head << 2 | current_drive);
	flpydsk_send_command(track);
	flpydsk_send_command(head);
	flpydsk_send_command(sector);
	flpydsk_send_command(FLPYDSK_SECTOR_DTL_512);
	flpydsk_send_command(((sector + 1) >= FLPY_SECTORS_PER_TRACK ) 
		? FLPY_SECTORS_PER_TRACK 
		: sector + 1);
	flpydsk_send_command(FLPYDSK_GAP3_LENGTH_3_5);
	flpydsk_send_command(0xff);

	// wait for irq
	flpydsk_wait_irq ();

	// read status info
	for (int j = 0; j < 7; j++) {
		flpydsk_read_data();
	}
	
	// let FDC know we handled interrupt
	flpydsk_check_int(&st0,&cyl);
}

// seek to given track/cylinder
int flpydsk_seek(uint8 cyl, uint8 head) {
	uint32 st0, cyl0;

	if (current_drive >= 4)
		return -1;

	for (int i = 0; i < 10; i++ ) {
		// send the command
		flpydsk_send_command(FDC_CMD_SEEK);
		flpydsk_send_command((head) << 2 | current_drive);
		flpydsk_send_command(cyl);

		// wait for the results phase IRQ
		flpydsk_wait_irq();
		flpydsk_check_int(&st0, &cyl0);

		// found the cylinder?
		if(cyl0 == cyl)
			return 0;
	}

	return -1;
}

// convert LBA to CHS
void flpydsk_lba_to_chs (int lba,int *head,int *track,int *sector) {
   *head = ( lba % ( FLPY_SECTORS_PER_TRACK * 2 ) ) / ( FLPY_SECTORS_PER_TRACK );
   *track = lba / ( FLPY_SECTORS_PER_TRACK * 2 );
   *sector = lba % FLPY_SECTORS_PER_TRACK + 1;
}

// install floppy driver
void flpydsk_install () {
	// reset the fdc
	flpydsk_reset();

	// transfer speed 500kb/s
	flpydsk_write_ccr(0);

	// pass mechanical drive info. steprate=3ms, unload time=240ms, load time=16ms
	flpydsk_drive_data(3, 16, 240, true);

	// calibrate the disk
	flpydsk_calibrate(current_drive);
}

// set current working drive
void flpydsk_set_working_drive (uint8 drive) {
	if (drive < 4)
		current_drive = drive;
}

// get current working drive
uint8 flpydsk_get_working_drive () {
	return current_drive;
}

// read a sector
uint8* flpydsk_read_sector (uint32 sectorLBA) {
	if (current_drive >= 4)
		return 0;

	// convert LBA sector to CHS
	int head = 0, track = 0, sector = 1;
	flpydsk_lba_to_chs(sectorLBA, &head, &track, &sector);

	// turn motor on and seek to track
	flpydsk_control_motor(true);
	if (flpydsk_seek((uint8)track, (uint8)head) != 0)
		return 0;

	// read sector and turn motor off
	flpydsk_read_sector_imp((uint8)head, (uint8)track, (uint8)sector);
	flpydsk_control_motor(false);

	flpydsk_reset();//todo: remove after fix a bug after first sector reading
	
	// warning: this is a bit hackish
	return (int8*) DMA_BUFFER;
}
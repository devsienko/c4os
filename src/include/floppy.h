#ifndef FLPYDSK_DRIVER_H
#define FLPYDSK_DRIVER_H

#include <stdlib.h>

// sets DMA address
void flpydsk_set_dma(int addr);

// install floppy driver
void flpydsk_install();

// set current working drive
void flpydsk_set_working_drive(uint8 drive);

// get current working drive
uint8 flpydsk_get_working_drive();

// read a sector
uint8* flpydsk_read_sector(uint32 sectorLBA);

// converts an LBA address to CHS
void flpydsk_lba_to_chs(int lba, int *head, int *track, int *sector);

#endif
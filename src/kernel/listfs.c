#include "stdlib.h"
#include "listfs.h"
#include "memory_manager.h"
#include "dma.h"

listfs_file_header* get_file_info_by(uint32 sector_number) {
    flpydsk_set_dma(TEMP_DMA_BUFFER_ADDR);
    listfs_file_header *first_file_header 
        = (listfs_file_header*)flpydsk_read_sector(sector_number);
    return first_file_header;
}
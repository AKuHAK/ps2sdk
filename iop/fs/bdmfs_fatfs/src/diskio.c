/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <bdm.h>
#include "ff.h"     /* Obtains integer types */
#include "diskio.h" /* Declarations of disk functions */

#include "fs_driver.h"

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(
    BYTE pdrv /* Physical drive number to identify the drive */
)
{
    DSTATUS stat;
    int result;

    result = (mounted_bd == NULL) ? STA_NODISK : FR_OK;

    return result;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
    BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
    DSTATUS stat;
    int result;

    result = (mounted_bd == NULL) ? STA_NODISK : FR_OK;

    return result;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(
    BYTE pdrv,    /* Physical drive nmuber to identify the drive */
    BYTE *buff,   /* Data buffer to store read data */
    LBA_t sector, /* Start sector in LBA */
    UINT count    /* Number of sectors to read */
)
{
    DRESULT res;

    res = mounted_bd->read(mounted_bd, sector, buff, count);

    return (res==count)? FR_OK : FR_DISK_ERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(
    BYTE pdrv,        /* Physical drive nmuber to identify the drive */
    const BYTE *buff, /* Data to be written */
    LBA_t sector,     /* Start sector in LBA */
    UINT count        /* Number of sectors to write */
)
{
    DRESULT res;

    res = mounted_bd->write(mounted_bd, sector, buff, count);

    return (res==count)? FR_OK : FR_DISK_ERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(
    BYTE pdrv, /* Physical drive nmuber (0..) */
    BYTE cmd,  /* Control code */
    void *buff /* Buffer to send/receive control data */
)
{
    DRESULT res;

	res = FR_OK;

	switch (cmd){
		case CTRL_SYNC:
			mounted_bd->flush(mounted_bd);
			break;
		case GET_SECTOR_COUNT:
			*(unsigned int*)buff = mounted_bd->sectorCount;
			break;
		case GET_SECTOR_SIZE:
			*(unsigned int*)buff = mounted_bd->sectorSize;
			break;
		case GET_BLOCK_SIZE:
			*(unsigned int*)buff = 0;
			break;
	}

    return res;
}

/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

/**
 *  @file
 *  @brief Power-off library.
 *  @details Poweroff is a library that substitutes for the missing poweroff
 *  functionality in rom0:CDVDMAN, which is offered by newer CDVDMAN
 *  modules (i.e. sceCdPoweroff). Other than allowing the PlayStation 2
 *  to be switched off via software means, this is used to safeguard
 *  against the user switching off/resetting the console before data
 *  can be completely written to disk.
 */

#ifndef __LIBPWROFF_H__
#define __LIBPWROFF_H__

#define POWEROFF_THREAD_PRIORITY 0x70

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*poweroff_callback)(void *arg);

/**
 *  @brief Initializes the poweroff library.
 *  @details A service thread with a default priority of POWEROFF_THREAD_PRIORITY
 *  will be created.
 */
int poweroffInit(void);

/**
 *  @brief Set callback function
 *
 *  @param [in] cb Function that will be called when power button is pressed.
 *  @param [in] arg Arguments that are sent to cb function (can be NULL)
 *
 *  @details Callback function should be defined elsewhere. There are some
 *  standart specifications. Last function inside callback should be
 *  poweroffShutdown.
 *  You should close all files (close(fd)) and unmount all partitions. If you
 *  use PFS, close all files and unmount all partitions.
 *  fileXioDevctl("pfs:", PDIOC_CLOSEALL, NULL, 0, NULL, 0)
 *  Shut down DEV9 (Network module), if you used it.
 *  while(fileXioDevctl("dev9x:", DDIOC_OFF, NULL, 0, NULL, 0) < 0){};
 */
void poweroffSetCallback(poweroff_callback cb, void *arg);

/**
 *  @brief Immidiate console shutdown.
 *
 */
void poweroffShutdown(void);

/**
 *  @brief Change thread priority
 *
 *  @details The callback thread runs at priority
 *  POWEROFF_THREAD_PRIORITY by default. You can change the priority
 *  by calling this function after poweroffSetCallback.
 *  You can call this function before poweroffSetCallback as well (but after
 *  poweroffInit). In that case poweroffSetCallback will be registered with
 *  the provided value.
 */
void poweroffChangeThreadPriority(int priority);

#ifdef __cplusplus
}
#endif

#endif /* __LIBPWROFF_H__ */

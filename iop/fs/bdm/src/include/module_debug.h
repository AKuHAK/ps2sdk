#ifndef _MODULE_DEBUG_H
#define _MODULE_DEBUG_H

//#define MINI_DRIVER

#ifndef MINI_DRIVER
#define M_DEBUG(format, args...) printf("BDM: " format, ##args)
#else
#define M_DEBUG(format, args...)
#endif

#ifdef DEBUG
#define M_DEBUG M_DEBUG
#else
#define M_DEBUG(format, args...)
#endif

#endif

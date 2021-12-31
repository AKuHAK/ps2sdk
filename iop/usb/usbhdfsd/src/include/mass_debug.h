#ifndef _MASS_DEBUG_H
#define _MASS_DEBUG_H 1

#ifdef DEBUG
#define M_DEBUG(format, args...)	printf("USBHDFSD: " format, ##args)
#else
#define M_DEBUG(args...)
#endif

#endif  /* _MASS_DEBUG_H */

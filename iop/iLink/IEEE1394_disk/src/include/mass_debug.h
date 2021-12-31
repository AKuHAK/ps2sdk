#ifndef _MASS_DEBUG_H
#define _MASS_DEBUG_H 1

#ifdef DEBUG
#define M_DEBUG(format, args...)	printf("IEEE1394_disk: " format, ##args)
#define iM_DEBUG(format, args...)	Kprintf("IEEE1394_disk: " format, ##args)
#else
#define M_DEBUG(format, args...)
#define iM_DEBUG(format, args...)
#endif

#endif  /* _MASS_DEBUG_H */

#ifndef __SONY_RX_H__
#define __SONY_RX_H__

extern int IsSonyRXModule(const char *path);
extern int GetSonyRXModInfo(const char *path, char *description, unsigned int MaxLength, unsigned short int *version);

#endif /* __SONY_RX_H__ */
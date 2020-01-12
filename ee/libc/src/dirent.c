#include <dirent.h>
#include <stdio.h>

#if defined(F_opendir) || defined(DOXYGEN)
DIR *opendir(const char *path)
{
	printf("opendir not implemented\n");
	return 0;
}
#endif

#if defined(F_readdir) || defined(DOXYGEN)
struct dirent *readdir(DIR *dir)
{
	printf("readdir not implemented\n");
	return 0;
}
#endif

#if defined(F_rewinddir) || defined(DOXYGEN)
void rewinddir(DIR *dir)
{
	printf("rewinddir not implemented\n");
}
#endif

#if defined(F_closedir) || defined(DOXYGEN)
int closedir(DIR *dir)
{
	printf("closedir not implemented\n");
	return 0;
}
#endif

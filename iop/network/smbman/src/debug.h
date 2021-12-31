/*
  Copyright 2009-2010, jimmikaelkael
  Licenced under Academic Free License version 3.0
*/

#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef DEBUG
#define M_DEBUG(args...)	printf(args)
#else
#define M_DEBUG(args...)
#endif

#endif

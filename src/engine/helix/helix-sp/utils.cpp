/*
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 * Portions (c) Paul Cifarelli 2005
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "utils.h"


int SafeSprintf(char *str, int max, const char *fmt, ...)
{
   int ret;
   va_list ap;
   va_start(ap, fmt);
   ret = vsnprintf(str,max,fmt,ap);
   va_end(ap);
   return ret;
}

char *SafeStrCpy(char *str1, const char *str2, int sz)
{
   int len = strlen(str2);
   if (len > sz)
      return 0;

   return (strcpy(str1,str2));
}

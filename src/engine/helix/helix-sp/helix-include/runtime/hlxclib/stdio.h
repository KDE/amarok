
/*
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 *
 */

#ifndef HLXSYS_STDIO_H
#define HLXSYS_STDIO_H

#if defined(_OPENWAVE)
#include "platform/openwave/hx_op_debug.h"
#include "platform/openwave/hx_op_stdc.h"
#include "platform/openwave/hx_op_fs.h"
#include "hlxclib/sys/types.h"
#else
#include <stdio.h>
#include <stdarg.h>
#endif

#if __cplusplus
extern "C" {
#endif
/* Make sure vsnprintf is defined for all platforms */

int __helix_snprintf(char *str, size_t size, const char  *format, ...);
int __helix_vsnprintf(char *str, size_t size, const char  *format, va_list ap);


#if defined(_OPENWAVE)
int __helix_printf(const char* format, ...);
int __helix_vprintf(const char  *format, va_list ap);

int __helix_sscanf(const char *buffer, const char *format, ...);

#define printf __helix_printf
#define vprintf __helix_vprintf
#define snprintf op_snprintf
#define vsnprintf __helix_vsnprintf
#define _vsnprintf __helix_vsnprintf
#define sscanf  __helix_sscanf
#define unlink OpFsRemove

typedef void* FILE;
#define stdin  (FILE*)0
#define stdout (FILE*)1
#define stderr (FILE*)2

#ifndef EOF
#define EOF ((size_t)-1)

FILE*	__helix_fopen(const char *, const char *);
size_t	__helix_fread(void *, size_t, size_t, FILE *);
size_t	__helix_fwrite(const void *, size_t, size_t, FILE *);
int		__helix_fseek(FILE *, long, int);
int		__helix_fclose(FILE *);
int		__helix_feof(FILE *);
long	__helix_ftell(FILE *);
char*	__helix_fgets(char*, int, FILE *);
int		__helix_fputc(int, FILE *);
int		__helix_ferror(FILE *);

int		__helix_fflush(FILE *);
int		__helix_rename(const char *oldname, const char *newname);

FILE*	__helix_fdopen(int, const char *);
int     __helix_fileno(FILE* );

int __helix_fprintf(FILE* f, const char *format, ...);
int __helix_vfprintf(FILE* f, const char  *format, va_list ap);
#define puts(x) printf("%s\n", (x))

#define fopen	__helix_fopen
#define fread	__helix_fread
#define fseek	__helix_fseek
#define fwrite	__helix_fwrite
#define fclose	__helix_fclose
#define feof	__helix_feof
#define ftell	__helix_ftell
#define fgets	__helix_fgets
#define fputc	__helix_fputc
#define putc	__helix_fputc
#define ferror  __helix_ferror
#define rewind(fp)  __helix_fseek(fp, 0, SEEK_SET)

#define fprintf __helix_fprintf
#define vfprintf __helix_fprintf

#define fflush	__helix_fflush             
#define rename  __helix_rename

#define _fdopen  __helix_fdopen

#define fileno  __helix_fileno

#endif	// end of _OPENWAVE

#elif defined(_WINDOWS)
#define snprintf _snprintf
#define vsnprintf _vsnprintf

#elif defined(_SYMBIAN) || defined(_WINCE) || defined(_IRIX)
#define snprintf __helix_snprintf
#define vsnprintf __helix_vsnprintf
#endif

#if	__cplusplus
}
#endif

#endif /* HLXSYS_STDIO_H */

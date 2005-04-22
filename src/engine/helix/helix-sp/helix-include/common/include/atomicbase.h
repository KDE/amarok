
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


/***********************************************************************
 *  THIS CODE IS HIGHLY CRITICAL TO THE SERVER'S STABILITY!!!
 *  DO NOT MAKE CHANGES TO THE ATOMIC OPERATORS OR TO THE
 *  MUTEX CODE WITHOUT A SERVER TEAM CODE-REVIEW! (dev@helix-server)
 */


/****************************************************************************
 *  $Id$
 *
 *  atomicbase.h - Defines several atomic operations
 * 
 *  See server/common/util/pub/servatomic.h for broader platform support.
 *  Also conditionally overrides InterlockedIncrement/Decrement
 *  via USE_HX_ATOMIC_INTERLOCKED_INC_DEC.
 *
 *
 ***********************************************************************
 *
 * Defines:
 *
 * void HXAtomicIncINT32(INT32* p)             -- Increment *p
 * void HXAtomicDecINT32(INT32* p)             -- Decrement *p
 * void HXAtomicAddINT32(INT32* p, INT32 n)    -- Increment *p by n 
 * void HXAtomicSubINT32(INT32* p, INT32 n)    -- Decrement *p by n
 * INT32 HXAtomicIncRetINT32(INT32* p)         -- Increment *p and return it
 * INT32 HXAtomicDecRetINT32(INT32* p)         -- Decrement *p and return it
 * INT32 HXAtomicAddRetINT32(INT32* p, INT32 n)-- Increment *p by n, return it
 * INT32 HXAtomicSubRetINT32(INT32* p, INT32 n)-- Increment *p by n, return it
 *
 *
 * There are also UINT32 versions:
 *
 * void HXAtomicIncUINT32(UINT32* p)
 * void HXAtomicDecUINT32(UINT32* p)
 * void HXAtomicAddUINT32(UINT32* p, UINT32 n)
 * void HXAtomicSubUINT32(UINT32* p, UINT32 n)
 * UINT32 HXAtomicIncRetUINT32(UINT32* p)
 * UINT32 HXAtomicDecRetUINT32(UINT32* p)
 * UINT32 HXAtomicAddRetUINT32(UINT32* p, UINT32 n)
 * UINT32 HXAtomicSubRetUINT32(UINT32* p, UINT32 n)
 *
 ***********************************************************************
 *
 * TODO:
 *   Add INT64 versions
 *   Obsolete the 0x80000000-based Solaris implementation entirely.
 *
 ***********************************************************************/
#ifndef _ATOMICBASE_H_
#define _ATOMICBASE_H_


/***********************************************************************
 * Sun Solaris / SPARC (Native compiler)
 *
 * Implementation Notes:
 * This uses inline assembly from server/common/util/platform/solaris/atomicops.il
 * Note: Sparc/gcc is in include/atomicbase.h
 */
#if defined (_SOLARIS) && !defined (__GNUC__)

#if defined(__cplusplus)
extern "C" {
#endif
    //UINT32 _HXAtomicIncRetUINT32 (UINT32* pNum);
    //UINT32 _HXAtomicDecRetUINT32 (UINT32* pNum);
    UINT32 _HXAtomicAddRetUINT32 (UINT32* pNum, UINT32 ulNum);
    UINT32 _HXAtomicSubRetUINT32 (UINT32* pNum, UINT32 ulNum);
#if defined(__cplusplus)
}
#endif


#define HXAtomicIncUINT32(p)      _HXAtomicAddRetUINT32((p),(UINT32)1)
#define HXAtomicDecUINT32(p)      _HXAtomicSubRetUINT32((p),(UINT32)1)
#define HXAtomicIncRetUINT32(p)   _HXAtomicAddRetUINT32((p),(UINT32)1)
#define HXAtomicDecRetUINT32(p)   _HXAtomicSubRetUINT32((p),(UINT32)1)
#define HXAtomicAddUINT32(p,n)    _HXAtomicAddRetUINT32((p),(n))
#define HXAtomicSubUINT32(p,n)    _HXAtomicSubRetUINT32((p),(n))
#define HXAtomicAddRetUINT32(p,n) _HXAtomicAddRetUINT32((p),(n))
#define HXAtomicSubRetUINT32(p,n) _HXAtomicSubRetUINT32((p),(n))

inline void HXAtomicIncINT32(INT32* p)              { HXAtomicIncUINT32((UINT32*)p); }
inline void HXAtomicDecINT32(INT32* p)              { HXAtomicDecUINT32((UINT32*)p); }
inline void HXAtomicAddINT32(INT32* p, INT32 n)     { HXAtomicAddUINT32((UINT32*)p, (UINT32)n); }
inline void HXAtomicSubINT32(INT32* p, INT32 n)     { HXAtomicSubUINT32((UINT32*)p, (UINT32)n); }
inline INT32 HXAtomicIncRetINT32(INT32* p)          { return HXAtomicIncRetUINT32((UINT32*)p); }
inline INT32 HXAtomicDecRetINT32(INT32* p)          { return HXAtomicDecRetUINT32((UINT32*)p); }
inline INT32 HXAtomicAddRetINT32(INT32* p, INT32 n) { return HXAtomicAddRetUINT32((UINT32*)p, (UINT32)n); }
inline INT32 HXAtomicSubRetINT32(INT32* p, INT32 n) { return HXAtomicSubRetUINT32((UINT32*)p, (UINT32)n); }



/***********************************************************************
 * Sun Solaris / SPARC (gcc)
 *
 * Implementation Notes:
 * The sparc method of pipelining and use of "delay slots" requires
 * the nop's.  Be extra careful modifying these routines!
 *
 * This implementation sacrifices being able to store the value
 * 0x800000000 in the INT32 value, which is a special "busy" marker value.
 * Since these are intended for use primarily with AddRef/Release and
 * resource usage counters, this should be acceptable for now.  If a counter
 * is incremented to the point it would conflict with the flag, it is
 * incremented one more to hop over it.  The same in reverse for decrement.
 * This is far from ideal, however...  See the inline-assembly file
 * server/common/util/platform/solaris/mutex_setbit.il for *much*
 * better implementations using newer sparc assembly operators.
 *
 * Basic design of the flag-based implementation:
 *   1. Load a register with 0x80000000
 *   2. _atomically_ swap it with the INT32 (critical!)
 *   3. Compare what we got with 0x80000000
 *   4. Branch if equal to #2
 *   5. Increment (or decrement) the result
 *   6. Compare to 0x80000000
 *   7. Branch if equal to #5
 *   8. Save the new value to the INT32's location in memory
 *   9. Return new INT32 result if required
 *   
 * This implementation primarily exists due to limitations in the ancient
 * version of gcc we used to use on Solaris (2.7.2.3), and more modern
 * gcc's can probably handle assembly more like what's used in Sun's
 * Native compiler version.  
 *
 */
#elif defined (__sparc__) && defined (__GNUC__)

/* Increment by 1 */
inline void
HXAtomicIncUINT32(UINT32* pNum)
{
    __asm__ __volatile__(\
"1:      swap    [%0], %2;               ! Swap *pNum and %2\n"
"        nop;                            ! delay slot...\n"
"        cmp     %2, %1;                 ! Is someone else using pNum?\n"
"        be      1b;                     ! If so, retry...\n"
"        nop;                            ! delay slot...yawn\n"
"2:      inc     %2;                     ! Increment %2\n"
"        cmp     %2, %1;                 ! check for overflow\n"
"        be      2b;                     ! if so, inc again\n"
"        nop;                            ! but this means a delay, sigh\n"
"        st      %2, [%0];               ! Save new value into *pNum\n"
        : /* no output */
        : "r" (pNum), "r" (0x80000000), "r" (0x80000000)
        : "cc", "memory"
        );
}

/* Decrement by 1 */
inline void
HXAtomicDecUINT32(UINT32* pNum)
{
    __asm__ __volatile__(
"1:      swap    [%0], %2;               ! Swap *pNum and %2\n"
"        nop;                            ! delay slot...\n"
"        cmp     %2, %1;                 ! Is someone else using pNum?\n"
"        be      1b;                     ! If so, retry...\n"
"        nop;                            ! delay slot...yawn\n"
"2:      dec     %2;                     ! Increment %2\n"
"        cmp     %2, %1;                 ! check for overflow\n"
"        be      2b;                     ! if so, dec again\n"
"        nop;                            ! but this means a delay, sigh\n"
"        st      %2, [%0];               ! Save new value into *pNum\n"
        : /* no output */
        : "r" (pNum), "r" (0x80000000), "r" (0x80000000)
        : "cc", "memory"
        );
}

/* Increment by 1 and return new value */
inline UINT32
HXAtomicIncRetUINT32(UINT32* pNum)
{
    volatile UINT32 ulRet;
    __asm__ __volatile__(
"        mov     %2, %0;                 ! Copy %2 to %0 \n"
"1:      swap    [%1], %0;               ! Swap *pNum and %0\n"
"        nop;                            ! delay slot...\n"
"        cmp     %0, %2;                 ! Is someone else using pNum?\n"
"        be      1b;                     ! If so, retry...\n"
"        nop;                            ! delay slot...yawn\n"
"2:      inc     %0;                     ! Increment %0\n"
"        cmp     %0, %2;                 ! check for overflow\n"
"        be      2b;                     ! if so, inc again\n"
"        nop;                            ! but this means a delay, sigh\n"
"        st      %0, [%1];               ! Save new value into *pNum\n"
        : "=r" (ulRet)
        : "r" (pNum), "r" (0x80000000), "0" (ulRet)
        : "cc", "memory"
        );
    return ulRet;
}

/* Decrement by 1 and return new value */
inline UINT32
HXAtomicDecRetUINT32(UINT32* pNum)
{   volatile UINT32 ulRet;
    __asm__ __volatile__(
"        mov     %2, %0;                 ! Copy %2 to %0 \n"
"1:      swap    [%1], %0;               ! Swap *pNum and %0\n"
"        nop;                            ! delay slot...\n"
"        cmp     %0, %2;                 ! Is someone else using pNum?\n"
"        be      1b;                     ! If so, retry...\n"
"        nop;                            ! delay slot...yawn\n"
"2:      dec     %0;                     ! Decrement %0\n"
"        cmp     %0, %2;                 ! check for overflow\n"
"        be      2b;                     ! if so, dec again\n"
"        nop;                            ! but this means a delay, sigh\n"
"        st      %0, [%1];               ! Save new value into *pNum\n"
        : "=r" (ulRet)
        : "r" (pNum), "r" (0x80000000), "0" (ulRet)
        : "cc", "memory"
        );
    return ulRet;
}

/* Add n */
inline void
HXAtomicAddUINT32(UINT32* pNum, UINT32 ulNum)
{
    __asm__ __volatile__(
"1:      swap    [%0], %2;               ! Swap *pNum and %2\n"
"        nop;                            ! delay slot...\n"
"        cmp     %2, %1;                 ! Is someone else using pNum?\n"
"        be      1b;                     ! If so, retry...\n"
"        nop;                            ! delay slot...yawn\n"
"        add     %2, %3, %2;             ! Add ulNum to %2\n"
"        cmp     %2, %1;                 ! check for overflow\n"
"        bne     2f;                     ! if not, skip to the end\n"
"        nop;                            ! but this means a delay, sigh\n"
"        inc     %2;                     ! skip marker value\n"
"2:      st      %2, [%0];               ! Save new value into *pNum\n"
        : /* no output */
        : "r" (pNum), "r" (0x80000000), "r" (0x80000000), "r" (ulNum)
        : "cc", "memory"
        );
}

/* Subtract n */
inline void
HXAtomicSubUINT32(UINT32* pNum, UINT32 ulNum)
{
    __asm__ __volatile__(
"1:      swap    [%0], %2;               ! Swap *pNum and %2\n"
"        nop;                            ! delay slot...\n"
"        cmp     %2, %1;                 ! Is someone else using pNum?\n"
"        be      1b;                     ! If so, retry...\n"
"        nop;                            ! delay slot...yawn\n"
"        sub     %2, %3, %2;             ! Subtract ulNum to %2\n"
"        cmp     %2, %1;                 ! check for overflow\n"
"        bne     2f;                     ! if not, skip to the end\n"
"        nop;                            ! but this means a delay, sigh\n"
"        inc     %2;                     ! skip marker value\n"
"2:      st      %2, [%0];               ! Save new value into *pNum\n"
        : /* no output */
        : "r" (pNum), "r" (0x80000000), "r" (0x80000000), "r" (ulNum)
        : "cc", "memory"
        );
}

/* Add n and return new value */
inline UINT32
HXAtomicAddRetUINT32(UINT32* pNum, UINT32 ulNum)
{
    volatile UINT32 ulRet; \
    __asm__ __volatile__(
"        mov     %2, %0                  ! Copy %2 to %0 \n"
"1:      swap    [%1], %0;               ! Swap *pNum and %0\n"
"        nop;                            ! delay slot...\n"
"        cmp     %0, %2;                 ! Is someone else using pNum?\n"
"        be      1b;                     ! If so, retry...\n"
"        nop;                            ! delay slot...yawn\n"
"        add     %0, %3, %0;             ! Add ulNum to %0\n"
"        cmp     %0, %2;                 ! check for overflow\n"
"        bne     2f;                     ! if not, skip to the end\n"
"        nop;                            ! but this means a delay, sigh\n"
"        inc     %0;                     ! skip marker value\n"
"2:      st      %0, [%1];               ! Save new value into *pNum\n"
        : "=r" (ulRet)
        : "r" (pNum), "r" (0x80000000), "r" (ulNum), "0" (ulRet)
        : "cc", "memory"
        );
        return ulRet;
}

/* Subtract n and return new value */
inline UINT32
HXAtomicSubRetUINT32(UINT32* pNum, UINT32 ulNum)
{   volatile UINT32 ulRet;
    __asm__ __volatile__(
"        mov     %2, %0                  ! Copy %2 to %0 \n"
"1:      swap    [%1], %0;               ! Swap *pNum and %0\n"
"        nop;                            ! delay slot...\n"
"        cmp     %0, %2;                 ! Is someone else using pNum?\n"
"        be      1b;                     ! If so, retry...\n"
"        nop;                            ! delay slot...yawn\n"
"        sub     %0, %3, %0;             ! Sub ulNum from %0\n"
"        cmp     %0, %2;                 ! check for overflow\n"
"        bne     2f;                     ! if not, skip to the end\n"
"        nop;                            ! but this means a delay, sigh\n"
"        dec     %0;                     ! skip marker value\n"
"2:      st      %0, [%1];               ! Save new value into *pNum\n"
        : "=r" (ulRet)
        : "r" (pNum), "r" (0x80000000), "r" (ulNum), "0" (ulRet)
        : "cc", "memory"
        );
        return ulRet;
}

inline void HXAtomicIncINT32(INT32* p)              { HXAtomicIncUINT32((UINT32*)p); }
inline void HXAtomicDecINT32(INT32* p)              { HXAtomicDecUINT32((UINT32*)p); }
inline void HXAtomicAddINT32(INT32* p, INT32 n)     { HXAtomicAddUINT32((UINT32*)p, (UINT32)n); }
inline void HXAtomicSubINT32(INT32* p, INT32 n)     { HXAtomicSubUINT32((UINT32*)p, (UINT32)n); }
inline INT32 HXAtomicIncRetINT32(INT32* p)          { return HXAtomicIncRetUINT32((UINT32*)p); }
inline INT32 HXAtomicDecRetINT32(INT32* p)          { return HXAtomicDecRetUINT32((UINT32*)p); }
inline INT32 HXAtomicAddRetINT32(INT32* p, INT32 n) { return HXAtomicAddRetUINT32((UINT32*)p, (UINT32)n); }
inline INT32 HXAtomicSubRetINT32(INT32* p, INT32 n) { return HXAtomicSubRetUINT32((UINT32*)p, (UINT32)n); }



/***********************************************************************
 * Windows / x86 (Visual C/C++)
 *
 * Implementation Notes:
 *   'xadd' is only available in the 486 series and later, not the 386.
 *   There is no 'xsub' counterpart, you have to negate the operand
 *   and use 'xadd'.  Note the use of the 'lock' prefix to ensure
 *   certain operations occur atomically.
 */
#elif defined (_M_IX86) /* && _M_IX86 > 300 XXX wschildbach: disabled until the build system delivers the correct value */

/* Increment by 1 */
static __inline void
HXAtomicIncUINT32(UINT32* pNum)
{
        // register usage summary:
        //   eax - pointer to the value we're modifying
    _asm
    {
             mov  eax, pNum              ; Load the pointer into a register
        lock inc  dword ptr [eax]        ; Atomically increment *pNum
    }
}

/* Decrement by 1 */
static __inline void
HXAtomicDecUINT32(UINT32* pNum)
{
        // register usage summary:
        //   eax - pointer to the value we're modifying
    _asm
    {
             mov  eax,  pNum             ; Load the pointer into a register
        lock dec  dword ptr [eax]        ; Atomically decrement *pNum
    }
}

/* Increment by 1 and return new value */
static __inline UINT32
HXAtomicIncRetUINT32(UINT32* pNum)
{
    volatile UINT32 ulRet;     
        // register usage summary:
        //   eax - pointer to the value we're modifying
        //   ebx - work register
    _asm
    {
             mov  eax, pNum              ; Load the pointer into a register
             mov  ebx, 0x1               ; Load increment amount into a register
        lock xadd dword ptr [eax], ebx   ; Increment *pNum; ebx gets old value
             inc  ebx                    ; Increment old value
             mov  ulRet, ebx             ; Set the return value
    }
    return ulRet;
}

/* Decrement by 1 and return new value */
static __inline UINT32
HXAtomicDecRetUINT32(UINT32* pNum)
{   
    volatile UINT32 ulRet;
        // register usage summary:
        //   eax - pointer to the value we're modifying
        //   ebx - work register
        // note: we increment by 0xffffffff to decrement by 1
    _asm
    {
             mov  eax, pNum              ; Load the pointer into a register
             mov  ebx, 0xffffffff        ; Load decrement amount into a register
        lock xadd dword ptr [eax], ebx   ; Decrement *pNum; ebx gets old value
             dec  ebx                    ; decrement old value
             mov  ulRet, ebx             ; Set the return value
    }
    return ulRet;
}

/* Add n */
static __inline void
HXAtomicAddUINT32(UINT32* pNum, UINT32 ulNum)
{
        // register usage summary:
        //   eax - pointer to the value we're modifying
        //   ebx - work register
    _asm
    {
             mov  eax, pNum              ; Load the pointer into a register
             mov  ebx, ulNum             ; Load increment amount into a register
        lock add  dword ptr [eax], ebx   ; Increment *pNum by ulNum
    }
}

/* Subtract n */
static __inline void
HXAtomicSubUINT32(UINT32* pNum, UINT32 ulNum)
{
        // register usage summary:
        //   eax - pointer to the value we're modifying
        //   ebx - work register
    _asm
    {
             mov  eax, pNum              ; Load the pointer into a register
             mov  ebx, ulNum             ; Load increment amount into a register
        lock sub  dword ptr [eax], ebx   ; Atomically decrement *pNum by ulNum
    }
}

/* Add n and return new value */
static __inline UINT32
HXAtomicAddRetUINT32(UINT32* pNum, UINT32 ulNum)
{
    volatile UINT32 ulRet;
        // register usage summary:
        //   eax - pointer to the value we're modifying
        //   ebx - work register
        //   ecx - work register #2
    _asm
    {
             mov  eax, pNum              ; Load the pointer into a register
             mov  ebx, ulNum             ; Load increment amount into a register
             mov  ecx, ebx               ; copy ebx into ecx
        lock xadd dword ptr [eax], ecx   ; Increment *pNum; ecx gets old value
             add  ecx, ebx               ; Add ulNum to it
             mov  ulRet, ecx             ; save result in ulRet
    }
    return ulRet;
}

/* Subtract n and return new value */
static __inline UINT32
HXAtomicSubRetUINT32(UINT32* pNum, UINT32 ulNum) 
{   
    volatile UINT32 ulRet;
        // register usage summary:
        //   eax - pointer to the value we're modifying
        //   ebx - work register
        //   ecx - work register #2
    _asm
    {
             mov  eax, pNum              ; Load the pointer into a register
             mov  ebx, ulNum             ; Load increment amount into a register
             mov  ecx, 0x0               ; zero out ecx
             sub  ecx, ebx               ; compute -(ulNum), saving in ecx
        lock xadd dword ptr [eax], ecx   ; Decrement *pNum; ecx gets old value
             sub  ecx, ebx               ; subtract ulNum from it
             mov  ulRet, ecx             ; save result in ulRet
    }
    return ulRet;
}

static __inline void HXAtomicIncINT32(INT32* p)              { HXAtomicIncUINT32((UINT32*)p); }
static __inline void HXAtomicDecINT32(INT32* p)              { HXAtomicDecUINT32((UINT32*)p); }
static __inline void HXAtomicAddINT32(INT32* p, INT32 n)     { HXAtomicAddUINT32((UINT32*)p, (UINT32)n); }
static __inline void HXAtomicSubINT32(INT32* p, INT32 n)     { HXAtomicSubUINT32((UINT32*)p, (UINT32)n); }
static __inline INT32 HXAtomicIncRetINT32(INT32* p)          { return HXAtomicIncRetUINT32((UINT32*)p); }
static __inline INT32 HXAtomicDecRetINT32(INT32* p)          { return HXAtomicDecRetUINT32((UINT32*)p); }
static __inline INT32 HXAtomicAddRetINT32(INT32* p, INT32 n) { return HXAtomicAddRetUINT32((UINT32*)p, (UINT32)n); }
static __inline INT32 HXAtomicSubRetINT32(INT32* p, INT32 n) { return HXAtomicSubRetUINT32((UINT32*)p, (UINT32)n); }



/***********************************************************************
 * Intel x86 (gcc) / Unix  -- i486 and higher - 32-bit
 *
 * Implementation Notes:
 *   'xadd' is only available in the 486 series and later, not the 386.
 *   There is no 'xsub' counterpart, you have to negate the operand
 *   and use 'xadd'.  Note the use of the 'lock' prefix to ensure
 *   certain operations occur atomically.
 *
 *   OpenBSD is excluded since the standard assembler on x86 systems
 *   can't handle the xadd instruction.
 *
 */
#elif defined(__GNUC__) && !defined(_OPENBSD) && \
      (__GNUC__>2 || (__GNUC__==2 && __GNUC_MINOR__>=95)) && \
      ( defined (__i486__) || defined (__i586__) || defined (__i686__) || \
        defined (__pentium__) || defined (__pentiumpro__))

/* Increment by 1 */
static __inline__ void
HXAtomicIncUINT32(UINT32* pNum)
{
    __asm__ __volatile__(
        "lock incl (%0);"                // atomically add 1 to *pNum
        : /* no output */
        : "r" (pNum)
        : "cc", "memory"
        );
}

/* Decrement by 1 */
static __inline__ void
HXAtomicDecUINT32(UINT32* pNum)
{
    __asm__ __volatile__(
        "lock decl (%0);"                // atomically add -1 to *pNum
        : /* no output */
        : "r" (pNum)
        : "cc", "memory"
        );
}

/* Increment by 1 and return new value */
static __inline__ UINT32
HXAtomicIncRetUINT32(UINT32* pNum)
{
    volatile UINT32 ulRet;
    __asm__ __volatile__(
        "lock xaddl %0, (%1);"           // atomically add 1 to *pNum
        "     inc   %0;"                 // old value in %0, increment it
        : "=r" (ulRet)
        : "r" (pNum), "0" (0x1)
        : "cc", "memory"
        );
    return ulRet;
}

/* Decrement by 1 and return new value */
static __inline__ UINT32
HXAtomicDecRetUINT32(UINT32* pNum)
{   
    volatile UINT32 ulRet;
    __asm__ __volatile__(
        "lock xaddl %0, (%1);"           // atomically add -1 to *pNum
        "     dec   %0;"                 // old value in %0, decrement it
        : "=r" (ulRet)
        : "r" (pNum), "0" (-1)
        : "cc", "memory"
        );
    return ulRet;
}

/* Add n */
static __inline__ void
HXAtomicAddUINT32(UINT32* pNum, UINT32 ulNum)
{
    __asm__ __volatile__(
        "lock addl %1, (%0);"            // atomically add ulNum to *pNum
        : /* no output */
        : "r" (pNum), "r" (ulNum)
        : "cc", "memory"
        );
}

/* Subtract n */
static __inline__ void
HXAtomicSubUINT32(UINT32* pNum, UINT32 ulNum)
{
    __asm__ __volatile__(
        "lock subl %1, (%0);"            // atomically add ulNum to *pNum
        : /* no output */
        : "r" (pNum), "r" (ulNum)
        : "cc", "memory"
        );
}

/* Add n and return new value */
static __inline__ UINT32
HXAtomicAddRetUINT32(UINT32* pNum, UINT32 ulNum)
{
    volatile UINT32 ulRet;
    __asm__ __volatile__(
        "     mov   %2, %0;"             // copy ulNum into %0
        "lock xaddl %0, (%1);"           // atomically add ulNum to *pNum
        "     add   %2, %0;"             // old value in %0, add ulNum
        : "=r" (ulRet)
        : "r" (pNum), "r" (ulNum), "0" (0)
        : "cc", "memory"
        );
    return ulRet;
}

/* Subtract n and return new value */
static __inline__ UINT32
HXAtomicSubRetUINT32(UINT32* pNum, UINT32 ulNum) 
{   
    volatile UINT32 ulRet;
    __asm__ __volatile__(
        "     sub   %2, %0;"             // negate ulNum, saving in %0
        "lock xaddl %0, (%1);"           // atomically add -(ulNum) to *pNum
        "     sub   %2, %0;"             // old value in %0, subtract ulNum
        : "=r" (ulRet)
        : "r" (pNum), "r" (ulNum), "0" (0)
        : "cc", "memory"
        );
    return ulRet;
}


static __inline__ void HXAtomicIncINT32(INT32* p)              { HXAtomicIncUINT32((UINT32*)p); }
static __inline__ void HXAtomicDecINT32(INT32* p)              { HXAtomicDecUINT32((UINT32*)p); }
static __inline__ void HXAtomicAddINT32(INT32* p, INT32 n)     { HXAtomicAddUINT32((UINT32*)p, (UINT32)n); }
static __inline__ void HXAtomicSubINT32(INT32* p, INT32 n)     { HXAtomicSubUINT32((UINT32*)p, (UINT32)n); }
static __inline__ INT32 HXAtomicIncRetINT32(INT32* p)          { return HXAtomicIncRetUINT32((UINT32*)p); }
static __inline__ INT32 HXAtomicDecRetINT32(INT32* p)          { return HXAtomicDecRetUINT32((UINT32*)p); }
static __inline__ INT32 HXAtomicAddRetINT32(INT32* p, INT32 n) { return HXAtomicAddRetUINT32((UINT32*)p, (UINT32)n); }
static __inline__ INT32 HXAtomicSubRetINT32(INT32* p, INT32 n) { return HXAtomicSubRetUINT32((UINT32*)p, (UINT32)n); }



/***********************************************************************
 * Intel x86/amd64/x86_64 (gcc) / Unix  -- 64-bit
 *
 * Implementation Notes:
 *
 */
#elif defined(__GNUC__) && (defined (__amd64__) || defined (__x86_64__))

/* Increment by 1 */
static __inline__ void
HXAtomicIncUINT32(UINT32* pNum)
{
    __asm__ __volatile__(
        "lock incl (%%rax);"             // atomically add 1 to *pNum
        : /* no output */
        : "a" (pNum)
        : "cc", "memory"
        );
}

/* Decrement by 1 */
static __inline__ void
HXAtomicDecUINT32(UINT32* pNum)
{
    __asm__ __volatile__(
        "lock decl (%%rax);"             // atomically add -1 to *pNum
        : /* no output */
        : "a" (pNum)
        : "cc", "memory"
        );
}

/* Increment by 1 and return new value */
static __inline__ UINT32
HXAtomicIncRetUINT32(UINT32* pNum)
{
    volatile UINT32 ulRet;
    __asm__ __volatile__(
        "lock xaddl %%ebx, (%%rax);"     // atomically add 1 to *pNum
        "     incl  %%ebx;"              // old value in %%ebx, increment it
        : "=b" (ulRet)
        : "a" (pNum), "b" (0x1)
        : "cc", "memory"
        );
    return ulRet;
}

/* Decrement by 1 and return new value */
static __inline__ UINT32
HXAtomicDecRetUINT32(UINT32* pNum)
{   
    volatile UINT32 ulRet;
    __asm__ __volatile__(
        "lock xaddl %%ebx, (%%rax);"     // atomically add -1 to *pNum
        "     decl  %%ebx;"              // old value in %%ebx, decrement it
        : "=b" (ulRet)
        : "a" (pNum), "b" (-1)
        : "cc", "memory"
        );
    return ulRet;
}

/* Add n */
static __inline__ void
HXAtomicAddUINT32(UINT32* pNum, UINT32 ulNum)
{
    __asm__ __volatile__(
        "lock addl %%ebx, (%%rax);"      // atomically add ulNum to *pNum
        : /* no output */
        : "a" (pNum), "b" (ulNum)
        : "cc", "memory"
        );
}

/* Subtract n */
static __inline__ void
HXAtomicSubUINT32(UINT32* pNum, UINT32 ulNum)
{
    __asm__ __volatile__(
        "lock subl %%ebx, (%%rax);"      // atomically add ulNum to *pNum
        : /* no output */
        : "a" (pNum), "b" (ulNum)
        : "cc", "memory"
        );
}

/* Add n and return new value */
static __inline__ UINT32
HXAtomicAddRetUINT32(UINT32* pNum, UINT32 ulNum)
{
    volatile UINT32 ulRet;
    __asm__ __volatile__(
        "     movl  %%ebx, %%ecx;"       // copy ulNum into %0
        "lock xaddl %%ecx, (%%rax);"     // atomically add ulNum to *pNum
        "     addl  %%ebx, %%ecx;"       // old value in %%ecx, add ulNum
        : "=c" (ulRet)
        : "a" (pNum), "b" (ulNum), "c" (0)
        : "cc", "memory"
        );
    return ulRet;
}

/* Subtract n and return new value */
static __inline__ UINT32
HXAtomicSubRetUINT32(UINT32* pNum, UINT32 ulNum) 
{   
    volatile UINT32 ulRet;
    __asm__ __volatile__(
        "     subl  %%ebx, %%ecx;"       // negate ulNum, saving in %0
        "lock xaddl %%ecx, (%%rax);"     // atomically add -(ulNum) to *pNum
        "     subl  %%ebx, %%ecx;"       // old value in %%ecx, subtract ulNum
        : "=c" (ulRet)
        : "a" (pNum), "b" (ulNum), "c" (0)
        : "cc", "memory"
        );
    return ulRet;
}


static __inline__ void HXAtomicIncINT32(INT32* p)              { HXAtomicIncUINT32((UINT32*)p); }
static __inline__ void HXAtomicDecINT32(INT32* p)              { HXAtomicDecUINT32((UINT32*)p); }
static __inline__ void HXAtomicAddINT32(INT32* p, INT32 n)     { HXAtomicAddUINT32((UINT32*)p, (UINT32)n); }
static __inline__ void HXAtomicSubINT32(INT32* p, INT32 n)     { HXAtomicSubUINT32((UINT32*)p, (UINT32)n); }
static __inline__ INT32 HXAtomicIncRetINT32(INT32* p)          { return HXAtomicIncRetUINT32((UINT32*)p); }
static __inline__ INT32 HXAtomicDecRetINT32(INT32* p)          { return HXAtomicDecRetUINT32((UINT32*)p); }
static __inline__ INT32 HXAtomicAddRetINT32(INT32* p, INT32 n) { return HXAtomicAddRetUINT32((UINT32*)p, (UINT32)n); }
static __inline__ INT32 HXAtomicSubRetINT32(INT32* p, INT32 n) { return HXAtomicSubRetUINT32((UINT32*)p, (UINT32)n); }



/***********************************************************************
 * HP-UX / IA64 (Native compiler)
 *
 * Implementation Notes:
 *      A work-in-progress...
 */
#elif defined(_HPUX) && defined(_IA64)

#if defined(__cplusplus)
extern "C" {
#endif
    UINT32 _HXAtomicIncRetUINT32 (UINT32* pNum);
    UINT32 _HXAtomicDecRetUINT32 (UINT32* pNum);
    UINT32 _HXAtomicAddRetUINT32 (UINT32* pNum, UINT32 ulNum);
    UINT32 _HXAtomicSubRetUINT32 (UINT32* pNum, UINT32 ulNum);
#if defined(__cplusplus)
}
#endif

#define HXAtomicIncINT32(p)       _HXAtomicIncRetUINT32((UINT32*)(p))
#define HXAtomicDecINT32(p)       _HXAtomicDecRetUINT32((UINT32*)(p))
#define HXAtomicIncRetINT32(p)    _HXAtomicIncRetUINT32((UINT32*)(p))
#define HXAtomicDecRetINT32(p)    _HXAtomicDecRetUINT32((UINT32*)(p))
#define HXAtomicAddINT32(p,n)     _HXAtomicAddRetUINT32((UINT32*)(p),(INT32)(n))
#define HXAtomicSubINT32(p,n)     _HXAtomicSubRetUINT32((UINT32*)(p),(INT32)(n))
#define HXAtomicAddRetINT32(p,n)  _HXAtomicAddRetUINT32((UINT32*)(p),(INT32)(n))
#define HXAtomicSubRetINT32(p,n)  _HXAtomicSubRetUINT32((UINT32*)(p),(INT32)(n))

#define HXAtomicIncUINT32(p)      _HXAtomicIncRetUINT32((p))
#define HXAtomicDecUINT32(p)      _HXAtomicDecRetUINT32((p))
#define HXAtomicIncRetUINT32(p)   _HXAtomicIncRetUINT32((p))
#define HXAtomicDecRetUINT32(p)   _HXAtomicDecRetUINT32((p))
#define HXAtomicAddUINT32(p,n)    _HXAtomicAddRetUINT32((p),(n))
#define HXAtomicSubUINT32(p,n)    _HXAtomicSubRetUINT32((p),(n))
#define HXAtomicAddRetUINT32(p,n) _HXAtomicAddRetUINT32((p),(n))
#define HXAtomicSubRetUINT32(p,n) _HXAtomicSubRetUINT32((p),(n))



/***********************************************************************
 * Tru64 (OSF1) / Alpha (Native compiler)
 *
 * Implementation Notes:
 *
 * The Alpha CPU provides instructions to load-lock a value,
 * modify it, and attempt to write it back.  If the value has
 * been modified by someone else since the load-lock occured,
 * the write will fail and you can check the status code to
 * know whether you need to retry or not.
 *
 */
#elif defined (__alpha)

#include <c_asm.h>

/* Increment by 1 and return new value */
inline INT32
HXAtomicIncRetINT32(INT32* pNum)
{
    return asm (
        "10:     ldl_l   %t0, (%a0);"       // Load-lock value into a register
        "        addl    %t0, 1, %t0;"      // Increment value
        "        or      %t0, %zero, %v0;"  // set new value for return.
        "        stl_c   %t0, (%a0);"       // Save new value into *pNum
        "        beq     %t0, 10b;"         // Retry if sequence failed
        , pNum);
}

/* Decrement by 1 and return new value */
inline INT32
HXAtomicDecRetINT32(INT32* pNum)
{
    return asm (
        "10:     ldl_l   %t0, (%a0);"       // Load-lock value into a register
        "        subl    %t0, 1, %t0;"      // Decrement value
        "        or      %t0, %zero, %v0;"  // set new value for return.
        "        stl_c   %t0, (%a0);"       // Save new value into *pNum
        "        beq     %t0, 10b;"         // Retry if sequence failed
        , pNum);
}

/* Add n and return new value */
inline INT32
HXAtomicAddRetINT32(INT32* pNum, INT32 n)
{
    return asm (
        "10:     ldl_l   %t0, (%a0);"       // Load-lock value into a register
        "        addl    %t0, %a1, %t0;"    // Add n to value
        "        or      %t0, %zero, %v0;"  // set new value for return.
        "        stl_c   %t0, (%a0);"       // Save new value into *pNum
        "        beq     %t0, 10b;"         // Retry if sequence failed
        , pNum, n);
}

/* Subtract n and return new value */
inline INT32
HXAtomicSubRetINT32(INT32* pNum, INT32 n)
{
    return asm (
        "10:     ldl_l   %t0, (%a0);"       // Load-lock value into a register
        "        subl    %t0, %a1, %t0;"    // Subtract n from value
        "        or      %t0, %zero, %v0;"  // set new value for return.
        "        stl_c   %t0, (%a0);"       // Save new value into *pNum
        "        beq     %t0, 10b;"         // Retry if sequence failed
        , pNum, n);
}

/* Increment by 1 and return new value */
inline UINT32
HXAtomicIncRetUINT32(UINT32* pNum)
{
    return asm (
        "10:     ldl_l   %t0, (%a0);"       // Load-lock value into a register
        "        addl    %t0, 1, %t0;"      // Increment value
        "        or      %t0, %zero, %v0;"  // set new value for return.
        "        stl_c   %t0, (%a0);"       // Save new value into *pNum
        "        beq     %t0, 10b;"         // Retry if sequence failed
        , pNum);
}

/* Decrement by 1 and return new value */
inline UINT32
HXAtomicDecRetUINT32(UINT32* pNum)
{
    return asm (
        "10:     ldl_l   %t0, (%a0);"       // Load-lock value into a register
        "        subl    %t0, 1, %t0;"      // Decrement value
        "        or      %t0, %zero, %v0;"  // set new value for return.
        "        stl_c   %t0, (%a0);"       // Save new value into *pNum
        "        beq     %t0, 10b;"         // Retry if sequence failed
        , pNum);
}

/* Add n and return new value */
inline UINT32
HXAtomicAddRetUINT32(UINT32* pNum, UINT32 n)
{
    return asm (
        "10:     ldl_l   %t0, (%a0);"       // Load-lock value into a register
        "        addl    %t0, %a1, %t0;"    // Add n to value
        "        or      %t0, %zero, %v0;"  // set new value for return.
        "        stl_c   %t0, (%a0);"       // Save new value into *pNum
        "        beq     %t0, 10b;"         // Retry if sequence failed
        , pNum, n);
}

/* Subtract n and return new value */
inline UINT32
HXAtomicSubRetUINT32(UINT32* pNum, UINT32 n)
{
    return asm (
        "10:     ldl_l   %t0, (%a0);"       // Load-lock value into a register
        "        subl    %t0, %a1, %t0;"    // Subtract n from value
        "        or      %t0, %zero, %v0;"  // set new value for return.
        "        stl_c   %t0, (%a0);"       // Save new value into *pNum
        "        beq     %t0, 10b;"         // Retry if sequence failed
        , pNum, n);
}

#define HXAtomicIncINT32(p)    HXAtomicIncRetINT32((p))
#define HXAtomicDecINT32(p)    HXAtomicDecRetINT32((p))
#define HXAtomicAddINT32(p,n)  HXAtomicAddRetINT32((p),(n))
#define HXAtomicSubINT32(p,n)  HXAtomicSubRetINT32((p),(n))

#define HXAtomicIncUINT32(p)   HXAtomicIncRetUINT32((p))
#define HXAtomicDecUINT32(p)   HXAtomicDecRetUINT32((p))
#define HXAtomicAddUINT32(p,n) HXAtomicAddRetUINT32((p),(n))
#define HXAtomicSubUINT32(p,n) HXAtomicSubRetUINT32((p),(n))



/***********************************************************************
 * AIX / PowerPC (Native compiler)
 *
 * Implementation Notes:
 *
 * XXXDC: The xlc compiler is able to do inline asm for C but when I do
 * it for C++ it crashes, so for now I have resorted to putting
 * the asm in a seperate assembler routine.  The way you inline with
 * xlc/xlC is difficult to use, requiring the use of "#pragma mc_func".
 */
#elif defined (_AIX)

//defined in common/util/platform/aix/atomicops.s
#if defined(__cplusplus)
extern "C" {
#endif
    INT32 _HXAtomicAddRetINT32   (INT32*  pNum, INT32  lNum);
    INT32 _HXAtomicSubRetINT32   (INT32*  pNum, INT32  lNum);
    UINT32 _HXAtomicAddRetUINT32 (UINT32* pNum, UINT32 ulNum);
    UINT32 _HXAtomicSubRetUINT32 (UINT32* pNum, UINT32 ulNum);
#if defined(__cplusplus)
}
#endif

#define HXAtomicIncINT32(p)       _HXAtomicAddRetINT32((p),(INT32)1)
#define HXAtomicDecINT32(p)       _HXAtomicSubRetINT32((p),(INT32)1)
#define HXAtomicIncRetINT32(p)    _HXAtomicAddRetINT32((p),(INT32)1)
#define HXAtomicDecRetINT32(p)    _HXAtomicSubRetINT32((p),(INT32)1)
#define HXAtomicAddINT32(p,n)     _HXAtomicAddRetINT32((p),(n))
#define HXAtomicSubINT32(p,n)     _HXAtomicSubRetINT32((p),(n))
#define HXAtomicAddRetINT32(p,n)  _HXAtomicAddRetINT32((p),(n))
#define HXAtomicSubRetINT32(p,n)  _HXAtomicSubRetINT32((p),(n))

#define HXAtomicIncUINT32(p)      _HXAtomicAddRetUINT32((p),(UINT32)1)
#define HXAtomicDecUINT32(p)      _HXAtomicSubRetUINT32((p),(UINT32)1)
#define HXAtomicIncRetUINT32(p)   _HXAtomicAddRetUINT32((p),(UINT32)1)
#define HXAtomicDecRetUINT32(p)   _HXAtomicSubRetUINT32((p),(UINT32)1)
#define HXAtomicAddUINT32(p,n)    _HXAtomicAddRetUINT32((p),(n))
#define HXAtomicSubUINT32(p,n)    _HXAtomicSubRetUINT32((p),(n))
#define HXAtomicAddRetUINT32(p,n) _HXAtomicAddRetUINT32((p),(n))
#define HXAtomicSubRetUINT32(p,n) _HXAtomicSubRetUINT32((p),(n))


/***********************************************************************
 * MAC / PowerPC (CW)
 *
 * Implementation Notes:
 *
 * This will need to be rewritten, probably, once we move away from CW to PB.
 *
 * Note: This is an imcompletely-defined platform, be aware that
 * not all standard HXAtomic operators are defined!
 *
 */
#elif defined(_MACINTOSH) && defined(__MWERKS__)

inline UINT32
HXAtomicIncRetUINT32(register UINT32* pNum)
{
    register UINT32 zeroOffset = 0;
    register UINT32 temp;

    asm
    {
	again:
	    lwarx temp, zeroOffset, pNum
	    addi temp, temp, 1
	    stwcx. temp, zeroOffset, pNum
	    bne- again
    }

    return temp;
}

inline UINT32
HXAtomicDecRetUINT32(register UINT32* pNum)
{
    register UINT32 zeroOffset = 0;
    register UINT32 temp;

    asm
    {
	again:
	    lwarx temp, zeroOffset, pNum
	    subi temp, temp, 1
	    stwcx. temp, zeroOffset, pNum
	    bne- again
    }

    return temp;
}


/***********************************************************************
 * MAC - PowerPC (PB or XCode) / Linux - PowerPC
 *
 * Implementation Notes:
 *
 * Use PowerPC load exclusive and store exclusive instructions
 *
 */
#elif defined(_MAC_UNIX) || (defined(_LINUX) && defined(__powerpc__))

// could also probably be defined(__GNUC__) && defined(__powerpc)

static inline UINT32
HXAtomicIncRetUINT32(UINT32* pNum)
{
    volatile UINT32 result;

	__asm__ __volatile__ (
"1:      lwarx  %0, %3, %2;\n"
"        addi   %0, %0, 1;\n"
"        stwcx. %0, %3, %2;\n"
"        bne- 1b;"
         : "=b" (result)
         : "0" (result), "b" (pNum), "b" (0x0)
         : "cc", "memory"
		 );
	
	return result;
}

static inline UINT32
HXAtomicDecRetUINT32(UINT32* pNum)
{
    volatile UINT32 result;

	__asm__ __volatile__ (
"1:      lwarx  %0, %3, %2;\n"
"        subi   %0, %0, 1;\n"
"        stwcx. %0, %3, %2;\n"
"        bne- 1b;"
         : "=b" (result)
         : "0" (result), "b" (pNum), "b" (0x0)
         : "cc", "memory"
         );
	
	return result;
}


static inline UINT32
HXAtomicAddRetUINT32(UINT32* pNum, UINT32 ulNum)
{
    volatile UINT32 result;

	__asm__ __volatile__ (
"1:      lwarx  %0, %3, %2;\n"
"        add    %0, %0, %4;\n"
"        stwcx. %0, %3, %2;\n"
"        bne- 1b;"
         : "=b" (result)
         : "0" (result), "b" (pNum), "b" (0x0), "b" (ulNum)
         : "cc", "memory"
		 );
	
	return result;
}


static inline UINT32
HXAtomicSubRetUINT32(UINT32* pNum, UINT32 ulNum)
{
    volatile UINT32 result;

	__asm__ __volatile__ (
"1:      lwarx  %0, %3, %2;\n"
"        sub    %0, %0, %4;\n"
"        stwcx. %0, %3, %2;\n"
"        bne- 1b;"
         : "=b" (result)
         : "0" (result), "b" (pNum), "b" (0x0), "b" (ulNum)
         : "cc", "memory"
         );
	
	return result;
}

// the rest of these atomic operations can be implemented in terms of the four above.

static inline void HXAtomicIncINT32(INT32* p) { (void)HXAtomicIncRetUINT32((UINT32*)p); }
static inline void HXAtomicDecINT32(INT32* p) { (void)HXAtomicDecRetUINT32((UINT32*)p); }
static inline void HXAtomicAddINT32(INT32* p, INT32 n) { (void)HXAtomicAddRetUINT32((UINT32*)p, (UINT32)n); }
static inline void HXAtomicSubINT32(INT32* p, INT32 n) { (void)HXAtomicSubRetUINT32((UINT32*)p, (UINT32)n); }
static inline INT32 HXAtomicIncRetINT32(INT32* p) { return (INT32)HXAtomicIncRetUINT32((UINT32*)p); }
static inline INT32 HXAtomicDecRetINT32(INT32* p) { return (INT32)HXAtomicDecRetUINT32((UINT32*)p); }
static inline INT32 HXAtomicAddRetINT32(INT32* p, INT32 n) { return (INT32)HXAtomicAddRetUINT32((UINT32*)p, (UINT32)n); }
static inline INT32 HXAtomicSubRetINT32(INT32* p, INT32 n) { return (INT32)HXAtomicSubRetUINT32((UINT32*)p, (UINT32)n); }
static inline void HXAtomicIncUINT32(UINT32* p) { (void)HXAtomicIncRetUINT32(p); }
static inline void HXAtomicDecUINT32(UINT32* p) { (void)HXAtomicDecRetUINT32(p); }
static inline void HXAtomicAddUINT32(UINT32* p, UINT32 n) { (void)HXAtomicAddRetUINT32(p, n); }
static inline void HXAtomicSubUINT32(UINT32* p, UINT32 n) { (void)HXAtomicSubRetUINT32(p, n); }


/***********************************************************************
 * Generic
 *
 * Implementation Notes:
 *
 * This should work on any platform with a HXMutex-style mutex.
 * It allocates a pool of mutexes and hashes the int pointers
 * to one of the mutexes.  Since the mutexes are held for
 * such a short time, only long enough to increment an int,
 * collisions should be extremely rare and this should work fine,
 * although it is probably less fast than the extra-high-performance
 * atomic operators provided above.  You need to link in atomic.cpp
 * to get HXAtomic::m_pLocks defined.
 *
 * Basic design of the mutex-based lock-pool implementation:
 *   At startup, allocate an array of N mutexes (where N is a power of 2).
 *   When a method is called, hash the int pointer to one of the locks.
 *   Lock this mutex.
 *   Modify the value.
 *   Unlock this mutex.
 *
 *
 * Platform-specific notes:
 *   Any platforms that use this should be documented here!
 *   Why are you using the generic operators for this platform?
 *
 * HP-UX / HP-PA:
 *   This is used on the HP-PA processor since it doesn't provide the
 *   necessary assembler operators to implement proper atomic updates
 *   of ints.  HP's mutex primitive seems pretty fast however, resulting
 *   in a workable solution.
 *
 * OpenBSD:
 *   The standard assembler on x86 can't handle the gcc/asm operators
 *   defined above, so we're using the lock-pool approach for now.
 *   This approach also makes it possible to support non-x86 OpenBSD
 *   builds more easily (someday).
 *
 */
#elif defined(_HPUX) || defined(_OPENBSD)

#if defined(__cplusplus)
#include "microsleep.h"
#include "hxcom.h"
#include "hxmutexlock.h"

class HXAtomic
{
public:
    HXAtomic();
    ~HXAtomic();
    void InitLockPool();

    /* Users of the HXAtomic routines should *NEVER* call these directly.
     * They should *ALWAYS* use the HXAtomicAddRetINT32-style macros instead.
     */
    INT32  _AddRetINT32  (INT32*  pNum, INT32  nNum);
    UINT32 _AddRetUINT32 (UINT32* pNum, UINT32 ulNum);
    INT32  _SubRetINT32  (INT32*  pNum, INT32  nNum);
    UINT32 _SubRetUINT32 (UINT32* pNum, UINT32 ulNum);

private:
    void Lock   (HX_MUTEX pLock);
    void Unlock (HX_MUTEX pLock);

    HX_MUTEX* m_pLocks;
};

extern HXAtomic g_AtomicOps; //in common/util/atomicops.cpp

#define HXAtomicIncINT32(p)       g_AtomicOps._AddRetINT32((p),(INT32)1)
#define HXAtomicDecINT32(p)       g_AtomicOps._SubRetINT32((p),(INT32)1)
#define HXAtomicIncRetINT32(p)    g_AtomicOps._AddRetINT32((p),(INT32)1)
#define HXAtomicDecRetINT32(p)    g_AtomicOps._SubRetINT32((p),(INT32)1)

#define HXAtomicAddRetINT32(p,n)  g_AtomicOps._AddRetINT32((p),(n))
#define HXAtomicSubRetINT32(p,n)  g_AtomicOps._SubRetINT32((p),(n))
#define HXAtomicAddINT32(p,n)     g_AtomicOps._AddRetINT32((p),(n))
#define HXAtomicSubINT32(p,n)     g_AtomicOps._SubRetINT32((p),(n))

#define HXAtomicIncUINT32(p)      g_AtomicOps._AddRetUINT32((p),(UINT32)1)
#define HXAtomicDecUINT32(p)      g_AtomicOps._SubRetUINT32((p),(UINT32)1)
#define HXAtomicIncRetUINT32(p)   g_AtomicOps._AddRetUINT32((p),(UINT32)1)
#define HXAtomicDecRetUINT32(p)   g_AtomicOps._SubRetUINT32((p),(UINT32)1)

#define HXAtomicAddRetUINT32(p,n) g_AtomicOps._AddRetUINT32((p),(n))
#define HXAtomicSubRetUINT32(p,n) g_AtomicOps._SubRetUINT32((p),(n))
#define HXAtomicAddUINT32(p,n)    g_AtomicOps._AddRetUINT32((p),(n))
#define HXAtomicSubUINT32(p,n)    g_AtomicOps._SubRetUINT32((p),(n))
#endif



/***********************************************************************
 * SYMBIAN
 *
 * Implementation Notes:
 *
 * Note: This is an imcompletely-defined platform, be aware that
 * not all standard HXAtomic operators are defined!
 *
 */
#elif defined(_SYMBIAN)

/* Increment by 1 and return new value */
inline INT32
HXAtomicIncRetINT32(INT32* pNum)
{
    return User::LockedInc(*((TInt*)pNum)) + 1;
}

/* Decrement by 1 and return new value */
inline INT32
HXAtomicDecRetINT32(INT32* pNum)
{
    return User::LockedDec(*((TInt*)pNum)) - 1;
}

/* Increment by 1 and return new value */
inline UINT32
HXAtomicIncRetUINT32(UINT32* pNum)
{
    return ((UINT32)User::LockedInc(*((TInt*)pNum))) + 1;
}

/* Decrement by 1 and return new value */
inline UINT32
HXAtomicDecRetUINT32(UINT32* pNum)
{
    return ((UINT32)User::LockedDec(*((TInt*)pNum))) - 1;
}

#define HXAtomicIncINT32(p)    HXAtomicIncRetINT32((p))
#define HXAtomicDecINT32(p)    HXAtomicDecRetINT32((p))
#define HXAtomicIncUINT32(p)   HXAtomicIncRetUINT32((p))
#define HXAtomicDecUINT32(p)   HXAtomicDecRetUINT32((p))

#if 0

/* 
 * Add and subtract operations are not implemented
 * at this time because there isn't an easy way to
 * do it using the facilities provided by Symbian.
 * Assembly will likely be needed.
 */

/* Add n and return new value */
inline INT32
HXAtomicAddRetINT32(INT32* pNum, INT32 n)
{

}

/* Subtract n and return new value */
inline INT32
HXAtomicSubRetINT32(INT32* pNum, INT32 n)
{

}

/* Add n and return new value */
inline UINT32
HXAtomicAddRetUINT32(UINT32* pNum, UINT32 n)
{

}

/* Subtract n and return new value */
inline UINT32
HXAtomicSubRetUINT32(UINT32* pNum, UINT32 n)
{

}

#define HXAtomicAddINT32(p,n)  HXAtomicAddRetINT32((p),(n))
#define HXAtomicSubINT32(p,n)  HXAtomicSubRetINT32((p),(n))

#define HXAtomicAddUINT32(p,n) HXAtomicAddRetUINT32((p),(n))
#define HXAtomicSubUINT32(p,n) HXAtomicSubRetUINT32((p),(n))

#endif

/***********************************************************************
 * Linux / ARM (gcc)
 *
 * Implementation Notes:
 *
 * This implementation sacrifices being able to store the value
 * 0x800000000 in the INT32 value, which is a special "busy" marker value.
 * Since these are intended for use primarily with AddRef/Release and
 * resource usage counters, this should be acceptable for now.  If a counter
 * is incremented to the point it would conflict with the flag, it is
 * incremented one more to hop over it.  The same in reverse for decrement.
 *
 * Basic design of the flag-based implementation:
 *   1. Load a register with 0x80000000
 *   2. _atomically_ swap it with the INT32 (critical!)
 *   3. Compare what we got with 0x80000000
 *   4. Branch if equal to #2
 *   5. Increment (or decrement) the result
 *   6. Compare to 0x80000000
 *   7. Increment (or decrement) again if equal
 *   8. Save the new value to the INT32's location in memory
 *   9. Return new INT32 result if required
 *   
 */
#elif defined (_ARM) && defined (__GNUC__)

/* Increment by 1 */
inline void
HXAtomicIncUINT32(UINT32* pNum)
{
    UINT32 ulTmp;
    __asm__ __volatile__(
"        mov   %0, #0x80000000;\n"      /* Set ulTmp to 0x800000000    */
"1:      swp   %0, %0, [%1];\n"         /* Swap *pNum and ulTmp        */
"        cmp   %0, #0x80000000;\n"      /* Is someone else using pNum? */
"        beq   1;\n"                    /* If so, retry...             */
"        add   %0, %0, #1;\n"           /* Increment ulTmp             */
"        cmp   %0, #0x80000000;\n"      /* check for overflow          */
"        addeq %0, %0, #1;\n"           /* if so, increment again      */
"        str   %0, [%1];\n"             /* Save new value into *pNum   */
        : /* no output */
        : "r" (ulTmp), "r" (pNum)
        : "cc", "memory"
        );
}

/* Decrement by 1 */
inline void
HXAtomicDecUINT32(UINT32* pNum)
{
    UINT32 ulTmp;
    __asm__ __volatile__(
"        mov   %0, #0x80000000;\n"      /* Set ulTmp to 0x800000000    */
"1:      swp   %0, %0, [%1];\n"         /* Swap *pNum and ulTmp        */
"        cmp   %0, #0x80000000;\n"      /* Is someone else using pNum? */
"        beq   1;\n"                    /* If so, retry...             */
"        sub   %0, %0, #1;\n"           /* Decrement ulTmp             */
"        cmp   %0, #0x80000000;\n"      /* check for overflow          */
"        subeq %0, %0, #1;\n"           /* if so, decrement again      */
"        str   %0, [%1];\n"             /* Save new value into *pNum   */
        : /* no output */
        :  "r" (ulTmp), "r" (pNum)
        : "cc", "memory"
        );
}

/* Increment by 1 and return new value */
inline UINT32
HXAtomicIncRetUINT32(UINT32* pNum)
{
    volatile UINT32 ulRet;
    __asm__ __volatile__(
"        mov   %0, #0x80000000;\n"      /* Set ulRet to 0x80000000     */
"1:      swp   %0, %0, [%1];\n"         /* Swap *pNum and ulRet        */
"        cmp   %0, #0x80000000;\n"      /* Is someone else using pNum? */
"        beq   1;\n"                    /* If so, retry...             */
"        add   %0, %0, #1;\n"           /* Increment ulRet             */
"        cmp   %0, #0x80000000;\n"      /* check for overflow          */
"        addeq %0, %0, #1;\n"         /* if so, increment again      */
"        str   %0, [%1];\n"             /* Save new value into *pNum   */
        : "=&r" (ulRet)
        : "r" (pNum)
        : "cc", "memory"
        );
    return ulRet;
}

/* Decrement by 1 and return new value */
inline UINT32
HXAtomicDecRetUINT32(UINT32* pNum)
{
    volatile UINT32 ulRet;
    __asm__ __volatile__(
"        mov   %0, #0x80000000;\n"      /* Set ulRet to 0x80000000     */
"1:      swp   %0, %0, [%1];\n"         /* Swap *pNum and ulRet        */
"        cmp   %0, #0x80000000;\n"      /* Is someone else using pNum? */
"        beq   1;\n"                    /* If so, retry...             */
"        sub   %0, %0, #1;\n"           /* Decrement ulRet             */
"        cmp   %0, #0x80000000;\n"      /* check for overflow          */
"        subeq %0, %0, #1;\n"         /* if so, decrement again      */
"        str   %0, [%1];\n"             /* Save new value into *pNum   */
        : "=&r" (ulRet)
        : "r" (pNum)
        : "cc", "memory"
        );
    return ulRet;
}

/* Add n */
inline void
HXAtomicAddUINT32(UINT32* pNum, UINT32 ulNum)
{
    UINT32 ulTmp;
    __asm__ __volatile__(
"        mov   %0, #0x80000000;\n"      /* Set ulTmp to 0x800000000    */
"1:      swp   %0, %0, [%1];\n"         /* Swap *pNum and ulTmp        */
"        cmp   %0, #0x80000000;\n"      /* Is someone else using pNum? */
"        beq   1;\n"                    /* If so, retry...             */
"        add   %0, %0, %2;\n"           /* Add ulNum to ulTmp          */
"        cmp   %0, #0x80000000;\n"      /* check for overflow          */
"        addeq %0, %0, #1;\n"           /* if so, increment again      */
"        str   %0, [%1];\n"             /* Save new value into *pNum   */
        : /* no output */
        : "r" (ulTmp), "r" (pNum), "r" (ulNum)
        : "cc", "memory"
        );
}

/* Subtract n */
inline void
HXAtomicSubUINT32(UINT32* pNum, UINT32 ulNum)
{
    UINT32 ulTmp;
    __asm__ __volatile__(
"        mov   %0, #0x80000000;\n"      /* Set ulTmp to 0x800000000    */
"1:      swp   %0, %0, [%1];\n"         /* Swap *pNum and ulTmp        */
"        cmp   %0, #0x80000000;\n"      /* Is someone else using pNum? */
"        beq   1;\n"                    /* If so, retry...             */
"        sub   %0, %0, %2;\n"           /* Subtract ulNum from ulTmp   */
"        cmp   %0, #0x80000000;\n"      /* check for overflow          */
"        subeq %0, %0, #1;\n"           /* if so, decrement again      */
"        str   %0, [%1];\n"             /* Save new value into *pNum   */
        : /* no output */
        : "r" (ulTmp), "r" (pNum), "r" (ulNum)
        : "cc", "memory"
        );
}

/* Add n and return new value */
inline UINT32
HXAtomicAddRetUINT32(UINT32* pNum, UINT32 ulNum)
{
    volatile UINT32 ulRet;
    __asm__ __volatile__(
"        mov   %0, #0x80000000;\n"      /* Set ulRet to 0x80000000     */
"1:      swp   %0, %0, [%1];\n"         /* Swap *pNum and ulRet        */
"        cmp   %0, #0x80000000;\n"      /* Is someone else using pNum? */
"        beq   1;\n"                    /* If so, retry...             */
"        add   %0, %0, %2;\n"           /* Add ulNum to ulRet          */
"        cmp   %0, #0x80000000;\n"      /* check for overflow          */
"        addeq %0, %0, #1;\n"         /* if so, increment again      */
"        str   %0, [%1];\n"             /* Save new value into *pNum   */
        : "=&r" (ulRet)
        : "r" (pNum) , "r" (ulNum)
        : "cc", "memory"
        );
    return ulRet;
}

/* Subtract n and return new value */
inline UINT32
HXAtomicSubRetUINT32(UINT32* pNum, UINT32 ulNum)
{   
    volatile UINT32 ulRet;
    __asm__ __volatile__(
"        mov   %0, #0x80000000;\n"      /* Set ulRet to 0x80000000     */
"1:      swp   %0, %0, [%1];\n"         /* Swap *pNum and ulRet        */
"        cmp   %0, #0x80000000;\n"      /* Is someone else using pNum? */
"        beq   1;\n"                    /* If so, retry...             */
"        sub   %0, %0, %2;\n"           /* Subtract ulNum from ulRet   */
"        cmp   %0, #0x80000000;\n"      /* check for overflow          */
"        subeq %0, %0, #1;\n"         /* if so, decrement again      */
"        str   %0, [%1];\n"             /* Save new value into *pNum   */
        : "=&r" (ulRet)
        : "r" (pNum), "r" (ulNum)
        : "cc", "memory"
        );
    return ulRet;
}

inline void HXAtomicIncINT32(INT32* p)              { HXAtomicIncUINT32((UINT32*)p); }
inline void HXAtomicDecINT32(INT32* p)              { HXAtomicDecUINT32((UINT32*)p); }
inline void HXAtomicAddINT32(INT32* p, INT32 n)     { HXAtomicAddUINT32((UINT32*)p, (UINT32)n); }
inline void HXAtomicSubINT32(INT32* p, INT32 n)     { HXAtomicSubUINT32((UINT32*)p, (UINT32)n); }
inline INT32 HXAtomicIncRetINT32(INT32* p)          { return HXAtomicIncRetUINT32((UINT32*)p); }
inline INT32 HXAtomicDecRetINT32(INT32* p)          { return HXAtomicDecRetUINT32((UINT32*)p); }
inline INT32 HXAtomicAddRetINT32(INT32* p, INT32 n) { return HXAtomicAddRetUINT32((UINT32*)p, (UINT32)n); }
inline INT32 HXAtomicSubRetINT32(INT32* p, INT32 n) { return HXAtomicSubRetUINT32((UINT32*)p, (UINT32)n); }

/***********************************************************************
 * Add new platforms above here
 */
#else

//
// Unsupported platform
//

#ifndef HELIX_CONFIG_DISABLE_ATOMIC_OPERATORS
// Defining HELIX_CONFIG_DISABLE_ATOMIC_OPERATORS will use the ++ and --
// operators in place of atomic operators in some places in the code. These
// operators are not thread-safe, and should only be used in the intermediary
// stages of porting.
#  error "You need to create atomic dec/inc opers for your platform or #define HELIX_CONFIG_DISABLE_ATOMIC_OPERATORS"
#endif

#endif



/*************************************************************************/

/*
 * Conditional override of InterlockedIncrement/Decrement
 *
 * Place this in your Umakefil/.pcf file to turn off atomic
 * InterlockedIncrement/Decrement on a per-module basis,
 * or place it in your umake profile for system-wide scope.
 * If this is defined you'll still have access to the underlying
 * HXAtomicxxx operators (if they exist for your platform),
 * just that the specific InterlockedIncrement/InterlockedDecrement
 * macros won't be defined to use them.
 */
#if !defined (HELIX_CONFIG_DISABLE_ATOMIC_OPERATORS)

#undef InterlockedIncrement
#undef InterlockedDecrement

// Since many classes (incorrectly) implement their refcount using LONG32
// rather than the proper ULONG32, we have to use the typecast for things
// to build on many platforms.
#define InterlockedIncrement(p) HXAtomicIncRetUINT32((UINT32*)(p))
#define InterlockedDecrement(p) HXAtomicDecRetUINT32((UINT32*)(p))

#if !defined(HAVE_INTERLOCKED_INCREMENT)
#define HAVE_INTERLOCKED_INCREMENT //so hxcom.h doesn't redefine these to ++/--
#endif // /HAVE_INTERLOCKED_INCREMENT.

#endif /* !defined(HELIX_CONFIG_DISABLE_ATOMIC_OPERATORS) */

#endif /* _ATOMICBASE_H_ */

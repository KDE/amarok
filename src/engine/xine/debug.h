// (c) 2004 Max Howell (max.howell@methylblue.com)
// See COPYING file for licensing information

#ifndef XINEDEBUG_H
#define XINEDEBUG_H

#include <kdebug.h>

#ifdef NDEBUG
   static inline kndbgstream debug() { return kndbgstream(); }
#else
   #define DEBUG_PREFIX "xine-engine"

   #ifndef DEBUG_PREFIX
      #define _DEBUG_PREFIX ""
   #else
      #define _DEBUG_PREFIX "[" DEBUG_PREFIX "] "
   #endif

   static inline kdbgstream debug() { return kdbgstream( _DEBUG_PREFIX, 0, 0 ); }

   #undef _DEBUG_PREFIX
#endif

#define DEBUG_BEGIN kdDebug() << ">> " << __PRETTY_FUNCTION__ << endl;
#define DEBUG_END   kdDebug() << "<< " << __PRETTY_FUNCTION__ << endl;

#endif

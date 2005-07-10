
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

#ifndef _HXPLUGNCOMPAT_H_
#define _HXPLUGNCOMPAT_H_

// In order to maintain interoperability with existing software that uses
// plugins, redefine these symbols to their old names.

#ifndef HXCreateInstance
#define HXCreateInstance RMACreateInstance
#endif /* HXCreateInstance */

#ifndef HXCREATEINSTANCE
#define HXCREATEINSTANCESTR "RMACreateInstance"
#define HXCREATEINSTANCE RMACreateInstance
#endif /* HXCREATEINSTANCE */

#ifndef HXShutdown
#define HXShutdown RMAShutdown
#endif /* HXShutdown */

#ifndef HXSHUTDOWN
#define HXSHUTDOWNSTR "RMAShutdown"
#define HXSHUTDOWN RMAShutdown
#endif /* HXSHUTDOWN */

#endif // _HXPLUGNCOMPAT_H_

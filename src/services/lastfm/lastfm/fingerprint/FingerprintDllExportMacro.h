/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef FINGERPRINTDLLEXPORTMACRO_H
#define FINGERPRINTDLLEXPORTMACRO_H

// When we compile the tools header as part of liblastfmtools, we need
// dllexport for the functions to be exported. When including the header
// as part of the client modules, we want dllimport so that the compiler
// can do some optimisations. BUILD_EXPORT_DLL should only be defined in
// the tools .pro.
#if defined(_WIN32) || defined(WIN32)
    #ifdef FINGERPRINT_DLLEXPORT_PRO
        #define FINGERPRINT_DLLEXPORT __declspec(dllexport)
    #else
        #define FINGERPRINT_DLLEXPORT __declspec(dllimport)
    #endif
#else
    #define FINGERPRINT_DLLEXPORT
#endif

#endif // FINGERPRINTDLLEXPORTMACRO_H
/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef FINGERPRINTDLLEXPORTMACRO_H
#define FINGERPRINTDLLEXPORTMACRO_H

// When we compile the tools header as part of liblastfmtools, we need
// dllexport for the functions to be exported. When including the header
// as part of the client modules, we want dllimport so that the compiler
// can do some optimisations. BUILD_EXPORT_DLL should only be defined in
// the tools .pro.
#if defined(_WIN32) || defined(WIN32)
    #ifdef FINGERPRINT_DLLEXPORT_PRO
        #define FINGERPRINT_DLLEXPORT __declspec(dllexport)
    #else
        #define FINGERPRINT_DLLEXPORT __declspec(dllimport)
    #endif
#else
    #define FINGERPRINT_DLLEXPORT
#endif

#endif // FINGERPRINTDLLEXPORTMACRO_H

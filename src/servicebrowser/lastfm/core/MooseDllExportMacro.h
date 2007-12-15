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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef MOOSEDLLEXPORTMACRO_H
#define MOOSEDLLEXPORTMACRO_H

// When we compile the header in a DLL as part of the DLL, we need
// dllexport for the functions to be exported. When including the header
// as part of the client modules, we need dllimport. X_DLLEXPORT_PRO
// should only be defined in the DLL .pro.
#if defined(_WIN32) || defined(WIN32)
    #ifdef MOOSE_DLLEXPORT_PRO
        #define MOOSE_DLLEXPORT __declspec(dllexport)
    #else
        #define MOOSE_DLLEXPORT __declspec(dllimport)
    #endif
#else
    #define MOOSE_DLLEXPORT
#endif

#endif // MOOSEDLLEXPORTMACRO_H

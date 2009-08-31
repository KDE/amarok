/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef CONTEXT_H
#define CONTEXT_H

#include <QGraphicsItem>

/**
 * add the ContextState (which we need)
 */
namespace Context
{

enum ContextState { Home = 0 /**< Currently showing the home screen */,
        Current, /**< Showing Current Track screen. NB: a Current message is sent every time the track changes */
        Service /**< User is browsing a service */
};

} // Context namespace

#endif // multiple inclusion guard

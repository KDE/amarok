/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
*                                                                         *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef CONTEXT_H
#define CONTEXT_H

#include <QGraphicsItem>

/**
 * add the ContextState (which we need)
 */
namespace Context
{

enum ContextState { Home = 0 /**< Currently showing the home screen */,
        Current /**< Showing Current Track screen*/
};

} // Context namespace

#endif // multiple inclusion guard

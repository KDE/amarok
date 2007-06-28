/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
NOTE i need this empty skeleton file so cmake generates the ContextItem.moc file. if anyone knows how to make the MOC thing work with just a .h, please fix it/let me know!
*/

#include "ContextItem.h"

using namespace Context;

const QString ContextItem::name() 
{
    return "";
}
    
#include "ContextItem.moc"
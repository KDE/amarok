/****************************************************************************************
 * Copyright (c) 2007-2008 Leo Franchi <lfranchi@gmail.com>                             *
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

/*
  Significant parts of this code is inspired and/or copied from KDE plasma sources,
  available at kdebase/workspace/plasma
*/

#include "ContextView.h"
#include <QDebug>

namespace Context
{

ContextView* ContextView::s_self = 0;


ContextView::ContextView()
{
    qDebug() << "=============================youhou=========================================";
}

ContextView::~ContextView()
{}

ContextView* ContextView::self()
{
    if( !s_self ) s_self = new ContextView();
    return s_self; 
}

} // Context namespace

#include "ContextView.moc"


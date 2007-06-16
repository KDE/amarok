/***************************************************************************
 *   Copyright (C) 2007 by Leo Franchi <lfranchi@gmail.com>                *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "ContextScriptManager.h"

#include "contextview.h"

namespace Context {

ContextScriptManager *ContextScriptManager::s_instance = 0;

int ContextScriptManager::addContextBox( const QString& title, const QString& contents, const QString& stylesheet)
{

    int boxNum;
    if( m_boxes->isEmpty() ) 
        boxNum = 1;
    else 
        boxNum = ( m_boxes->keys().last() ) + 1; // next free number. keys() returns keys in ascending order
    
    GenericInfoBox* box = new GenericInfoBox();
    
    box->setTitle( title );
    box->setContents( contents );
    
    // TODO:  find an easy way to set the stylesheet (do we need to work w/ html?)
    //box->
    
    m_boxes->insert( boxNum ,box );
    
    ContextView::instance()->addContextBox( box );
    
    return boxNum;
}

void ContextScriptManager::changeBoxTitle( const int boxNum, const QString& title )
{
    (*m_boxes)[ boxNum ]->setTitle( title );
}

void ContextScriptManager::changeBoxContents( const int boxNum, const QString& contents )
{
    (*m_boxes)[ boxNum ]->setContents( contents );
}

void ContextScriptManager::changeBoxStylesheet( const int boxNum, const QString& stylesheet )
{
    //TODO:
    return;
}

void ContextScriptManager::removeContextBox( const int boxNum )
{
    if( m_boxes != 0 && m_boxes->contains( boxNum ) )
    {
        GenericInfoBox* b = (*m_boxes)[ boxNum ];
        m_boxes->remove( boxNum );
        // TODO call removeContentBox on ContextView here... needs to be implemented first
        //ContextView::instance()->
    }
}
}

#include "ContextScriptManager.moc"

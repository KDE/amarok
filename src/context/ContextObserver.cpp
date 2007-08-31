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


#include "ContextObserver.h"

#include "debug.h"

//////////////////////////////////////////////////////////////
////// CLASS ContextObserver
//////////////////////////////////////////////////////////////

ContextObserver::ContextObserver()
    : m_subject( 0 )
{}

ContextObserver::ContextObserver( ContextSubject *s )
    : m_subject( s )
{
    m_subject->attach( this );
}

ContextObserver::~ContextObserver()
{
    if( m_subject )
        m_subject->detach( this );
}

////////////////////////////////////////////////////////////////
//// CLASS ContextSubject
///////////////////////////////////////////////////////////////

void ContextSubject::messageNotify( const Context::ContextState& message )
{
    foreach( ContextObserver* obs, m_observers )
        obs->message( message );
}

void ContextSubject::attach( ContextObserver *obs )
{
    if( !obs  )
        return;
    m_observers.insert( obs );
}

void ContextSubject::detach( ContextObserver *obs )
{
    m_observers.removeAll( obs );
}


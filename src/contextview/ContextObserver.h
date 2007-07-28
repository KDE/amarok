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

#ifndef CONTEXT_OBSERVER_H
#define CONTEXT_OBSERVER_H

#include <QList>


class ContextSubject;

class ContextObserver
{
public:
    ContextObserver();
    ContextObserver( ContextSubject* );
    virtual ~ContextObserver();
    
    virtual void message( const QString& ) {}
    
private:
    ContextSubject *m_subject;
};

class ContextSubject
{
public:
    void attach( ContextObserver *observer );
    void detach( ContextObserver *observer );
    
protected:
    ContextSubject() {}
    virtual ~ContextSubject() {}
    
    void messageNotify( const QString& message );
    
private:
    QList< ContextObserver* > m_observers;
};

#endif

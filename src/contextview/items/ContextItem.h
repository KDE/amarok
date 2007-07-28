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

#ifndef CONTEXT_ITEM_H
#define CONTEXT_ITEM_H

// why do we need this?
#include <QObject>

namespace Context
{

// Base class for all context view items

class ContextItem : public QObject
{
    
    Q_OBJECT
        
public:
    ContextItem() {}
    
    virtual const QString name();
    virtual const QString shownDuring() { return QString(); } // e.g. "home", "context"
    virtual void enable() {}
    virtual void disable() {}
    
    // controls where the boxes owned by this item get inserted in the contextview. use m_order when calling ContextView::addContextBox
    virtual void setPosition( int order ) { m_order = order; }
    virtual int getPosition() { return m_order; }
    
    ~ContextItem() {}
protected:
    int m_order; // location of insertion of context boxes in CV
};

}

#endif

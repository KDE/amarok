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
    virtual void notify( const QString& message );
};

}

#endif

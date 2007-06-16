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

#ifndef GENERIC_INFO_BOX_H
#define GENERIC_INFO_BOX_H

#include "contextbox.h"

namespace Context
{

    class GenericInfoBox : public ContextBox
    {
        Q_OBJECT
     public:
        explicit GenericInfoBox( QGraphicsItem *parent = 0, QGraphicsScene *scene = 0 );
                
        void setContents( const QString& );
        void clearContents();
    private slots:
        void externalUrl( const QString& );
    private:
        void init();
        
        QGraphicsTextItem* m_content;
    }; 
}
			

#endif // GENERIC_INFO_BOX_H

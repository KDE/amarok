/**************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>          *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef AMAROK_CONTEXT_TOOLBAR_VIEW
#define AMAROK_CONTEXT_TOOLBAR_VIEW

#include <QGraphicsView>

class QGraphicsScene;
class QWidget;

namespace Context
{

class ToolbarView : public QGraphicsView
{
    Q_OBJECT
    public:
        ToolbarView( QGraphicsScene* scene, QWidget* parent = 0 );
        ~ToolbarView();
        
        virtual QSize sizeHint() const;
        int heightForWidth ( int w ) const;
    protected:
        void resizeEvent( QResizeEvent * event );
        
    private:
        int m_height;
};    
    
}


#endif

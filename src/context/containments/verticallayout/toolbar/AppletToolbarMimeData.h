/**************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org  >        *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef AMAROK_APPLET_TOOLBAR_MIME_DATA_H
#define AMAROK_APPLET_TOOLBAR_MIME_DATA_H

#include <QMimeData>

namespace Context
{
    
class AppletToolbarAppletItem;
    
class AppletToolbarMimeData : public QMimeData
{
    Q_OBJECT
    public:
        AppletToolbarMimeData();
        ~AppletToolbarMimeData();
        
        void setAppletData( AppletToolbarAppletItem* applet );
        AppletToolbarAppletItem* appletData() const;
        
        void setLocationData( int loc );
        int locationData()  const;
    private:
        AppletToolbarAppletItem* m_applet;
        int m_location;
};
    
} // namespace

#endif

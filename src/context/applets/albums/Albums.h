/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2009 simon.esneault <simon.esneault@gmail.com>                         *
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

#ifndef ALBUMS_APPLET_H
#define ALBUMS_APPLET_H

#include <context/Applet.h>
#include <context/DataEngine.h>

class AlbumsView;
class TextScrollingWidget;
namespace Collections {
    class Collection;
}

class Albums : public Context::Applet
{
    Q_OBJECT
public:
    Albums( QObject* parent, const QVariantList& args );
    ~Albums();

public slots:
    virtual void init();
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );

protected:
    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );
    void createConfigurationInterface( KConfigDialog *parent );

private slots:
    void collectionDataChanged( Collections::Collection *collection );
    void saveConfiguration();
    void setRecentCount( int val );
    void setRightAlignLength( int state );

private:
    int m_recentCount;
    bool m_rightAlignLength;
    AlbumsView *m_albumsView;
    TextScrollingWidget *m_headerText;
};

K_EXPORT_AMAROK_APPLET( albums, Albums )

#endif

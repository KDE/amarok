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

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "core/meta/forward_declarations.h"

#include <QGraphicsLinearLayout>

class AlbumsView;
class KLineEdit;
namespace Collections {
    class Collection;
}
namespace Plasma {
    class IconWidget;
}

class Albums : public Context::Applet
{
    Q_OBJECT
public:
    Albums( QObject* parent, const QVariantList& args );
    ~Albums();

public Q_SLOTS:
    virtual void init();
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );

protected:
    void createConfigurationInterface( KConfigDialog *parent );
    void keyPressEvent( QKeyEvent *event );

private Q_SLOTS:
    void collectionDataChanged( Collections::Collection *collection );
    void saveConfiguration();
    void setRecentCount( int val );
    void setRightAlignLength( int state );
    void showFilterBar();
    void closeFilterBar();
    void filterTextChanged( const QString &text );

private:
    int m_recentCount;
    bool m_rightAlignLength;
    AlbumsView *m_albumsView;
    Meta::AlbumList m_albums;
    Meta::TrackPtr m_currentTrack;
    Plasma::IconWidget *m_filterIcon;
};

class AlbumsFilterBar : public QGraphicsWidget
{
    Q_OBJECT

public:
    AlbumsFilterBar( QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0 );
    ~AlbumsFilterBar() {}

    bool eventFilter( QObject *obj, QEvent *e );
    void focusEditor();

Q_SIGNALS:
    void closeRequested();
    void filterTextChanged( const QString &text );

private:
    KLineEdit *m_editor;
    Plasma::IconWidget *m_closeIcon;
};

AMAROK_EXPORT_APPLET( albums, Albums )

#endif

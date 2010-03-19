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

#include "AlbumsView.h"

#include <context/Applet.h>
#include <context/DataEngine.h>
#include <meta/Meta.h>

#include <KDialog>

#include <QList>
#include <QAction>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QStandardItemModel>

class TextScrollingWidget;
class QGraphicsPixmapItem;
class QLabel;
class QHBoxLayout;

class Albums : public Context::Applet
{
    Q_OBJECT
public:
    Albums( QObject* parent, const QVariantList& args );
    ~Albums();

    void init();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints);

public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );
    void showConfigurationInterface();


private slots:
    void configAccepted();
    void connectSource( const QString &source );

private:
    void prepareElements();
    QList<QAction*> contextualActions();

    QHBoxLayout* m_configLayout;
    int m_width;
    const qreal m_albumWidth;

    Meta::AlbumList m_albums;
    int m_albumCount;

    int m_maxTextWidth;
    QStandardItemModel *m_model;

    AlbumsView *m_albumsView;

    TextScrollingWidget *m_headerText;
    QVariantList m_albumsTracks;
};

K_EXPORT_AMAROK_APPLET( albums, Albums )

#endif

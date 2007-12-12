/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "amarok.h"
#include "debug.h"

#include "PodcastCategory.h"
#include "PodcastModel.h"

#include <QAction>
#include <QToolBar>
#include <QHeaderView>
#include <qnamespace.h>
#include <QIcon>
#include <QPainter>
#include <QPixmapCache>

namespace PlaylistBrowserNS {

PodcastCategory::PodcastCategory( PlaylistBrowserNS::PodcastModel *podcastModel )
 : Ui_PodcastCategoryBase()
    , m_podcastModel( podcastModel )
{
    Ui_PodcastCategoryBase::setupUi( this );
    podcastTreeView->setModel( podcastModel );
    podcastTreeView->header()->hide();
    podcastTreeView->setItemDelegate( new PodcastCategoryDelegate(podcastTreeView) );

    addPodcastButton->setIcon( KIcon( Amarok::icon( "add_playlist" ) ) );
    refreshPodcastsButton->setIcon( KIcon( Amarok::icon( "refresh" ) ) );
    configurePodcastsButton->setIcon( KIcon( Amarok::icon( "configure" ) ) );
    podcastsIntervalButton->setIcon( KIcon( Amarok::icon( "configure" ) ) );

    connect( addPodcastButton, SIGNAL( pressed() ), m_podcastModel, SLOT( addPodcast() ) );
    connect( refreshPodcastsButton, SIGNAL( pressed() ), m_podcastModel, SLOT( refreshPodcasts() ) );
    connect( configurePodcastsButton, SIGNAL( pressed() ), m_podcastModel, SLOT( configurePodcasts() ) );
    connect( podcastsIntervalButton, SIGNAL( pressed() ), m_podcastModel, SLOT( setPodcastsInterval() ) );
}

PodcastCategory::~PodcastCategory()
{
}

PodcastCategoryDelegate::PodcastCategoryDelegate( QTreeView * view ) : QItemDelegate()
        , m_view( view )
{
}

PodcastCategoryDelegate::~PodcastCategoryDelegate()
{
}

void
PodcastCategoryDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                                const QModelIndex & index) const
{
    DEBUG_BLOCK

    int width = m_view->viewport()->size().width() - 4;
    int height = 90;
    int iconWidth = 22;
    int iconHeight = 22;
    int iconPadX = 8;
    int iconPadY = 4;

    painter->save();
    painter->setRenderHint ( QPainter::Antialiasing );

    QPixmap background( width - 4, height - 4 );

    background.fill( Qt::transparent );

    painter->drawPixmap( option.rect.topLeft().x() + 2, option.rect.topLeft().y() + 2, background );


    if (option.state & QStyle::State_Selected)
        painter->setPen(Qt::blue);
    else
        painter->setPen(Qt::gray);

    //painter->drawRoundRect( option.rect.topLeft().x() + 2, option.rect.topLeft().y() + 2 ,250,66, 8 ,8 );

    if (option.state & QStyle::State_Selected)
        painter->setPen(Qt::blue);
    else
        painter->setPen(Qt::black);

    painter->setFont(QFont("Arial", 12));

    painter->drawPixmap( option.rect.topLeft() + QPoint( iconPadX, iconPadY ) , index.data( Qt::DecorationRole ).value<QIcon>().pixmap( iconWidth, iconHeight ) );


    QRectF titleRect;
    titleRect.setLeft( option.rect.topLeft().x() + iconWidth + iconPadX );
    titleRect.setTop( option.rect.top() );
    titleRect.setWidth( width - ( iconWidth  + iconPadX * 2 ) );
    titleRect.setHeight( iconHeight + iconPadY );

    QString title = index.data( Qt::DisplayRole ).toString();
    debug() << "title is " << title;
    painter->drawText ( titleRect, Qt::AlignLeft | Qt::AlignVCenter, title );

    painter->setFont(QFont("Arial", 9));

    QRectF textRect;
    textRect.setLeft( option.rect.topLeft().x() + iconPadX );
    textRect.setTop( option.rect.top() + iconHeight + iconPadY );
    textRect.setWidth( width - iconPadX * 2 );
    textRect.setHeight( height - ( iconHeight + iconPadY ) );

    painter->drawText ( textRect, Qt::TextWordWrap | Qt::AlignHCenter, index.data( ShortDescriptionRole ).toString() );

    painter->restore();

}

QSize
PodcastCategoryDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED( option );
    Q_UNUSED( index );

    //DEBUG_BLOCK

    int width = m_view->viewport()->size().width() - 4;
    int heigth = 90;

    return QSize ( width, heigth );
}

}

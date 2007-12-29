/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>
   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

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
#include "PodcastMeta.h"

#include <QAction>
#include <QToolBar>
#include <QHeaderView>
#include <qnamespace.h>
#include <QIcon>
#include <QPainter>
#include <QPixmapCache>
#include <QLinearGradient>
#include <QFontMetrics>
#include <QRegExp>

#include <typeinfo>

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
    debug() << "width = " << width;
    int height = 90;
    int iconWidth = 32;
    int iconHeight = 32;
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
        painter->setPen(Qt::black);

    painter->setFont(QFont("Arial", 12));

    painter->drawPixmap( option.rect.topLeft() + QPoint( iconPadX, iconPadY ) , index.data( Qt::DecorationRole ).value<QIcon>().pixmap( iconWidth, iconHeight ) );


    QRectF titleRect;
    titleRect.setLeft( option.rect.topLeft().x() + iconWidth + iconPadX );
    titleRect.setTop( option.rect.top() );
    titleRect.setWidth( width - ( iconWidth  + iconPadX * 2 ) );
    titleRect.setHeight( iconHeight + iconPadY );

    painter->drawText ( titleRect, Qt::AlignLeft | Qt::AlignVCenter, index.data( Qt::DisplayRole ).toString() );

    painter->setFont(QFont("Arial", 9));

    QRectF textRect;
    textRect.setLeft( option.rect.topLeft().x() + iconPadX );
    textRect.setTop( option.rect.top() + iconHeight + iconPadY );
    textRect.setWidth( width - iconPadX * 2 - m_view->indentation() );
    textRect.setHeight( height - ( iconHeight + iconPadY ) );

    QString description = index.data( ShortDescriptionRole ).toString();

    description.replace( QRegExp("\n+"), "\n" );
    QFontMetricsF fm( painter->font() );
    QRectF textBound = fm.boundingRect( textRect, Qt::TextWordWrap | Qt::AlignHCenter, description );
    bool toWide = textBound.width() > textRect.width();
    bool toHigh = textBound.height() > textRect.height();
    if ( toHigh || toWide ) {
        QLinearGradient gradient;
        gradient.setStart( textRect.topLeft() );

        if( toWide && toHigh ) gradient.setFinalStop( textRect.bottomRight() );
        else if ( toWide ) gradient.setFinalStop( textRect.topRight() );
        else gradient.setFinalStop( textRect.bottomLeft() );

        gradient.setColorAt(0.8, painter->pen().color());
        gradient.setColorAt(1.0, Qt::transparent);
        QPen pen;
        pen.setBrush(QBrush(gradient));
        painter->setPen(pen);
    }

    painter->drawText( textRect, Qt::TextWordWrap | Qt::AlignHCenter, description );

    painter->restore();

}

QSize
PodcastCategoryDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED( option );

    //DEBUG_BLOCK

    int width = m_view->viewport()->size().width() - 4;

    Meta::PodcastMetaCommon* pmc = static_cast<Meta::PodcastMetaCommon *>( index.internalPointer() );
    int heigth = 90;
    if ( typeid( * pmc ) == typeid( Meta::PodcastChannel ) )
    {
        heigth = 40;
    }

    return QSize ( width, heigth );
}

}

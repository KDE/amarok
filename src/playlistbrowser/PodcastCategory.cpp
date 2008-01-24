/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>
   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
   Copyright (c) 2007  Henry de Valence <hdevalence@gmail.com>

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
#include <QSvgRenderer>
#include <QFile>

#include <KStandardDirs>

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

    addPodcastButton->setIcon( KIcon( "list-add-amarok" ) );
    refreshPodcastsButton->setIcon( KIcon( "view-refresh-amarok" ) );
    configurePodcastsButton->setIcon( KIcon( "configure-amarok" ) );
    podcastsIntervalButton->setIcon( KIcon( "configure-amarok" ) );

    connect( addPodcastButton, SIGNAL( pressed() ), m_podcastModel, SLOT( addPodcast() ) );
    connect( refreshPodcastsButton, SIGNAL( pressed() ), m_podcastModel, SLOT( refreshPodcasts() ) );
    connect( configurePodcastsButton, SIGNAL( pressed() ), m_podcastModel, SLOT( configurePodcasts() ) );
    connect( podcastsIntervalButton, SIGNAL( pressed() ), m_podcastModel, SLOT( setPodcastsInterval() ) );

    m_viewKicker = new ViewKicker( podcastTreeView );
    
    //connect( podcastTreeView, SIGNAL( clicked( const QModelIndex & ) ), podcastModel, SLOT( emitLayoutChanged() ) );
    connect( podcastTreeView, SIGNAL( clicked( const QModelIndex & ) ), m_viewKicker, SLOT( kickView() ) );
    

}

PodcastCategory::~PodcastCategory()
{
}




ViewKicker::ViewKicker( QTreeView * treeView )
{
     DEBUG_BLOCK
     m_treeView = treeView;
}

void ViewKicker::kickView()
{
    DEBUG_BLOCK
    m_treeView->setRootIndex( QModelIndex() );
}








PodcastCategoryDelegate::PodcastCategoryDelegate( QTreeView * view ) : QItemDelegate()
        , m_view( view )

{
}

PodcastCategoryDelegate::~PodcastCategoryDelegate()
{
}

void
PodcastCategoryDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option,
                                const QModelIndex & index ) const
{
    DEBUG_BLOCK
    //debug() << "Option state = " << option.state;

    int width = m_view->viewport()->size().width() - 4;
    //debug() << "width = " << width;
    int iconWidth = 16;
    int iconHeight = 16;
    int iconPadX = 8;
    int iconPadY = 4;
    int height = option.rect.height();

    //HACK:Just for testing!!
    /*if  ( m_lastHeight > 1 ) {
        //m_lastHeight = 1;
        PodcastModel * podcastModel = ( PodcastModel * ) index.model();
        debug() << "HEEEEEEEEEEEEEEEEREEEEEEEEEEE DAMMMMITTTTTT!!!!!";
        podcastModel->emitLayoutChanged();
    }*/

    painter->save();
    painter->setRenderHint ( QPainter::Antialiasing );

    QPixmap background( width - 4, height - 4 );
    QString key;

    if (option.state & QStyle::State_Selected) {
        /*TODO: don't open file every time; it's only this way
        because I had problems getting it to work as a member
        variable like it is in the ServiceListDelegate class;
        I'm not sure why it didn't work, but it crashed every
        time; I think I was doing it wrong. */
        iconPadY = 12; //looks bad if too close to border
        iconPadX = 12;
        key = QString("service_list_item_selected:%1x%2").arg( width ).arg( height );
        if (!QPixmapCache::find(key, background)) {
            background.fill( Qt::transparent );
            QPainter pt( &background );
            //only opens if selected AND not in cache, see TODO above
            QFile file( KStandardDirs::locate( "data","amarok/images/service-browser-element.svg" ) );
            file.open( QIODevice::ReadOnly );
            QString svg_source( file.readAll() );
            QSvgRenderer *svgRenderer = new QSvgRenderer( svg_source.toAscii() );

            svgRenderer->render ( &pt,  QRectF( 0, 0 ,width - 20, height - 4 ) );
            QPixmapCache::insert(key, background);
        }
    } else { background.fill( Qt::transparent ); }


    painter->drawPixmap( option.rect.topLeft().x() + 2, option.rect.topLeft().y() + 2, background );

    painter->setPen(Qt::black);

    painter->setFont(QFont("Arial", 9));

    painter->drawPixmap( option.rect.topLeft() + QPoint( iconPadX, iconPadY ) ,
                         index.data( Qt::DecorationRole ).value<QIcon>().pixmap( iconWidth, iconHeight ) );


    QRectF titleRect;
    titleRect.setLeft( option.rect.topLeft().x() + iconWidth + iconPadX );
    titleRect.setTop( option.rect.top() );
    titleRect.setWidth( width - ( iconWidth  + iconPadX * 2 + m_view->indentation() ) );
    titleRect.setHeight( iconHeight + iconPadY );

    QString title = index.data( Qt::DisplayRole ).toString();


    //TODO: these metrics should be made static members so they are not created all the damn time!!
    QFontMetricsF tfm( painter->font() );

    title = tfm.elidedText ( title, Qt::ElideRight, titleRect.width(), Qt::AlignHCenter );
    //TODO: has a weird overlap
    painter->drawText ( titleRect, Qt::AlignLeft | Qt::AlignBottom, title );

    painter->setFont(QFont("Arial", 8));

    QRectF textRect;
    textRect.setLeft( option.rect.topLeft().x() + iconPadX );
    textRect.setTop( option.rect.top() + iconHeight + iconPadY );
    textRect.setWidth( width - ( iconPadX * 2 + m_view->indentation() + 16) );
    textRect.setHeight( height - ( iconHeight + iconPadY ) );


    
    QFontMetricsF fm( painter->font() );
    QRectF textBound;
    
    QString description = index.data( ShortDescriptionRole ).toString();
    description.replace( QRegExp("\n "), "\n" );
    description.replace( QRegExp("\n+"), "\n" );


    if (option.state & QStyle::State_Selected)
        textBound = fm.boundingRect( textRect, Qt::TextWordWrap | Qt::AlignHCenter, description );
    else
        textBound = fm.boundingRect( titleRect, Qt::TextWordWrap | Qt::AlignHCenter, title );

    bool toWide = textBound.width() > textRect.width();
    bool toHigh = textBound.height() > textRect.height();
    if ( toHigh || toWide ) {
        QLinearGradient gradient;
        gradient.setStart( textRect.bottomLeft().x(), textRect.bottomLeft().y() - 16 );

        //if( toWide && toHigh ) gradient.setFinalStop( textRect.bottomRight() );
        //else if ( toWide ) gradient.setFinalStop( textRect.topRight() );
        gradient.setFinalStop( textRect.bottomLeft() );

        gradient.setColorAt(0, painter->pen().color());
        gradient.setColorAt(0.5, Qt::transparent);
        gradient.setColorAt(1, Qt::transparent);
        QPen pen;
        pen.setBrush(QBrush(gradient));
        painter->setPen(pen);
    }

    if (option.state & QStyle::State_Selected) {
        painter->drawText( textRect, Qt::TextWordWrap | Qt::AlignVCenter | Qt::AlignLeft, description );
        //debug() << "drawing description text";
    }

    painter->restore();

}

QSize
PodcastCategoryDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED( option );

    DEBUG_BLOCK

    //debug() << "Option state = " << option.state;

    int width = m_view->viewport()->size().width() - 4;

    //todo: the heigth should be defined the way it is in the delegate: iconpadY*2 + iconheight
    Meta::PodcastMetaCommon* pmc = static_cast<Meta::PodcastMetaCommon *>( index.internalPointer() );
    int heigth = 24;
    if ( typeid( * pmc ) == typeid( Meta::PodcastChannel ) ) {
        heigth = 24;
    }
    if (/*option.state & QStyle::State_HasFocus*/ m_view->currentIndex() == index ) {
        //lets try to do heights based on amount of text...

        QString description = index.data( ShortDescriptionRole ).toString();
        
        QFontMetrics fm( QFont( "Arial", 8 ) );
        heigth = fm.boundingRect ( 0, 0, width - ( 32 + m_view->indentation() ), 1000,
                                   Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap ,
                                   description ).height() + 40;
	    debug() << "Option is selected, height = " << heigth;
    }
    //else
	//debug() << "Option is not selected, height = " << heigth;

    //m_lastHeight = heigth;
    return QSize ( width, heigth );
}

}

#include "PodcastCategory.moc"

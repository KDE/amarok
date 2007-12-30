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
    int height = (iconPadY * 2) + iconHeight;
    if ( option.state & QStyle::State_Selected )
    {
        height = 90;
	//debug() << "Option is selected; height = " << height;
    }

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
    if (option.state & QStyle::State_Selected)
    {
        //debug() << "Selected, painting blue gradient";
	background.fill( Qt::blue );
	QPixmap grad( background.size() );
	QPainter p( &grad );
	QLinearGradient g( grad.width() / 2, 0, grad.width() / 2, grad.height() );
	g.setColorAt(0, Qt::gray);
	g.setColorAt(0.1, QColor(50, 50, 50));
	g.setColorAt(0.5, Qt::black);
	g.setColorAt(0.9, QColor(50, 50, 50));
	g.setColorAt(1, Qt::gray);
	p.setBrush(g);
	p.setPen(Qt::NoPen);
	p.drawRect( 0, 0, grad.width(), grad.height() );
	background.setAlphaChannel( grad );
    }
    else
        background.fill( Qt::transparent );

    painter->drawPixmap( option.rect.topLeft().x() + 2, option.rect.topLeft().y() + 2, background );

/*  //used to test the blue on blue effect
    if (option.state & QStyle::State_Selected)
        painter->setPen(Qt::blue);
    else */
        painter->setPen(Qt::black);

    painter->setFont(QFont("Arial", 9));

    painter->drawPixmap( option.rect.topLeft() + QPoint( iconPadX, iconPadY ) , index.data( Qt::DecorationRole ).value<QIcon>().pixmap( iconWidth, iconHeight ) );


    QRectF titleRect;
    titleRect.setLeft( option.rect.topLeft().x() + iconWidth + iconPadX );
    titleRect.setTop( option.rect.top() );
    titleRect.setWidth( width - ( iconWidth  + iconPadX * 2 ) );
    titleRect.setHeight( iconHeight + iconPadY );

    QString title = index.data( Qt::DisplayRole ).toString();
    painter->drawText ( titleRect, Qt::AlignLeft | Qt::AlignVCenter, title );

    painter->setFont(QFont("Arial", 8));

    QRectF textRect;
    textRect.setLeft( option.rect.topLeft().x() + iconPadX );
    textRect.setTop( option.rect.top() + iconHeight + iconPadY );
    textRect.setWidth( width - iconPadX * 2 - m_view->indentation() );
    textRect.setHeight( height - ( iconHeight + iconPadY ) );

    QFontMetricsF fm( painter->font() );
    QRectF textBound;
    QString description; 
    if (option.state & QStyle::State_Selected)
    {
        QString description = index.data( ShortDescriptionRole ).toString();
        description.replace( QRegExp("\n+"), "\n" );
	//debug() << "description = " << description;
        textBound = fm.boundingRect( textRect, Qt::TextWordWrap | Qt::AlignHCenter, description );
    }
    else
        textBound = fm.boundingRect( titleRect, Qt::TextWordWrap | Qt::AlignHCenter, title );

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

    if (option.state & QStyle::State_Selected)
    {
        painter->drawText( textRect, Qt::TextWordWrap | Qt::AlignHCenter, description );
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
    if ( typeid( * pmc ) == typeid( Meta::PodcastChannel ) )
    {
        heigth = 24;
    }
    if (/*option.state & QStyle::State_HasFocus*/ m_view->currentIndex() == index )
    {
        heigth = 90;
	    debug() << "Option is selected, height = " << heigth;
    }
    //else
	//debug() << "Option is not selected, height = " << heigth;

    //m_lastHeight = heigth;
    return QSize ( width, heigth );
}

}

#include "PodcastCategory.moc"

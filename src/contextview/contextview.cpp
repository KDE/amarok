/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "albumbox.h"
#include "cloudbox.h"
#include "collectiondb.h"
#include "contextbox.h"
#include "contextview.h"
#include "debug.h"
#include "graphicsitemfader.h"

#include <kstandarddirs.h>

#include <math.h> // scaleView()
#include <QBrush>
#include <QColor>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QWheelEvent>

using namespace Context;

ContextView *ContextView::s_instance = 0;

ContextView::ContextView()
    : QGraphicsView()
{
    s_instance = this; // we are a singleton class

    initiateScene();
    setAlignment( Qt::AlignTop | Qt::AlignLeft );
    setRenderHints( QPainter::Antialiasing );
    setCacheMode( QGraphicsView::CacheBackground ); // this won't be changing regularly

    showHome();
}

void ContextView::initiateScene()
{
    m_contextScene = new QGraphicsScene( this );
    m_contextScene->setItemIndexMethod( QGraphicsScene::BspTreeIndex );
    m_contextScene->setBackgroundBrush( palette().highlight() );
    setScene( m_contextScene );
}

void ContextView::showHome()
{
    clear();

    ContextBox *welcomeBox = new ContextBox();
    welcomeBox->setTitle( "Hooray, welcome to Amarok::ContextView!" );

    addContextBox( welcomeBox );
    debug() << "WELCOME BOX: " << welcomeBox->pos() << endl;

    /*
    AlbumBox *album = new AlbumBox( 0, m_contextScene );
    const QString &cover = CollectionDB::instance()->albumImage( "3 Doors Down", "The Better Life", false, 50 );
    debug() << "cover: " << cover ;
    album->addAlbumInfo( cover, "3 Doors Down - The Better Life" );
    addContextBox( album );
    */

    /*
    FadingImageItem * fadeingImage = new FadingImageItem (QPixmap( KStandardDirs::locate("data", "amarok/images/splash_screen.jpg" ) ) );
    QColor color = palette().highlight();
    fadeingImage->setFadeColor( color );
    fadeingImage->setTargetAlpha( 200 );
    */

    QGraphicsPixmapItem *logoItem = new QGraphicsPixmapItem ( QPixmap( KStandardDirs::locate("data", "amarok/images/splash_screen.jpg" ) ) );

    GraphicsItemFader *logoFader = new GraphicsItemFader( logoItem, 0 );

    logoFader->setDuration( 5000 );
    logoFader->setFPS( 30 );
    logoFader->setStartAlpha( 0 );
    logoFader->setTargetAlpha( 200 );
    logoFader->setFadeColor( palette().highlight() );

    addContextBox( logoFader );
    logoFader->startFading();

    QGraphicsPixmapItem *logoItem1 = new QGraphicsPixmapItem ( QPixmap( KStandardDirs::locate("data", "amarok/images/splash_screen.jpg" ) ) );
    addContextBox( logoItem1 );
}

void ContextView::scaleView( qreal factor )
{
    qreal scaleF = matrix().scale( factor, factor).mapRect(QRectF(0, 0, 1, 1)).width();
    if( scaleF < 0.07 || scaleF > 100 )
         return;

    scale( factor, factor );
}

void ContextView::wheelEvent( QWheelEvent *event )
{
     scaleView( pow( (double)2, -event->delta() / 240.0) );
}

void ContextView::clear()
{
    delete m_contextScene;
    initiateScene();
}

void ContextView::addContextBox( QGraphicsItem *newBox, int after )
{
    // For now, let's assume that all the items are listed in a vertical alignment
    // with a constant padding between the elements. Does this need to be more robust?
    QList<QGraphicsItem*> items = m_contextScene->items();
    qreal yposition = BOX_PADDING;

    if( !items.isEmpty() )
    {
        // Since the items are returned in no particular order, we must sort the items first
        // based on the topmost edge of the box.
        qSort( items.begin(), items.end(), higherThan );

        if( after >= items.count() )
            after = -1;

        QGraphicsItem *afterItem = 0;

        // special case 'add-to-end' index, -1.
        if( after < 0 )
            afterItem = items.last();
        else
            afterItem = items.at( after );

        if( afterItem )
            yposition = afterItem->sceneBoundingRect().bottom() + BOX_PADDING;
    }

    debug() << "placing box at position: " << after << ", y position of box: " << yposition << endl;

    m_contextScene->addItem( newBox );
    newBox->setPos( BOX_PADDING, yposition );
}

bool ContextView::higherThan( const QGraphicsItem *i1, const QGraphicsItem *i2 )
{
    return ( i1->sceneBoundingRect().top() > i2->sceneBoundingRect().top() );
}

/***************************************************************************
 * copyright     : (C) 2007 Seb Ruiz <ruiz@kde.org>                        *
 *                 (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> *
*                  (C) 2007 Leonardo Franchi <lfranchi@gmail.com>          *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarokconfig.h"
#include "debug.h"
#include "albumbox.h"
#include "cloudbox.h"
#include "GenericInfoBox.h"
#include "textfader.h"
#include "collectiondb.h"
#include "contextbox.h"
#include "contextview.h"
#include "debug.h"
#include "enginecontroller.h"
#include "graphicsitemfader.h"
#include "introanimation.h"
#include "scriptmanager.h"
#include "statusbar.h"
#include "LyricsItem.h"
#include "WikipediaItem.h"

#include <kstandarddirs.h>

#include <math.h> // scaleView()
#include <QBrush>
#include <QColor>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QWheelEvent>

//just for testing
#include <QSvgRenderer>

using namespace Context;

ContextView *ContextView::s_instance = 0;

ContextView::ContextView()
    : QGraphicsView()
    , EngineObserver( EngineController::instance() )
{
    s_instance = this; // we are a singleton class

    LyricsItem::instance();
    WikipediaItem::instance();
    
    initiateScene();
    setAlignment( Qt::AlignTop );
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

void ContextView::engineStateChanged( Engine::State state, Engine::State oldState )
{
    DEBUG_BLOCK
    Q_UNUSED( oldState );

    switch( state )
    {
        case Engine::Playing:
            showCurrentTrack();
            break;

        case Engine::Empty:
            showHome();
            break;

        default:
            ;
    }
}

void ContextView::engineNewMetaData( const MetaBundle&, bool )
{
}

void ContextView::showHome()
{
    clear();

    /*
    FadingImageItem * fadeingImage = new FadingImageItem (QPixmap( KStandardDirs::locate("data", "amarok/images/splash_screen.jpg" ) ) );
    QColor color = palette().highlight();
    fadeingImage->setFadeColor( color );
    fadeingImage->setTargetAlpha( 200 );
    */

    /*
    QGraphicsPixmapItem *logoItem = new QGraphicsPixmapItem ( QPixmap( KStandardDirs::locate("data", "amarok/images/splash_screen.jpg" ) ) );

    GraphicsItemFader *logoFader = new GraphicsItemFader( logoItem, 0 );
//     logoFader->setTargetAlpha( 200 );
    logoFader->setFadeColor( palette().highlight() );
    logoFader->setDuration( 5000 );
    logoFader->setFPS( 30 );
    logoFader->setStartAlpha( 0 );
    logoFader->setTargetAlpha( 200 );

    addContextBox( logoFader );
    logoFader->startFading();
    */

    //test TextFader

   /* TextFader *textFader = new TextFader("Hello, World", 0);
    QFont font = textFader->font();
    font.setPointSize( 20 );
    textFader->setFont( font );

    textFader->setDuration( 5000 );
    textFader->setFPS( 30 );
    textFader->setStartAlpha( 0 );
    textFader->setTargetAlpha( 255 );

    addContextBox( textFader );
    textFader->startFading();*/

    /*
    IntroAnimation *introAnim = new IntroAnimation();

    connect( introAnim, SIGNAL( animationComplete() ), this, SLOT( introAnimationComplete() ) );

    debug() << "starting intro anim" << endl;

    introAnim->setFadeColor( palette().highlight() );
    addContextBox( introAnim );
    introAnim->startAnimation();
    */
    introAnimationComplete();
}



void ContextView::introAnimationComplete()
{
    clear();
    debug() << "introAnimationComplete!"  << endl;

    ContextBox *welcomeBox = new ContextBox();
    welcomeBox->setTitle( "Hooray, welcome to Amarok::ContextView!" );
    addContextBox( welcomeBox );


    AlbumBox *albumBox = new AlbumBox();
    albumBox->setTitle( "Your Newest Albums" );

    // because i don't know how to use the new QueryMaker class...
    QString query = "SELECT distinct(AL.name), AR.name, YR.name "
                    "FROM tags T "
                    "INNER JOIN album AL ON T.album = AL.id "
                    "INNER JOIN artist AR ON T.artist = AR.id "
                    "INNER JOIN year YR ON T.year = YR.id "
                    "WHERE T.sampler=0 "
                    "ORDER BY T.createdate DESC "
                     "LIMIT 5 OFFSET 0";
    QStringList values = CollectionDB::instance()->query( query, false );
    debug() << "Result count: " << values.count() << endl;

    for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it )
    {
        const QString album = *it;
        const QString artist = *++it;
        const QString year = *++it;
        const QString &cover = CollectionDB::instance()->albumImage( artist, album, false, 50 );
        debug() << "artist: " << artist << " album: " << album << " cover: " << cover << endl;
        albumBox->addAlbumInfo( cover, QString( "%1 - %2\n%3" ).arg( artist, album, year ) );
    }

    addContextBox( albumBox );

    // testing
    //QTimer::singleShot( 5000, this, SLOT( testBoxLayout() ) );
}

void ContextView::testBoxLayout()
{
    static bool s_add = true;

    if( !m_testItem )
    {
        m_testItem = new ContextBox();
        m_testItem->setTitle( "Test Item" );
//         QGraphicsSvgItem *svg = new QGraphicsSvgItem( KStandardDirs::locate("data", "amarok/images/amarok_icon.svg" ), m_testItem );
//         svg->scale(0.5, 0.5 );
//         svg->moveBy( 0, 30 );
    }

    if( s_add )
        addContextBox( m_testItem, 1, true );
    else
        removeContextBox( m_testItem, true );

    s_add = !s_add;
    QTimer::singleShot( 5000, this, SLOT( testBoxLayout() ) );
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
    if( event->modifiers() & Qt::ControlModifier )
        scaleView( pow( (double)2, -event->delta() / 240.0) );
    else
        QGraphicsView::wheelEvent( event );
}

void ContextView::resizeEvent( QResizeEvent *event )
{
    QSize newSize = event->size();
    QList<QGraphicsItem*> items = m_contextScene->items();

    foreach( QGraphicsItem *item, items )
    {
        ContextBox *box = dynamic_cast<ContextBox*>( item );
        if( box )
            box->ensureWidthFits( newSize.width() );
    }
}

void ContextView::clear()
{
    // tell member items that their boxes are being removed
    notifyItems( QString( "boxesRemoved" ) );
    
    delete m_contextScene;

    initiateScene();
    update();
}

void ContextView::removeContextBox( QGraphicsItem *oldBox, bool fadeOut )
{
    DEBUG_BLOCK

    if( !oldBox || !m_contextScene )
        return;

    if( fadeOut )
    {
        GraphicsItemFader *fader = new GraphicsItemFader( oldBox, 0 );
        fader->setDuration( 2500 );
        fader->setFPS( 30 );
        fader->setStartAlpha( 0 );
        fader->setTargetAlpha( 255 );
        fader->setFadeColor( palette().highlight().color() );
        fader->startFading();
        oldBox = fader;
    }

    QList<QGraphicsItem*> items = m_contextScene->items();

    QMap<qreal, ContextBox*> boxesAndBorders;

    if( !items.isEmpty() )
    {
        int index = 1;
        foreach( QGraphicsItem* i, items )
        {
            ContextBox *box = dynamic_cast<ContextBox*>(i);
            if( box )
            {
                boxesAndBorders.insertMulti( box->sceneBoundingRect().top(), box );
            }
        }

        // bottoms and boxes are guaranteed to be in the same order
        const QList<qreal>       tops  = boxesAndBorders.keys();
        const QList<ContextBox*> boxes = boxesAndBorders.values();

        QList<QGraphicsItem*> shuffleUp;

        // need to shuffle up all the boxes below
        for( ; index < boxes.size(); ++index )
        {
            ContextBox *box = boxes.at(index);

            debug() << "shuffling up: " << box->title() << endl;
            shuffleUp << box;
        }

        qreal distance = oldBox->boundingRect().height() + BOX_PADDING;
        debug() << "shuffling " << shuffleUp.size() << " items up, a total of " << distance << endl;

        shuffleItems( shuffleUp, distance, ShuffleUp );
    }

    m_contextScene->removeItem( oldBox );
}

// Places a context box at the location specified by @param index. -1 -> at the bottom
void ContextView::addContextBox( QGraphicsItem *newBox, int index, bool fadeIn )
{
    DEBUG_BLOCK

    if( !newBox || !m_contextScene )
        return;

    if( fadeIn )
    {
        GraphicsItemFader *fader = new GraphicsItemFader( newBox, 0 );
        fader->setDuration( 2500 );
        fader->setFPS( 30 );
        fader->setStartAlpha( 255 );
        fader->setTargetAlpha( 0 );
        fader->setFadeColor( palette().highlight().color() );
        fader->startFading();
        newBox = fader;
    }

    // For now, let's assume that all the items are listed in a vertical alignment
    // with a constant padding between the elements. Does this need to be more robust?
    QList<QGraphicsItem*> items = m_contextScene->items();
    qreal yposition = BOX_PADDING;

    QMap<qreal, ContextBox*> boxesAndBorders;

    if( !items.isEmpty() )
    {
        // Since the items are returned in no particular order, we must sort the items first
        // based on the bottom edge of the box. A QMap is always sorted by key
        foreach( QGraphicsItem* i, items )
        {
            // insertMulti allows for many values with the same key. important if we decide to change
            // the layout later (eg, multiple columns, with boxes with the same bottom border y position)
            ContextBox *box = dynamic_cast<ContextBox*>(i);
            if( box )
                boxesAndBorders.insertMulti( box->sceneBoundingRect().bottom(), box );
        }

        if( index >= items.count() )
            index = -1;

        // bottoms and boxes are guaranteed to be in the same order
        const QList<qreal>       bottoms = boxesAndBorders.keys();
        const QList<ContextBox*> boxes   = boxesAndBorders.values();

        debug() << "box count: " << boxes.size() << endl;

        // special case 'add-to-end' index, -1.
        if( index < 0 )
            yposition = bottoms.last() + BOX_PADDING;
        else
        {
            if( index > 0 ) // we need the bottom value for the box above it.
                yposition = bottoms.at( index - 1 ) + BOX_PADDING;

            debug() << "y-position: " << yposition << endl;

            QList<QGraphicsItem*> shuffleDown;

            // need to shuffle down all the boxes below
            for( int i = index; i < boxes.size(); ++i )
                shuffleDown << boxes.at( i );

            qreal distance = newBox->boundingRect().height() + BOX_PADDING;
            debug() << "shuffling " << shuffleDown.size() << " items down, a total of " << distance << endl;

            shuffleItems( shuffleDown, distance, ShuffleDown );
        }
    }

    debug() << "placing box at position: " << index << ", y position of box: " << yposition << endl;

    m_contextScene->addItem( newBox );
    newBox->setPos( BOX_PADDING, yposition );
}

void ContextView::addContextItem( ContextItem* item )
{
    m_contextItems << item;
}

void ContextView::removeContextItem( ContextItem* item )
{
    QList< ContextItem* >::iterator i;
    for( i = m_contextItems.begin(); i != m_contextItems.end(); ++i )
    {
        if( (*i) == item )
            m_contextItems.erase( i );
    }
}

/** Sends the desired message to all items that are registered on the ContextView */
void ContextView::notifyItems( const QString& message )
{
    foreach( ContextItem* item, m_contextItems )
        item->notify( message );
}

void ContextView::shuffleItems( QList<QGraphicsItem*> items, qreal distance, int direction )
{
    if( direction == ShuffleUp )
        distance = -distance;

    QList<QGraphicsItem*>::iterator i;
    for( i = items.begin(); i != items.end(); ++i )
    {
        (*i)->moveBy( 0, distance );
    }
}

void ContextView::showCurrentTrack()
{
    clear();

    MetaBundle bundle = EngineController::instance()->bundle();

    ContextBox *infoBox = new ContextBox();
    infoBox->setTitle( i18n("%1 - %2", bundle.title(), bundle.artist() ) );
    addContextBox( infoBox );

    CloudBox *relatedArtists = new CloudBox();
    relatedArtists->setTitle( i18n("Related Artists to %1", bundle.artist() ) );
    QStringList relations = CollectionDB::instance()->similarArtists( bundle.artist(), 10 );
    foreach( QString r, relations )
        relatedArtists->addText( r );

    addContextBox( relatedArtists );

    AlbumBox *albumBox = new AlbumBox();
    albumBox->setTitle( i18n("Albums By %1", bundle.artist() ) );

    int artistId = CollectionDB::instance()->artistID( bundle.artist() );
    // because i don't know how to use the new QueryMaker class...
    QString query = QString("SELECT distinct(AL.name), AR.name, YR.name "
            "FROM tags T "
            "INNER JOIN album AL ON T.album = AL.id "
            "INNER JOIN artist AR ON T.artist = AR.id "
            "INNER JOIN year YR ON T.year = YR.id "
            "WHERE AR.id = %1 "
            "ORDER BY YR.name DESC").arg( artistId );

    QStringList values = CollectionDB::instance()->query( query, false );
    debug() << "Result count: " << values.count() << endl;

    for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it )
    {
        QString album = *it;
        if( album.isEmpty() )
            album = i18n( "Unknown" );

        const QString artist = *++it;
        const QString year = *++it;
        const QString &cover = CollectionDB::instance()->albumImage( artist, album, false, 50 );
        debug() << "artist: " << artist << " album: " << album << " cover: " << cover << endl;
        albumBox->addAlbumInfo( cover, QString( "%1 - %2\n%3" ).arg( artist, album, year ) );
    }

    addContextBox( albumBox );
}

#include "contextview.moc"

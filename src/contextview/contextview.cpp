/***************************************************************************
 * copyright     : (C) 2007 Seb Ruiz <ruiz@kde.org>                        *
 *                 (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> *
 *                 (C) 2007 Leonardo Franchi <lfranchi@gmail.com>          *
 *                 (C) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>      *
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
#include "GenericInfoBox.h"
#include "textfader.h"
#include "collectiondb.h"
#include "contextbox.h"
#include "contextview.h"
#include "debug.h"
#include "enginecontroller.h"
#include "graphicsitemfader.h"
#include "graphicsitemscaler.h"
#include "introanimation.h"
#include "scriptmanager.h"
#include "statusbar.h"
#include "items/ContextItemManager.h"

#include <kstandarddirs.h>

#include <math.h> // scaleView()
#include <QBrush>
#include <QColor>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QWheelEvent>

//just for testing
#include <QSvgRenderer>

using namespace Context;

static bool enablePUD = false;
static bool eyeCandyFlag = false;

ContextView *ContextView::s_instance = 0;

ContextView::ContextView()
    : QGraphicsView()
    , EngineObserver( EngineController::instance() )
    , m_testItem( 0 )
    , m_pudShown( false )
{
    DEBUG_BLOCK

    s_instance = this; // we are a singleton class

    // start context item manager
    ContextItemManager::instance()->setStartBox( 2 ); // HACK, for now we are telling it
    // that the contextview itself puts two boxes on the CV (true)
    
    setRenderHints( QPainter::Antialiasing );

    initiateScene();
    setAlignment( Qt::AlignTop );
    setCacheMode( QGraphicsView::CacheBackground ); // this won't be changing regularly

    setMouseTracking( true );

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
    clear();
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
    messageNotify( QString( "showHome" ) );
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

    for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it )
    {
        const QString album = *it;
        const QString artist = *++it;
        const QString year = *++it;
        const QString &cover = CollectionDB::instance()->albumImage( artist, album, false, 50 );
        albumBox->addAlbumInfo( cover, QString( "%1 - %2\n%3" ).arg( artist, album, year ) );
    }

    addContextBox( albumBox );

    // testing
//     QTimer::singleShot( 5000, this, SLOT( testBoxLayout() ) );
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

    if( m_testItem )
    {
        if( s_add )
            addContextBox( m_testItem, 1 );
        else
            removeContextBox( m_testItem );
    }

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
            box->ensureWidthFits( (qreal)newSize.width() );
    }
}

void ContextView::clear()
{
    DEBUG_BLOCK
    // tell member items that their boxes are being removed
    messageNotify( QString( "boxesRemoved" ) );

    m_contextBoxes.clear();
    delete m_contextScene;

    initiateScene();
    update();
}

bool ContextView::boxHigherThan( const ContextBox *c1, const ContextBox *c2 )
{
    // "Higher" means closer towards the top of the screen, but it means closer to position 0 (so really it's lower)
    return c1->sceneBoundingRect().bottom() < c2->sceneBoundingRect().bottom();
}

void ContextView::boxHeightChanged( qreal change )
{
    ContextBox *box = dynamic_cast<ContextBox*>( sender() );
    if( !box )
        return;

    QList<QGraphicsItem*> shuffle;

    int index = m_contextBoxes.indexOf( box );
    for( ++index; index < m_contextBoxes.size(); ++index )
        shuffle << m_contextBoxes.at(index);

    // 'change' will be negative if it got smaller
    shuffleItems( shuffle, change );
}

void ContextView::showPopupDropper()
{
    if( !enablePUD )
        return;
    DEBUG_BLOCK
    if( m_pudShown )
        return;

    while( !m_pudScalers.isEmpty() )
        delete m_pudScalers.takeFirst();

    while( !m_pudFaders.isEmpty() )
        delete m_pudFaders.takeFirst();

    foreach( ContextBox* box, m_contextBoxes )
    {
        
        if( eyeCandyFlag )
        {
            GraphicsItemFader *fader = new GraphicsItemFader( box );
            fader->setDuration( 1 );
            fader->setDelay( 1000 );
            fader->setStartAlpha( 255 );
            fader->setTargetAlpha( 0 );
            m_pudFaders.append( fader );
        }
        
        GraphicsItemScaler *scaler = new GraphicsItemScaler( box );
        scaler->setDuration( 1000 );
        scaler->setTargetSize( 1, 1);
        m_pudScalers.append( scaler );
    }

    foreach( GraphicsItemScaler* scaler, m_pudScalers )
        scaler->startScaling();
    foreach( GraphicsItemFader* fader, m_pudFaders )
        fader->startFading();

    m_pudShown = true;
}

void ContextView::hidePopupDropper()
{
    if( !enablePUD ) 
        return;
    DEBUG_BLOCK
    if( !m_pudShown )
        return;

    foreach( GraphicsItemFader* fader, m_pudFaders )
    {
        fader->setStartAlpha( 0 );
        fader->setTargetAlpha( 255 );
        fader->setDuration( 1000 );
        fader->setDelay( 100 );
    }

    foreach( GraphicsItemScaler* scaler, m_pudScalers )
    {
        scaler->setDuration( 1 );
        scaler->setTargetSize( scaler->originalWidth(), scaler->originalHeight() );
    }

    foreach( GraphicsItemFader* fader, m_pudFaders )
        fader->startFading();

    foreach( GraphicsItemScaler* scaler, m_pudScalers )
        scaler->startScaling();

    m_pudShown = false;
}

void ContextView::mouseMoveEvent( QMouseEvent *e )
{
    if( m_pudShown )
        hidePopupDropper();
    QGraphicsView::mouseMoveEvent( e );
}

void ContextView::removeContextBox( ContextBox *oldBox, bool fadeOut )
{
    DEBUG_BLOCK

    if( !oldBox || !m_contextScene )
        return;

    if( fadeOut )
    {
//         GraphicsItemFader *fader = new GraphicsItemFader( oldBox );
//         fader->setDuration( 2500 );
//         fader->setStartAlpha( 0 );
//         fader->setTargetAlpha( 255 );
//         fader->setFadeColor( palette().highlight().color() );
//         fader->startFading();
    }

    if( !m_contextBoxes.isEmpty() )
    {
        QList<QGraphicsItem*> shuffleUp;

        // need to shuffle up all the boxes below
        int index = m_contextBoxes.indexOf( oldBox );
        for( ++index; index < m_contextBoxes.size(); ++index )
        {
            ContextBox *box = m_contextBoxes.at(index);
            debug() << "shuffling up: " << box->title() << endl;
            shuffleUp << box;
        }

        qreal distance = oldBox->boundingRect().height() + BOX_PADDING;
        debug() << "shuffling " << shuffleUp.size() << " items up, a total of " << distance << endl;

        shuffleItems( shuffleUp, distance, ShuffleUp );
    }

    // no need to resort when removing
    m_contextBoxes.removeAll( oldBox );
    m_contextScene->removeItem( oldBox );
    disconnect( oldBox, SIGNAL( heightChanged(qreal) ), this, SLOT( boxHeightChanged(qreal) ) );

}

// Places a context box at the location specified by @param index. -1 -> at the bottom
void ContextView::addContextBox( ContextBox *newBox, int index, bool fadeIn )
{
    DEBUG_BLOCK

    if( !newBox || !m_contextScene )
        return;

    debug() << "Adding box: " << newBox->title() << endl;

    if( fadeIn )
    {
//         GraphicsItemFader *fader = new GraphicsItemFader( newBox );
//         connect( fader, SIGNAL( animationComplete() ), this, SLOT( faderItemFinished() ) );
//         fader->setDuration( 2500 );
//         fader->setStartAlpha( 255 );
//         fader->setTargetAlpha( 0 );
//         fader->setFadeColor( palette().highlight().color() );
//         fader->startFading();
//         newBox = fader;
    }

    qreal yposition = BOX_PADDING;

    if( !m_contextBoxes.isEmpty() )
    {
        if( index >= m_contextBoxes.count() )
            index = -1;

        debug() << "box count: " << m_contextBoxes.size() << endl;

        // special case 'add-to-end' index, -1.
        if( index < 0 )
            yposition = m_contextBoxes.last()->sceneBoundingRect().bottom() + BOX_PADDING;
        else
        {
            if( index > 0 ) // we need the bottom value for the box above it.
                yposition = m_contextBoxes.at( index - 1 )->sceneBoundingRect().bottom() + BOX_PADDING;

            debug() << "y-position: " << yposition << endl;

            QList<QGraphicsItem*> shuffleDown;

            // need to shuffle down all the boxes below
            for( int i = index; i < m_contextBoxes.size(); ++i )
                shuffleDown << m_contextBoxes.at( i );

            qreal distance = newBox->boundingRect().height() + BOX_PADDING;
            debug() << "shuffling " << shuffleDown.size() << " items down, a total of " << distance << endl;

            shuffleItems( shuffleDown, distance, ShuffleDown );
        }
    }

    debug() << "placing box at position: " << index << ", y position of box: " << yposition << endl;

    m_contextScene->addItem( newBox );
    newBox->setPos( BOX_PADDING, yposition );
    m_contextBoxes.append( newBox );

    // sort them based on bottom position
    qSort( m_contextBoxes.begin(), m_contextBoxes.end(), boxHigherThan );

    connect( newBox, SIGNAL( heightChanged(qreal) ), this, SLOT( boxHeightChanged(qreal) ) );

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
    
    messageNotify( QString(  "showCurrentTrack" ) );
}

#include "contextview.moc"

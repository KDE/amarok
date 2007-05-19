/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 *                :(C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> *
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
#include "textfader.h"
#include "collectiondb.h"
#include "contextbox.h"
#include "contextview.h"
#include "debug.h"
#include "enginecontroller.h"
#include "graphicsitemfader.h"
#include "introanimation.h"

#include <kstandarddirs.h>

#include <math.h> // scaleView()
#include <QBrush>
#include <QColor>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QWheelEvent>

//just for testing
#include <QGraphicsSvgItem>
#include <QSvgRenderer>

using namespace Context;

ContextView *ContextView::s_instance = 0;

ContextView::ContextView()
    : QGraphicsView()
    , EngineObserver( EngineController::instance() )
{
    s_instance = this; // we are a singleton class

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
    addContextBox( welcomeBox, -1, true );


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

    addContextBox( albumBox, -1, true );

    //testing

    debug() << KStandardDirs::locate("data", "amarok/images/amarok_icon.svg" ) << endl;
    QGraphicsSvgItem * svg = new QGraphicsSvgItem( KStandardDirs::locate("data", "amarok/images/amarok_icon.svg" ) );
    svg->scale(0.5, 0.5 );
    addContextBox( svg , -1, true );
}

void ContextView::showCurrentTrack()
{
    clear();

    MetaBundle bundle = EngineController::instance()->bundle();

    ContextBox *infoBox = new ContextBox();
    infoBox->setTitle( i18n("%1 - %2", bundle.title(), bundle.artist() ) );
    addContextBox( infoBox, -1, true );

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

    addContextBox( albumBox, -1, true );
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
    update();
}

void ContextView::addContextBox( QGraphicsItem *newBox, int after, bool fadeIn )
{
    if( !newBox || !m_contextScene )
        return;

    if( fadeIn )
    {
        GraphicsItemFader *fader = new GraphicsItemFader( newBox, 0 );
        fader->setDuration( 2500 );
        fader->setFPS( 30 );
        fader->setStartAlpha( 255 );
        fader->setTargetAlpha( 0 );
        fader->setFadeColor( palette().highlight() );
        fader->startFading();
        newBox = fader;
    }

    // For now, let's assume that all the items are listed in a vertical alignment
    // with a constant padding between the elements. Does this need to be more robust?
    QList<QGraphicsItem*> items = m_contextScene->items();
    qreal yposition = BOX_PADDING;

    if( !items.isEmpty() )
    {
        // Since the items are returned in no particular order, we must sort the items first
        // based on the topmost edge of the box.
        qSort( items );

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

#include "contextview.moc"

/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "CurrentTrack.h"

#include "amarok.h"
#include "debug.h"
#include "context/Svg.h"
#include "meta/MetaUtility.h"

#include <QPainter>
#include <QBrush>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QMap>

CurrentTrack::CurrentTrack( QObject* parent, const QVariantList& args )
    : Plasma::Applet( parent, args )
    , m_config( 0 )
    , m_configLayout( 0 )
    , m_width( 0 )
    , m_aspectRatio( 0.0 )
    , m_size( QSizeF() )
    , m_rating( -1 )
    , m_trackLength( 0 )
{
    DEBUG_BLOCK

    setHasConfigurationInterface( true );
    setDrawStandardBackground( true );

    dataEngine( "amarok-current" )->connectSource( "current", this );

    m_theme = new Context::Svg( "widgets/amarok-currenttrack", this );
    m_theme->setContentType( Context::Svg::SingleImage );
    m_width = globalConfig().readEntry( "width", 500 );

    m_title = new QGraphicsSimpleTextItem( this );
    m_artist = new QGraphicsSimpleTextItem( this );
    m_album = new QGraphicsSimpleTextItem( this );
    m_score = new QGraphicsSimpleTextItem( this );
    m_numPlayed = new QGraphicsSimpleTextItem( this );
    m_playedLast = new QGraphicsSimpleTextItem( this );
    m_albumCover = new QGraphicsPixmapItem( this );

    m_title->setBrush( QBrush( Qt::white ) );
    m_artist->setBrush( QBrush( Qt::white ) );
    m_album->setBrush( QBrush( Qt::white ) );
    m_score->setBrush( QBrush( Qt::white ) );
    m_numPlayed->setBrush( QBrush( Qt::white ) );
    m_playedLast->setBrush( QBrush( Qt::white ) );

    // get natural aspect ratio, so we can keep it on resize
    m_theme->resize();
    m_aspectRatio = (qreal)m_theme->size().height() / (qreal)m_theme->size().width();
    resize( m_width, m_aspectRatio );

    constraintsUpdated();
    updateGeometry();
}

CurrentTrack::~CurrentTrack()
{
    DEBUG_BLOCK
}

QSizeF CurrentTrack::contentSizeHint() const
{
    return m_size;
}

void CurrentTrack::constraintsUpdated()
{
    DEBUG_BLOCK
    prepareGeometryChange();
    // here we put all of the text items into the correct locations
    m_title->setPos( m_theme->elementRect( "track" ).topLeft() );
    m_artist->setPos( m_theme->elementRect( "artist" ).topLeft() );
    m_album->setPos( m_theme->elementRect( "album" ).topLeft() );
    m_score->setPos( m_theme->elementRect( "score" ).topLeft() );
    m_numPlayed->setPos( m_theme->elementRect( "numplayed" ).topLeft() );
    m_playedLast->setPos( m_theme->elementRect( "playedlast" ).topLeft() );
    m_albumCover->setPos( m_theme->elementRect( "albumart" ).topLeft() );

    QPixmap cover = m_albumCover->pixmap();
    cover = cover.scaledToWidth( m_theme->elementRect( "albumart" ).size().width(), Qt::SmoothTransformation );
    m_albumCover->setPixmap( cover );
//     debug() << "changing pixmap size from " << m_albumCover->pixmap().width() << " to " << cover.width();

    dataEngine( "amarok-current" )->setProperty( "coverWidth", m_theme->elementRect( "albumart" ).size().width() );
}

void CurrentTrack::updated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );

    if( data.size() == 0 ) return;

    QVariantMap currentInfo = data[ "current" ].toMap();
    kDebug() << "got data from engine: " << currentInfo;
    m_title->setText( currentInfo[ Meta::Field::TITLE ].toString() );
    m_artist->setText( currentInfo.contains( Meta::Field::ARTIST ) ? currentInfo[ Meta::Field::ARTIST ].toString() : QString() );
    m_album->setText( currentInfo.contains( Meta::Field::ALBUM ) ? currentInfo[ Meta::Field::ALBUM ].toString() : QString() );
//     m_rating = currentInfo[ 3 ].toInt();
    // TODO i can't add ratings... so hardcoding to test
    m_rating = 7;
    //m_rating = currentInfo[ Meta::Field::RATING ].toInt();
    m_score->setText( currentInfo[ Meta::Field::SCORE ].toString() );
    m_trackLength = currentInfo[ Meta::Field::LENGTH ].toInt();
//     m_playedLast->setText( Amarok::verboseTimeSince( currentInfo[ Meta::Field::LAST_PLAYED ].toInt() ) );

    // scale pixmap on demand
    QPixmap cover = m_albumCover->pixmap();
    cover = cover.scaledToWidth( m_theme->elementRect( "albumart" ).size().width(), Qt::SmoothTransformation );
    m_albumCover->setPixmap( cover );
    debug() << "changing pixmap size from " << m_albumCover->pixmap().width() << " to " << cover.width();

    m_numPlayed->setText( currentInfo[ Meta::Field::PLAYCOUNT ].toString() );
    m_albumCover->setPixmap( data[ "albumart" ].value<QPixmap>() );

}

void CurrentTrack::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    DEBUG_BLOCK
    Q_UNUSED( option );

    debug() << "painting currenttrack applet in:" << contentsRect;
    
    p->save();
    m_theme->paint( p, contentsRect, "background" );
    p->restore();

    p->save();
    int stars = m_rating / 2;
    for( int i = 1; i <= stars; i++ )
    {
        p->translate( m_theme->elementSize( "star" ).width(), 0 );
        m_theme->paint( p, m_theme->elementRect( "star" ), "star" );
    } //
    if( m_rating % 2 != 0 )
    { // paint a half star too
        m_theme->paint( p, m_theme->elementRect( "half-star" ), "half-star" );
    }
    p->restore();

    // TODO get, and then paint, album pixmap

}

void CurrentTrack::showConfigurationInterface()
{
    if (m_config == 0)
    {
        m_config = new KDialog();
        m_config->setCaption( i18n( "Configure Current Track Applet" ) );

        QWidget* widget = new QWidget( m_config );
        m_config->setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Apply );
        connect( m_config, SIGNAL(applyClicked()), this, SLOT(configAccepted()) );
        connect( m_config, SIGNAL(okClicked()), this, SLOT(configAccepted()) );

        m_configLayout = new QHBoxLayout( widget );
        m_spinWidth = new QSpinBox();
        m_spinWidth->setRange( 200, 700 );
        m_spinWidth->setValue( m_width );

        QLabel *labelWidth = new QLabel(i18n("Width"));
        m_configLayout->addWidget( labelWidth );
        m_configLayout->addWidget( m_spinWidth );

    }

    m_config->show();
}

void CurrentTrack::configAccepted() // SLOT
{
    KConfigGroup cg = globalConfig();

    m_width = m_spinWidth->value();
    resize( m_width, m_aspectRatio );

    cg.writeEntry( "width", m_width );

    cg.sync();
    constraintsUpdated();
}

void CurrentTrack::resize( qreal newWidth, qreal aspectRatio )
{
    DEBUG_BLOCK
    debug() << "resizing to:" << newWidth;
    qreal height = aspectRatio * newWidth;
    debug() << "setting size:" << m_size;
    m_size.setWidth( newWidth );
    m_size.setHeight( height );
    
    m_theme->resize( m_size );
    kDebug() << "set new size: " << m_size;
    constraintsUpdated();
}

#include "CurrentTrack.moc"

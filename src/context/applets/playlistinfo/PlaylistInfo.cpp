/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
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

#include "PlaylistInfo.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "context/Svg.h"

#include <QPainter>
#include <QBrush>
#include <QSpinBox>
#include <QLabel>

PlaylistInfo::PlaylistInfo( QObject* parent, const QStringList& args )
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

    dataEngine( "amarok-current" )->connectSource( "current", this );

    m_theme = new Context::Svg( this );
    m_theme->setImagePath( "widgets/amarok-playlistinfo" );
    m_theme->setContentType( Context::Svg::SingleImage );
    m_width = globalConfig().readEntry( "width", 500 );

    m_totalTracks = new QGraphicsSimpleTextItem( this );
    m_totalTime = new QGraphicsSimpleTextItem( this );
    m_totalSize = new QGraphicsSimpleTextItem( this );

    m_totalTracks->setBrush( QBrush( Qt::white ) );
    m_totalTime->setBrush( QBrush( Qt::white ) );
    m_totalSize->setBrush( QBrush( Qt::white ) );

    // get natural aspect ratio, so we can keep it on resize
    m_theme->resize();
    m_aspectRatio = (qreal)m_theme->size().height() / (qreal)m_theme->size().width();
    resize( m_width, m_aspectRatio );

    constraintsEvent();
}

PlaylistInfo::~PlaylistInfo()
{
    DEBUG_BLOCK
}

void PlaylistInfo::constraintsEvent()
{
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

void PlaylistInfo::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );

    if( data.size() == 0 ) return;

    QVariantList currentInfo = data[ "current" ].toList();
    kDebug() << "got data from engine: " << currentInfo;
    m_title->setText( currentInfo[ 1 ].toString() );
    m_artist->setText( currentInfo[ 0 ].toString() );
    m_album->setText( currentInfo[ 2 ].toString() );
//     m_rating = currentInfo[ 3 ].toInt();
    // TODO i can't add ratings... so hardcoding to test
    m_rating = 6;
    m_score->setText( currentInfo[ 4 ].toString() );
    m_trackLength = currentInfo[ 5 ].toInt();
//     m_playedLast->setText( Amarok::verboseTimeSince( currentInfo[ 6 ].toInt() ) );

    // scale pixmap on demand
    QPixmap cover = m_albumCover->pixmap();
    cover = cover.scaledToWidth( m_theme->elementRect( "albumart" ).size().width(), Qt::SmoothTransformation );
    m_albumCover->setPixmap( cover );
//     debug() << "changing pixmap size from " << m_albumCover->pixmap().width() << " to " << cover.width();

    m_numPlayed->setText( currentInfo[ 7 ].toString() );
    m_albumCover->setPixmap( data[ "albumart" ].value<QPixmap>() );

}

void PlaylistInfo::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );

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

void PlaylistInfo::showConfigurationInterface()
{
    if (m_config == 0)
    {
        m_config = new KDialog();
        m_config->setCaption( i18n( "Configure Playlist Info Applet" ) );

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

void PlaylistInfo::configAccepted() // SLOT
{
    KConfigGroup cg = globalConfig();

    m_width = m_spinWidth->value();
    resize( m_width, m_aspectRatio );

    cg.writeEntry( "width", m_width );

    cg.sync();
    constraintsEvent();
}

void PlaylistInfo::resize( qreal newWidth, qreal aspectRatio )
{
    qreal height = aspectRatio * newWidth;
    m_size.setWidth( newWidth );
    m_size.setHeight( height );

    m_theme->resize( m_size );
    kDebug() << "set new size: " << m_size;
    constraintsEvent();
}

#include "PlaylistInfo.moc"


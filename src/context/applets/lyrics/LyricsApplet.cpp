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

#include "LyricsApplet.h"

#include "Amarok.h"
#include "debug.h"
#include "context/Svg.h"
#include "Theme.h"

#include <QGraphicsSimpleTextItem>
#include <QGraphicsProxyWidget>
#include <QTextEdit>
#include <QPainter>
#include <QPoint>

#include <KGlobalSettings>

LyricsApplet::LyricsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_header( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_lyricsLabel( 0 )
    , m_titleLabel( 0 )
    , m_artistLabel( 0 )
    , m_lyrics( 0 )
    , m_title( 0 )
    , m_artist( 0 )
{
    Context::Theme::self()->setApplication( "amarok" );
    setHasConfigurationInterface( false );
}

LyricsApplet::~ LyricsApplet()
{
    m_lyricsProxy->setWidget( 0 );
    delete m_lyricsProxy;
    m_lyricsProxy = 0;
    delete m_lyrics;
}

void LyricsApplet::init()
{

    dataEngine( "amarok-lyrics" )->connectSource( "lyrics", this );

    m_header = new Context::Svg( "widgets/amarok-lyrics", this );
    m_header->setContentType( Context::Svg::SingleImage );
    m_header->resize();
    m_headerAspectRatio = (qreal)m_header->size().height()
        / (qreal)m_header->size().width();
    setContentSize( m_header->size() );

    m_lyricsLabel = new QGraphicsSimpleTextItem( this );
    m_titleLabel = new QGraphicsSimpleTextItem( this );
    m_artistLabel = new QGraphicsSimpleTextItem( this );
    m_lyricsProxy = new QGraphicsProxyWidget( this );
    m_lyrics = new QTextEdit;
    m_lyrics->setReadOnly( true );
    m_lyricsProxy->setWidget( m_lyrics );
    m_title = new QGraphicsSimpleTextItem( this );
    m_artist = new QGraphicsSimpleTextItem( this );

    QFont labelFont;
    labelFont.setBold( true );
    labelFont.setPointSize( labelFont.pointSize() + 1 );
    labelFont.setStyleHint( QFont::Times );
    labelFont.setStyleStrategy( QFont::PreferAntialias );

    QFont textFont = QFont( labelFont );
    textFont.setBold( false );

    const QColor textColor = Plasma::Theme::self()->textColor();

    m_lyricsLabel->setBrush( textColor );
    m_lyricsLabel->setFont( labelFont );
    m_lyricsLabel->setText( i18n( "Lyrics" ) );

    m_titleLabel->setBrush( textColor );
    m_titleLabel->setFont( labelFont );
    m_titleLabel->setText( i18nc( "The name of the current playing song", "Title" ) + ':' );

    m_artistLabel->setBrush( textColor );
    m_artistLabel->setFont( labelFont );
    m_artistLabel->setText( i18n( "Artist" ) + ':' );

    m_lyrics->setTextColor( textColor );
    m_lyrics->setFont( textFont );
    m_title->setBrush( textColor );
    m_artist->setBrush( textColor );

    constraintsUpdated();
}

void LyricsApplet::constraintsUpdated( Plasma::Constraints constraints )
{
    prepareGeometryChange();

    if( constraints & Plasma::SizeConstraint && m_header )
        m_header->resize(contentSize().toSize());

    // align items
    m_lyricsLabel->setPos( m_header->elementRect( "title" ).topLeft());
    m_artistLabel->setPos( m_header->elementRect( "titlelabel" ).topLeft() );
    m_titleLabel->setPos( m_header->elementRect( "artistlabel" ).topLeft() );

    m_lyricsProxy->setPos( m_header->elementRect( "lyrics" ).topLeft() );
    m_title->setPos( m_header->elementRect( "lyricstrackname" ).topLeft() );
    m_artist->setPos( m_header->elementRect( "lyricsartist" ).topLeft() );

    QSizeF infoSize( m_header->elementRect( "lyrics" ).bottomRight().x() - m_header->elementRect( "lyrics" ).topLeft().x(),
                     m_header->elementRect( "lyrics" ).bottomRight().y() - m_header->elementRect( "lyrics" ).topLeft().y() );

    if ( infoSize.isValid() )
    {
        m_lyricsProxy->setMinimumSize( infoSize );
        m_lyricsProxy->setMaximumSize( infoSize );
    }
}

void LyricsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name )
    if( data.size() == 0 ) return;

    if( data.contains( "noscriptrunning" ) )
        m_lyrics->setPlainText( i18n( "No lyrics script is running!" ) );
    if( data.contains( "fetching" ) )
        m_lyrics->setPlainText( i18n( "Lyrics are being fetched." ) );
    else if( data.contains( "error" ) )
        m_lyrics->setPlainText( i18n( "Lyrics were not able to be downloaded. Please check your internet connection." ) );
    else if( data.contains( "suggested" ) )
        m_lyrics->setPlainText( i18n( "Todo.... show suggestions here!" ) );
    else if( data.contains( "lyrics" ) )
    {
        QVariantList lyrics  = data[ "lyrics" ].toList();
        m_title->setText( truncateTextToFit( lyrics[ 1 ].toString(), m_title->font(), m_header->elementRect( "lyricstrackname" ) ) );
        m_artist->setText( truncateTextToFit( lyrics[ 0 ].toString(), m_artist->font(), m_header->elementRect( "lyricsartist" ) ) );

        m_lyrics->setPlainText( lyrics[ 3 ].toString() );
//         m_lyrics->adjustSize();
//         m_lyricsProxy->resize( m_lyrics->size() );
    }
    else if( data.contains( "notfound" ) )
        m_lyrics->setPlainText( i18n( "There were no lyrics found for this track" ) );
    calculateHeight();
}

void LyricsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
    Q_UNUSED( p );

    QRectF tmp( 0, 0, contentsRect.width(), 0 );
    tmp.setHeight( contentsRect.width() * m_headerAspectRatio );
    //m_header->paint( p, tmp, "lyricsheader" );

    // align items
    m_lyricsLabel->setPos( m_header->elementRect( "title" ).topLeft());
    m_artistLabel->setPos( m_header->elementRect( "titlelabel" ).topLeft() );
    m_titleLabel->setPos( m_header->elementRect( "artistlabel" ).topLeft() );
    m_title->setPos( m_header->elementRect( "lyricstrackname" ).topLeft() );
    m_artist->setPos( m_header->elementRect( "lyricsartist" ).topLeft() );

    m_lyricsProxy->setPos( m_header->elementRect( "lyrics" ).topLeft() );
    m_lyricsProxy->show();
}

bool LyricsApplet::hasHeightForWidth() const
{
    return true;
}

qreal LyricsApplet::heightForWidth( qreal width ) const
{
    if( m_lyrics )
        return (width * m_headerAspectRatio) + m_lyricsProxy->boundingRect().height();
    else
        return 25; // enough for the error text
}


void LyricsApplet::calculateHeight()
{
    qreal lyricsheight = m_lyricsProxy->boundingRect().height();

    if( lyricsheight > m_header->elementRect( "lyrics" ).height() ) // too short
    {
        qreal expandBy = lyricsheight - m_header->elementRect( "lyrics" ).height();
        setContentSize( (int)size().width(), (int)size().height() + expandBy );
    } /*else if( lyricsheight < m_header->elementRect( "lyrics" ).height() )
    { // too long
        qreal shrinkBy = m_header->elementRect( "lyrics" ).height() - lyricsheight;
        debug() << "shrinking by:" << shrinkBy
            << "final height:" << m_size.height() - shrinkBy;
        m_size.setHeight( m_size.height() - shrinkBy );
    }*/
}

#include "LyricsApplet.moc"

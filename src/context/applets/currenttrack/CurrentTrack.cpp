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


#include <KIconLoader>


#include <QPainter>
#include <QBrush>
#include <QFont>
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
    , m_rating( -1 )
    , m_trackLength( 0 )
{
    DEBUG_BLOCK

    setHasConfigurationInterface( true );
}

CurrentTrack::~CurrentTrack()
{
    DEBUG_BLOCK
}

void CurrentTrack::init()
{
    DEBUG_BLOCK
    setDrawStandardBackground( false );
    dataEngine( "amarok-current" )->connectSource( "current", this );

    m_theme = new Context::Svg( "widgets/amarok-currenttrack", this );
    m_theme->setContentType( Context::Svg::SingleImage );
    m_width = globalConfig().readEntry( "width", 500 );

    KIconLoader iconLoader;

    m_titleLabel = new QGraphicsSimpleTextItem( i18n( "Track:" ) , this );
    m_artistLabel = new QGraphicsSimpleTextItem( i18n( "Artist:" ), this );
    m_albumLabel = new QGraphicsSimpleTextItem( i18n( "Album:" ), this );
    
    m_scoreLabel = new QGraphicsPixmapItem( QPixmap(iconLoader.loadIcon( "love", KIconLoader::Toolbar, KIconLoader::SizeLarge ) ), this );
    m_numPlayedLabel = new QGraphicsPixmapItem( QPixmap(iconLoader.loadIcon( "knotify", KIconLoader::Toolbar, KIconLoader::SizeLarge ) ), this );
    m_playedLastLabel = new QGraphicsPixmapItem( QPixmap(iconLoader.loadIcon( "month", KIconLoader::Toolbar, KIconLoader::SizeLarge ) ), this );

    m_scoreLabel->setTransformationMode( Qt::SmoothTransformation );
    m_numPlayedLabel->setTransformationMode( Qt::SmoothTransformation );
    m_playedLastLabel->setTransformationMode( Qt::SmoothTransformation );
    
    m_title = new QGraphicsSimpleTextItem( this );
    m_artist = new QGraphicsSimpleTextItem( this );
    m_album = new QGraphicsSimpleTextItem( this );
    m_score = new QGraphicsSimpleTextItem( this );
    m_numPlayed = new QGraphicsSimpleTextItem( this );
    m_playedLast = new QGraphicsSimpleTextItem( this );
    m_albumCover = new QGraphicsPixmapItem( this );
    
    m_titleLabel->setBrush( QBrush( Qt::white ) );
    m_artistLabel->setBrush( QBrush( Qt::white ) );
    m_albumLabel->setBrush( QBrush( Qt::white ) );
    //m_scoreLabel->setBrush( QBrush( Qt::white ) );
    //m_numPlayedLabel->setBrush( QBrush( Qt::white ) );
    //m_playedLastLabel->setBrush( QBrush( Qt::white ) );
    
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

}

void CurrentTrack::constraintsUpdated( Plasma::Constraints constraints )
{
    DEBUG_BLOCK


    prepareGeometryChange();

    if (constraints & Plasma::SizeConstraint && m_theme) {
        m_theme->resize(contentSize().toSize());
    }


    // here we put all of the text items into the correct locations
    m_titleLabel->setPos( m_theme->elementRect( "tracklabel" ).topLeft() );
    m_artistLabel->setPos( m_theme->elementRect( "artistlabel" ).topLeft() );
    m_albumLabel->setPos( m_theme->elementRect( "albumlabel" ).topLeft() );
    m_scoreLabel->setPos( m_theme->elementRect( "scorelabel" ).topLeft() );
    m_numPlayedLabel->setPos( m_theme->elementRect( "numplayedlabel" ).topLeft() );
    m_playedLastLabel->setPos( m_theme->elementRect( "playedlastlabel" ).topLeft() );

    m_title->setPos( m_theme->elementRect( "track" ).topLeft() );
    m_artist->setPos( m_theme->elementRect( "artist" ).topLeft() );
    m_album->setPos( m_theme->elementRect( "album" ).topLeft() );
    m_score->setPos( m_theme->elementRect( "score" ).topLeft() );
    m_numPlayed->setPos( m_theme->elementRect( "numplayed" ).topLeft() );
    m_playedLast->setPos( m_theme->elementRect( "playedlast" ).topLeft() );
    m_albumCover->setPos( m_theme->elementRect( "albumart" ).topLeft() );


    m_title->setFont( shrinkTextSizeToFit( m_title->text(), m_theme->elementRect( "track" ) ) );
    m_artist->setFont( shrinkTextSizeToFit( m_artist->text(), m_theme->elementRect( "artist" ) ) );
    m_album->setFont( shrinkTextSizeToFit( m_album->text(), m_theme->elementRect( "album" ) ) );
    m_score->setFont( shrinkTextSizeToFit( m_score->text(), m_theme->elementRect( "score" ) ) );
    m_numPlayed->setFont( shrinkTextSizeToFit( m_numPlayed->text(), m_theme->elementRect( "numplayed" ) ) );
    m_playedLast->setFont( shrinkTextSizeToFit( m_playedLast->text(), m_theme->elementRect( "playedlast" ) ) );
    
    m_titleLabel->setFont( m_title->font() );
    m_artistLabel->setFont( m_artist->font() );
    m_albumLabel->setFont( m_album->font() );


    //calc scale factor..
    m_scoreLabel->resetTransform ();
    m_numPlayedLabel->resetTransform ();
    m_playedLastLabel->resetTransform ();
    
    float currentHeight = m_scoreLabel->boundingRect().height();
    float desiredHeight = m_theme->elementRect( "scorelabel" ).height();

    float scaleFactor = desiredHeight / currentHeight;
    //float scaleFactor = currentHeight / desiredHeight;
    
    debug() << "scale factor: " << scaleFactor;
    
    m_scoreLabel->scale( scaleFactor, scaleFactor );
    m_numPlayedLabel->scale( scaleFactor, scaleFactor );
    m_playedLastLabel->scale( scaleFactor, scaleFactor );

    resizeCover(m_bigCover);

    debug() << "changing pixmap size from " << m_albumCover->pixmap().width() << " to " << m_theme->elementRect( "albumart" ).size().width();

    dataEngine( "amarok-current" )->setProperty( "coverWidth", m_theme->elementRect( "albumart" ).size().width() );

}

void CurrentTrack::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );

    if( data.size() == 0 ) return;

    QVariantMap currentInfo = data[ "current" ].toMap();
    kDebug() << "got data from engine: " << currentInfo;
    m_title->setText( currentInfo[ Meta::Field::TITLE ].toString() );
    m_artist->setText( currentInfo.contains( Meta::Field::ARTIST ) ? currentInfo[ Meta::Field::ARTIST ].toString() : QString() );
    m_album->setText( currentInfo.contains( Meta::Field::ALBUM ) ? currentInfo[ Meta::Field::ALBUM ].toString() : QString() );
    m_rating = currentInfo[ Meta::Field::RATING ].toInt();
    m_score->setText( currentInfo[ Meta::Field::SCORE ].toString() );
    m_trackLength = currentInfo[ Meta::Field::LENGTH ].toInt();
    m_playedLast->setText( Amarok::verboseTimeSince( currentInfo[ Meta::Field::LAST_PLAYED ].toUInt() ) );
    m_numPlayed->setText( currentInfo[ Meta::Field::PLAYCOUNT ].toString() );

    //scale pixmap on demand
    //store the big cover : avoid blur when resizing the applet
    m_bigCover = data[ "albumart" ].value<QPixmap>();

    if(!resizeCover(m_bigCover))
    {
        warning() << "album cover of current track is null, did you forget to call Meta::Album::image?";
    }
}

bool CurrentTrack::hasHeightForWidth() const
{
    return true;
}

qreal CurrentTrack::heightForWidth( qreal width ) const
{
    return width * m_aspectRatio;
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
//     constraintsUpdated();

}

void CurrentTrack::showConfigurationInterface()
{
    if (m_config == 0)
    {
        m_config = new KDialog();
        m_config->setCaption( i18n( "Configure Current Track Applet" ) );

        QWidget* widget = new QWidget( m_config );
        m_config->setMainWidget(widget);
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

// TODO abstract code into superclass so this is not duplicated in LastFmEvents, this is useful code for many applets

QFont CurrentTrack::shrinkTextSizeToFit( const QString& text, const QRectF& bounds )
{
    Q_UNUSED( text );
    int size = 12; // start here, shrink if needed
    QFont font( QString(), size, QFont::Light );
    font.setStyleHint( QFont::SansSerif );
    font.setStyleStrategy( QFont::PreferAntialias );
    
    QFontMetrics fm( font );
    while( fm.height() > bounds.height() + 4 )
    {
        if( size < 0 )
        {
            size = 5;
            break;
        }
        size--;
        fm = QFontMetrics( QFont( QString(), size ) );
    }
    
    // for aesthetics, we make it one smaller
    size--;

    QFont returnFont( QString(), size, QFont::Light );
    font.setStyleHint( QFont::SansSerif );
    font.setStyleStrategy( QFont::PreferAntialias );
    
    return QFont( returnFont );
}

// returns truncated text with ... appended.
QString CurrentTrack::truncateTextToFit( QString text, const QFont& font, const QRectF& bounds )
{
    QFontMetrics fm( font );
    while( fm.width( text) > bounds.width() )
    {
        text.chop( 4 );
        text += "...";
    }
    return text;
}

bool CurrentTrack::resizeCover(QPixmap cover){
    if( !cover.isNull() )
    {
        QSize rectSize = m_theme->elementRect( "albumart" ).size();
        debug() << "getting album rect:" <<  rectSize;
        int size = qMin( rectSize.width(), rectSize.height() );
        qreal pixmapRatio = (qreal)cover.width()/size;

        if(cover.height()/pixmapRatio > rectSize.height())
            cover = cover.scaledToHeight( size, Qt::SmoothTransformation );
        else
            cover = cover.scaledToWidth( size, Qt::SmoothTransformation );

        //center the cover : if the cover is not squared, we get the missing pixels and center
        qreal moveBy = qAbs(cover.rect().width()-cover.rect().height())/2.0;
        m_albumCover->setPos(m_albumCover->x()+ moveBy, m_albumCover->y());

        m_albumCover->setPixmap( cover );
        return true;
    }
    else
    {
        return false;
    }
}

#include "CurrentTrack.moc"

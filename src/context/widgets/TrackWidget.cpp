/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "TrackWidget.h"

#include "Amarok.h"
#include "Debug.h"
#include "core/meta/support/MetaUtility.h"
#include "PaletteHandler.h"
#include "playlist/PlaylistModelStack.h"

#include <plasma/widgets/iconwidget.h>

#include <KIcon>

#include <QFont>
#include <QFontMetricsF>


TrackWidget::TrackWidget( QGraphicsItem *parent )
    : ToolBoxIcon( parent, 0.4 ) // second argument = opacity
    , m_track( 0 )
    , m_rating( new RatingWidget( this ) )
{
    m_scoreLabel = new QGraphicsSimpleTextItem( i18nc( "Score of a track", "Score:" ), this );
    
    m_scoreText = new QGraphicsSimpleTextItem( this );
    m_scoreText->setCursor( Qt::ArrowCursor );
    
    QFont font;
    font.setBold( false );
    font.setPointSize( font.pointSize() - 2 );
    font.setStyleStrategy( QFont::PreferAntialias );

    m_scoreText->setFont( font );
    m_scoreText->setBrush( PaletteHandler::highlightColor().darker( 200 ) );
    m_scoreText->show();

    m_scoreLabel->setFont( font );
    m_scoreLabel->setBrush( PaletteHandler::highlightColor().darker( 150 ) );
    m_scoreLabel->show();

    setBrush( PaletteHandler::highlightColor().darker( 200 ) );
    
    setDrawBackground( true );
    m_rating->setSpacing( 2 );

    connect( m_rating, SIGNAL( ratingChanged( int ) ), SLOT( changeTrackRating( int ) ) );
}

TrackWidget::~TrackWidget()
{}

void
TrackWidget::changeTrackRating( int rating )
{
    if( m_track )
        m_track->setRating( rating );
}

void
TrackWidget::hide()
{
    //HACK: since the text is not being hidden until the applet is resized we set the text to an empty string
    setText( QString() );
    ToolBoxIcon::hide();
}

void
TrackWidget::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    m_scoreText->setBrush( The::paletteHandler()->palette().highlightedText() );
    m_scoreLabel->setBrush( The::paletteHandler()->palette().highlightedText() );
    ToolBoxIcon::hoverEnterEvent( event );
}

void
TrackWidget::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
    m_scoreText->setBrush( PaletteHandler::highlightColor().darker( 200 ) );
    m_scoreLabel->setBrush( PaletteHandler::highlightColor().darker( 150 ) );
    ToolBoxIcon::hoverLeaveEvent( event );
}

void
TrackWidget::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
{
    Q_UNUSED( event )
    The::playlistController()->insertOptioned( m_track, Playlist::Append );
}

void
TrackWidget::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    painter->setRenderHint( QPainter::Antialiasing );
    int ratingXPos = contentsRect().width() - PADDING - m_rating->size().width();
    m_rating->setPos( ratingXPos, contentsRect().height() / 2 - m_rating->size().height() / 2 - 2 );
    m_rating->setMinimumSize( contentsRect().width() / 5, contentsRect().height() - PADDING );
    m_rating->setMaximumSize( contentsRect().width() / 5, contentsRect().height() - PADDING );

    QFontMetrics fm( m_scoreText->font() );
    m_scoreText->setPos( ratingXPos - m_scoreText->boundingRect().width() - PADDING,
                         boundingRect().height() / 2 - fm.boundingRect( m_scoreText->text() ).height() / 2 );
    m_scoreLabel->setPos( m_scoreText->pos().x() - m_scoreLabel->boundingRect().width() - PADDING,
                          boundingRect().height() / 2 - fm.boundingRect( m_scoreLabel->text() ).height() / 2 );
    ToolBoxIcon::paint( painter, option, widget );
}

void
TrackWidget::setTrack( Meta::TrackPtr track )
{
    m_track = track;
    m_rating->setRating( track->rating() );
    m_scoreText->setText( QString("%1").arg( int( track->score() ) ) );
}

void
TrackWidget::show()
{
    // As a consequence of the hide() HACK now we have to re-set the text to display.
    if( m_track )
    {
        const QString playedLast = Amarok::verboseTimeSince( m_track->lastPlayed() );
        const QString fullText( i18n( "%1 - %2 (%3)", m_track->artist()->prettyName(), m_track->prettyName(), playedLast ) );
        const QFontMetricsF fm( font() );

        int rightMargin = m_scoreLabel->boundingRect().width() + m_scoreText->boundingRect().width() + \
                            m_rating->size().width() + PADDING;

        setText( fm.elidedText( fullText, Qt::ElideRight, contentsRect().width() - rightMargin ) );
    }

    ToolBoxIcon::show();
}

Meta::TrackPtr
TrackWidget::track() const
{
    return m_track;
}

#include "TrackWidget.moc"


/****************************************************************************************
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "FirstRunTutorialPage.h"

#include "core/support/Debug.h"

#include <KColorScheme>
#include <KPushButton>

#include <plasma/paintutils.h>

#include <QGraphicsProxyWidget>
#include <QGraphicsSimpleTextItem>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>


static const int   PADDING = 0.4;
static const float TOOLBOX_OPACITY = 0.4;


FirstRunTutorialPage::FirstRunTutorialPage()
    : QGraphicsWidget()
    , m_animOpacity( TOOLBOX_OPACITY )
{
    DEBUG_BLOCK

    m_text = new QGraphicsTextItem( this );
    m_text->setCursor( Qt::ArrowCursor ); // Don't show the carot, the text isn't editable.

    QFont font;
    font.setBold( true );
    font.setStyleHint( QFont::Times );
    font.setPointSize( font.pointSize() + 4 );
    font.setStyleStrategy( QFont::PreferAntialias );

    m_text->setFont( font );
    m_text->setDefaultTextColor( Qt::white );
    m_text->show();

    KPushButton* button = new KPushButton( i18n( "Close" ) );
    button->setAttribute( Qt::WA_NoSystemBackground );  // Removes ugly rectangular border
    connect( button, SIGNAL( clicked() ), this, SIGNAL( pageClosed() ) );

    m_closeButton = new QGraphicsProxyWidget( this );
    m_closeButton->setWidget( button);
}

FirstRunTutorialPage::~FirstRunTutorialPage()
{
    DEBUG_BLOCK
}


void
FirstRunTutorialPage::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    Q_UNUSED( option )
    Q_UNUSED( widget )
    m_text->setTextWidth( size().width() - 50 ); // Important: Without it, <center> does not work in the HTML

    QString htmlText(  // TODO: Make this i18n() once the text is finalized
       "<center>"
         "<br/>"
         "<br/>"
         "<br/>"
         "<h2>Welcome to Amarok 2.1!</h2>"
         "<p>"
         "Amarok is a powerful music player for Linux and Unix, MacOS X and Windows with an intuitive interface."
         "<p>"
         "It makes playing the music you love and discovering new music easier than ever before - and it looks good doing it!"
         "</p>"
         "<br/>"
         "<br/>"
         "<p>"
           "Discover what Amarok has to offer."
         "<p>"
       "</center>"
    );
    m_text->setHtml( htmlText );

    const QFontMetricsF fm( m_text->font() );
    //m_text->setPos( size().width() / 2 - fm.boundingRect( m_text->toPlainText() ).width() / 2, size().height() / 2 - fm.boundingRect( m_text->toPlainText() ).height() / 2 );
    m_closeButton->setPos( size().width() - 100, size().height() - 50 ); 

    painter->save();

    QColor color = KColorScheme( QPalette::Active, KColorScheme::Window, Plasma::Theme::defaultTheme()->colorScheme() ).background().color();

    QSize innerRectSize( size().width() - 7, size().height() - 7 );
    QPainterPath innerRect( Plasma::PaintUtils::roundedRectangle( QRectF( QPointF( 2.5, 2.5 ), innerRectSize ), 8 ) );

    painter->setBrush( color );
    painter->setRenderHint( QPainter::Antialiasing );
    painter->setOpacity( opacity() * m_animOpacity );
    painter->setPen( QPen( Qt::gray, 1 ) );
    painter->drawPath( innerRect );
    painter->restore();

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );
    painter->setPen( QPen( Qt::white, 4 ) );
    painter->drawPath( innerRect );
    painter->restore();
}

void
FirstRunTutorialPage::triggerResize( const QRectF& rect )
{
    DEBUG_BLOCK

    setGeometry( rect );
}

#include "FirstRunTutorialPage.moc"


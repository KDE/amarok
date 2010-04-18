/****************************************************************************************
 * Copyright (c) 2009 Jeff Mitchell <mitchell@kde.org>                                  *
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

#include "SongkickApplet.h"

#include "core/support/Amarok.h"
#include "App.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "dialogs/ScriptManager.h"
#include "core/meta/Meta.h"
#include "PaletteHandler.h"
#include "Theme.h"

#include <KGlobalSettings>
#include <KStandardDirs>

#include <QAction>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsProxyWidget>
#include <QLinearGradient>
#include <QTextBrowser>
#include <QPainter>
#include <QPoint>

SongkickApplet::SongkickApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_titleText( i18n("Songkick Concert Information") )
    , m_titleLabel( 0 )
    , m_reloadIcon( 0 )
    , m_songkick( 0 )
{
    setHasConfigurationInterface( false );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

SongkickApplet::~ SongkickApplet()
{
    m_songkickProxy->setWidget( 0 );
    delete m_songkickProxy;
    m_songkickProxy = 0;
    delete m_songkick;
}

void SongkickApplet::init()
{
    QColor highlight = PaletteHandler::highlightColor().darker( 300 );

    m_titleLabel = new QGraphicsSimpleTextItem( i18n("Concerts"), this );
    QFont bigger = m_titleLabel->font();
    bigger.setPointSize( bigger.pointSize() + 2 );
    m_titleLabel->setFont( bigger );
    m_titleLabel->setZValue( m_titleLabel->zValue() + 100 );
   // m_titleLabel->setBrush( highlight );

    QAction* reloadAction = new QAction( i18n("Reload Songkick"), this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( true );
    m_reloadIcon = addAction( reloadAction );

    connect( m_reloadIcon, SIGNAL( clicked() ), dataEngine( "amarok-songkick" ), SLOT( update() ) );

    m_songkickProxy = new QGraphicsProxyWidget( this );
    m_songkick = new QTextBrowser;
    m_songkick->setAttribute( Qt::WA_NoSystemBackground );
    m_songkick->setReadOnly( true );
    m_songkick->setOpenExternalLinks( true );
    m_songkick->setTextInteractionFlags( Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard );
    m_songkickProxy->setWidget( m_songkick );
    QPalette pal;
    QBrush brush(  PaletteHandler::highlightColor().lighter( 170 ) );
    brush.setStyle( Qt::SolidPattern );
    pal.setBrush( QPalette::Active, QPalette::Base, brush );
    pal.setBrush( QPalette::Inactive, QPalette::Base, brush );
    pal.setBrush( QPalette::Disabled, QPalette::Base, brush );
    pal.setBrush( QPalette::Window, brush );
    m_songkick->setPalette( pal );
    m_songkickProxy->setPalette( pal );
    m_songkick->setStyleSheet( QString( "QTextBrowser { background-color: %1; border-width: 0px; border-radius: 0px; color: %2; }" )
                                    .arg( PaletteHandler::highlightColor().lighter( 150 ).name() )
                                    .arg( PaletteHandler::highlightColor().darker( 400 ).name() ) );

    connect( dataEngine( "amarok-songkick" ), SIGNAL( sourceAdded( const QString& ) ), this, SLOT( connectSource( const QString& ) ) );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );

    constraintsEvent();
    connectSource( "ontour" );
    connectSource( "dates" );
}

void SongkickApplet::connectSource( const QString& source )
{
    if( source == "ontour" )
    {
        dataEngine( "amarok-songkick" )->connectSource( source, this );
        dataUpdated( source, dataEngine("amarok-songkick" )->query( "ontour" ) );
    }
    else if( source == "dates" )
    {
        dataEngine( "amarok-songkick" )->connectSource( source, this );
        dataUpdated( source, dataEngine("amarok-songkick" )->query( "dates" ) );
    }
}

void SongkickApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );

    prepareGeometryChange();

    QRectF rect = boundingRect();
    rect.setWidth( rect.width() - 30 );
    m_titleLabel->setText( truncateTextToFit( m_titleText, m_titleLabel->font(), rect ) );
    m_titleLabel->setPos( (size().width() - m_titleLabel->boundingRect().width() ) / 2, standardPadding() + 2 );

    m_reloadIcon->setPos( size().width() - m_reloadIcon->size().width() - standardPadding(), standardPadding() );
    m_reloadIcon->show();

    //m_songkickProxy->setPos( 0, m_reloadIcon->size().height() );
    m_songkickProxy->setPos( standardPadding(), m_titleLabel->pos().y() + m_titleLabel->boundingRect().height() + standardPadding() );
    QSize songkickSize( size().width() - 2 * standardPadding(), boundingRect().height() - m_songkickProxy->pos().y() - standardPadding() );
    m_songkickProxy->setMinimumSize( songkickSize );
    m_songkickProxy->setMaximumSize( songkickSize );
}

void SongkickApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    if( data.size() == 0 ) return;

    debug() << "songkick applet got name:" << name << "and data: " << data;

    m_titleLabel->show();
    if( data.contains( "fetching" ) )
    {
        m_songkick->show();
        m_songkick->setPlainText( i18n("Concert information is being fetched.") );
    }
    else if( data.contains( "error" ) )
    {
        m_songkick->show();
        m_songkick->setPlainText( i18n( "Songkick was not able to be downloaded. Please check your Internet connection: %1", data["error"].toString() ) );
    }
    else if( data.contains( "suggested" ) )
    {
        m_songkick->hide();
        QVariantList suggested = data[ "suggested" ].toList();
        // build simple HTML to show
        // a list
        QString html = QString( "<br><br>" );
        foreach( const QVariant &suggestion, suggested )
        {
                QString sug = suggestion.toString();
                //debug() << "parsing suggestion:" << sug;
                QStringList pieces = sug.split( " - " );
                const QString link = QString( "<a href=\"%1|%2|%3\">%4 - %5</a><br>" )
                                        .arg( pieces[0] )
                                        .arg( pieces[1] )
                                        .arg( pieces[2] )
                                        .arg( pieces[1] )
                                        .arg( pieces[0] );
                html += link;
        }
        //debug() << "setting html: " << html;
        //m_suggested->setHtml( html );
        //m_suggested->show();
    }
    else if( data.contains( "html" ) )
    {
        // show pure html in the text area
        m_songkick->setHtml( data[ "html" ].toString() );
        m_songkick->show();
    }
    else if( data.contains( "lyrics" ) )
    {
        m_songkick->show();
        QVariantList lyrics  = data[ "lyrics" ].toList();

        m_titleText = QString( "Songkick: %1 - %2" ).arg( lyrics[0].toString() )
                                                    .arg( lyrics[1].toString() );
        //  need padding for title
        m_songkick->setPlainText( lyrics[ 3 ].toString().trimmed() );
    }
    else if( data.contains( "notfound" ) )
    {
        m_songkick->show();
        m_songkick->setPlainText( i18n("There was no information found for this track" ));
    }
    setPreferredSize( (int)size().width(), (int)size().height() );
    updateConstraints();
    update();
}

bool SongkickApplet::hasHeightForWidth() const
{
    return false;
}

void
SongkickApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );
    p->setRenderHint( QPainter::Antialiasing );

    // tint the whole applet
    addGradientToAppletBackground( p );

    // draw rounded rect around title
    drawRoundedRectAroundText( p, m_titleLabel );

    //draw background of lyrics text
    p->save();
    QColor highlight( App::instance()->palette().highlight().color() );
    highlight.setHsvF( highlight.hueF(), 0.07, 1, highlight.alphaF() );

    QRectF songkickRect = m_songkickProxy->boundingRect();
    songkickRect.moveTopLeft( m_songkickProxy->pos() );
    QPainterPath path;
    path.addRoundedRect( songkickRect, 5, 5 );
    p->fillPath( path , highlight );
    p->restore();

}

QSizeF SongkickApplet::sizeHint(Qt::SizeHint which, const QSizeF & constraint) const
{
    Q_UNUSED( which );

 /*   if( m_songkick )
    {
        debug() << "returning sizehint height of" << m_songkick->sizeHint().height();
    //     return QSizeF( constraint.width(), m_songkickProxy->sizeHint().height() );
        if( m_textHeight > 0 )
            return QGraphicsWidget::sizeHint( which, constraint );

    } else
        return QGraphicsWidget::sizeHint( which, constraint ); */
    return QSizeF( QGraphicsWidget::sizeHint( which, constraint ).width(), 500 );
}

void
SongkickApplet::paletteChanged( const QPalette & palette )
{
    Q_UNUSED( palette )

    QColor highlight = PaletteHandler::highlightColor().darker( 200 );
    if( m_songkick )
        m_songkick->setStyleSheet(
                        QString( "QTextBrowser { background-color: %1; border-width: 0px; border-radius: 0px; color: %2; }" )
                            .arg( PaletteHandler::highlightColor().lighter( 150 ).name() )
                            .arg( PaletteHandler::highlightColor().darker( 400 ).name() )
                            );
}

#include "SongkickApplet.moc"

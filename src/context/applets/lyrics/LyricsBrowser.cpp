/****************************************************************************************
 * Copyright (c) 2011 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#include "LyricsBrowser.h"

#include "PaletteHandler.h"

#include <QApplication>
#include <QTextBlock>
#include <KTextBrowser>
#include <Plasma/Svg>
#include <Plasma/SvgWidget>

#include <QGraphicsSceneResizeEvent>

LyricsBrowser::LyricsBrowser( QGraphicsWidget *parent )
    : Plasma::TextBrowser( parent )
    , m_isRichText( true )
    , m_alignment( Qt::AlignLeft )
    , m_topBorder( new Plasma::SvgWidget( this ) )
    , m_bottomBorder( new Plasma::SvgWidget( this ) )
{
    KTextBrowser *native = nativeWidget();
    native->setOpenExternalLinks( true );
    native->setUndoRedoEnabled( true );
    native->setAutoFillBackground( false );
    native->setReadOnly( false );
    native->setWordWrapMode( QTextOption::WordWrap );
    native->setCursorWidth( 0 );
    native->document()->setDocumentMargin( 10 );
    native->viewport()->setAutoFillBackground( true );
    native->setTextInteractionFlags( Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard );

    Plasma::Svg *borderSvg = new Plasma::Svg( this );
    borderSvg->setImagePath( QLatin1String("widgets/scrollwidget") );

    m_topBorder->setSvg( borderSvg );
    m_topBorder->setElementID( QLatin1String("border-top") );
    m_topBorder->setZValue( 900 );

    m_bottomBorder->setSvg( borderSvg );
    m_bottomBorder->setElementID( QLatin1String("border-bottom") );
    m_bottomBorder->setZValue( 900 );

    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(paletteChanged(QPalette)) );
    paletteChanged( The::paletteHandler()->palette() );
}

LyricsBrowser::~LyricsBrowser()
{}

Qt::Alignment LyricsBrowser::alignment() const
{
    return m_alignment;
}

bool LyricsBrowser::isReadOnly() const
{
    return nativeWidget()->isReadOnly();
}

bool LyricsBrowser::isRichText() const
{
    return m_isRichText;
}

QString LyricsBrowser::lyrics() const
{
    return m_isRichText ? nativeWidget()->toHtml() : nativeWidget()->toPlainText();
}

void LyricsBrowser::clear()
{
    nativeWidget()->clear();
}

void LyricsBrowser::setAlignment( Qt::Alignment alignment )
{
    if( m_alignment == alignment )
        return;

    m_alignment = alignment;
    updateAlignment();
}

void LyricsBrowser::setLyrics( const QString &lyrics )
{
    KTextBrowser *w = nativeWidget();
    m_isRichText ?  w->setHtml( lyrics ) : w->setPlainText( lyrics );
    updateAlignment();
}

void LyricsBrowser::setReadOnly( bool readOnly )
{
    QPalette::ColorRole bg = readOnly ? QPalette::Base : QPalette::AlternateBase;
    nativeWidget()->viewport()->setBackgroundRole( bg );
    nativeWidget()->setReadOnly( readOnly );
    nativeWidget()->setCursorWidth( !readOnly ? 1 : 0 );
}

void LyricsBrowser::setRichText( bool richText )
{
    m_isRichText = richText;
}

void LyricsBrowser::paletteChanged( const QPalette &palette )
{
    QPalette p = palette;
    // set text color using app theme instead of plasma theme
    p.setColor( QPalette::Text, qApp->palette().text().color() );

    QPalette::ColorRole bg = isReadOnly() ? QPalette::Base : QPalette::AlternateBase;
    nativeWidget()->viewport()->setBackgroundRole( bg );
    nativeWidget()->setPalette( p );
}

void LyricsBrowser::updateAlignment()
{
    QTextCursor it( nativeWidget()->document()->firstBlock() );
    if( !it.block().isValid() )
        return;

    do
    {
        QTextBlockFormat fmt = it.blockFormat();
        fmt.setAlignment( m_alignment );
        it.setBlockFormat( fmt );
    } while ( it.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor) );
}

void LyricsBrowser::resizeEvent( QGraphicsSceneResizeEvent *event )
{
    Plasma::TextBrowser::resizeEvent( event );
    if( event->newSize() == event->oldSize() )
        return;

    if( m_topBorder && m_topBorder->isVisible() )
    {
        qreal newWidth = event->newSize().width();
        m_topBorder->resize( newWidth, m_topBorder->size().height() );
        m_bottomBorder->resize( newWidth, m_bottomBorder->size().height() );
        m_topBorder->setPos( boundingRect().topLeft() );
        QPointF bottomPoint = boundingRect().bottomLeft();
        bottomPoint.ry() -= m_bottomBorder->size().height();
        m_bottomBorder->setPos( bottomPoint );
    }
}


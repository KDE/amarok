/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2009 simon.esneault <simon.esneault@gmail.com>                         *
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

#include "LyricsApplet.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "EngineController.h"
#include "context/widgets/TextScrollingWidget.h"
#include "dialogs/ScriptManager.h"
#include "meta/Meta.h"
#include "PaletteHandler.h"
#include "Theme.h"

#include <KConfigDialog>
#include <KGlobalSettings>
#include <KStandardDirs>
#include <KMessageBox>
#include <KTabWidget>

#include <Plasma/IconWidget>
#include <Plasma/Containment>

#include <QAction>
#include <QGraphicsProxyWidget>
#include <QLinearGradient>
#include <QTextBrowser>
#include <QPainter>
#include <QPoint>
#include <QScrollBar>

LyricsApplet::LyricsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_titleText( i18n( "Lyrics" ) )
    , m_titleLabel( 0 )
    , m_saveIcon( 0 )
    , m_editIcon( 0 )
    , m_reloadIcon( 0 )
    , m_closeIcon( 0 )
    , m_proxy( 0 )
    , m_lyrics( 0 )
    , m_suggested( 0 )
    , m_hasLyrics( false )
    , m_isRichText( true )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

LyricsApplet::~ LyricsApplet()
{
    m_proxy->setWidget( 0 );
    delete m_proxy;
    delete m_lyrics;
    delete m_suggested;
}

void LyricsApplet::init()
{
    // properly set the size, asking for the whole cv size.
    resize( 500, -1 );

    m_titleLabel = new TextScrollingWidget( this );
    QFont bigger = m_titleLabel->font();
    bigger.setPointSize( bigger.pointSize() + 2 );
    m_titleLabel->setFont( bigger );
    m_titleLabel->setZValue( m_titleLabel->zValue() + 100 );
    m_titleLabel->setText( i18n( "Lyrics" ) );

    QAction* editAction = new QAction( this );
    editAction->setIcon( KIcon( "document-edit" ) );
    editAction->setVisible( true );
    editAction->setEnabled( false );
    editAction->setText( i18n( "Edit Lyrics" ) );
    m_editIcon = addAction( editAction );

    connect( editAction, SIGNAL( activated() ), this, SLOT( editLyrics() ) );

    QAction* closeAction = new QAction( this );
    closeAction->setIcon( KIcon( "document-close" ) );
    closeAction->setVisible( false );
    closeAction->setEnabled( false );
    closeAction->setText( i18n( "Close" ) );
    m_closeIcon = addAction( closeAction );

    connect( closeAction, SIGNAL( activated() ), this, SLOT( closeLyrics() ) );

    QAction* saveAction = new QAction( this );
    saveAction->setIcon( KIcon( "document-save" ) );
    saveAction->setVisible( false );
    saveAction->setEnabled( false );
    saveAction->setText( i18n( "Save Lyrics" ) );
    m_saveIcon = addAction( saveAction );

    connect( saveAction, SIGNAL( activated() ), this, SLOT( saveLyrics() ) );

    QAction* reloadAction = new QAction( this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( true );
    reloadAction->setText( i18n( "Reload Lyrics" ) );
    m_reloadIcon = addAction( reloadAction );

    connect( reloadAction, SIGNAL( activated() ), this, SLOT( refreshLyrics() ) );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setVisible( true );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    m_settingsIcon = addAction( settingsAction );

    connect( settingsAction, SIGNAL( activated() ), this, SLOT( showConfigurationInterface() ) );

    m_proxy = new QGraphicsProxyWidget( this );
    m_proxy->setAttribute( Qt::WA_NoSystemBackground );

    m_lyrics = new QTextBrowser;
    m_lyrics->setAttribute( Qt::WA_NoSystemBackground );
    m_lyrics->setOpenExternalLinks( true );
    m_lyrics->setUndoRedoEnabled( true );
    m_lyrics->setAutoFillBackground( false );
    m_lyrics->setWordWrapMode( QTextOption::WordWrap );
    m_lyrics->viewport()->setAttribute( Qt::WA_NoSystemBackground );
    m_lyrics->setTextInteractionFlags( Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard );

    m_suggested = new QTextBrowser;
    m_suggested->setAttribute( Qt::WA_NoSystemBackground );
    m_suggested->setOpenExternalLinks( true );
    m_suggested->setAutoFillBackground( false );
    m_suggested->setWordWrapMode( QTextOption::WordWrap );
    m_suggested->viewport()->setAttribute( Qt::WA_NoSystemBackground );
    m_suggested->viewport()->setCursor( Qt::PointingHandCursor );

    // Read config
    KConfigGroup config = Amarok::config("Lyrics Applet");
    QFont font;
    bool fontGood = font.fromString( config.readEntry( "Font", QString() ) );
    if( fontGood )
        m_lyrics->setFont( font );

    connect( m_suggested, SIGNAL( anchorClicked( const QUrl& ) ), this, SLOT( suggestionChosen( const QUrl& ) ) );
    connect( dataEngine( "amarok-lyrics" ), SIGNAL( sourceAdded( const QString& ) ), this, SLOT( connectSource( const QString& ) ) );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );

    constraintsEvent();
    updateConstraints();
    connectSource( "lyrics" );

    setEditing( false );
    showLyrics();
}

void LyricsApplet::connectSource( const QString& source )
{
    if( source == "lyrics" )
    {
        dataEngine( "amarok-lyrics" )->connectSource( source, this );
        refreshLyrics(); // get data initially
    }
    else if( source == "suggested" )
    {
        dataEngine( "amarok-lyrics" )->connectSource( source, this );
        dataUpdated( source, dataEngine("amarok-lyrics" )->query( "suggested" ) ); 
    }
} 

void LyricsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );

    prepareGeometryChange();

    // Assumes all icons are of equal width
    const float iconWidth = m_settingsIcon->size().width();

    qreal widmax = boundingRect().width() - 2 * iconWidth * 4 - 6 * standardPadding();
    QRectF rect( ( boundingRect().width() - widmax ) / 2, 0 , widmax, 15 );

    m_titleLabel->setScrollingText( m_titleText, rect );
    m_titleLabel->setPos( ( size().width() - m_titleLabel->boundingRect().width() ) / 2 , standardPadding() + 3 );

    m_settingsIcon->setPos( size().width() - iconWidth - standardPadding(), standardPadding() );

    m_reloadIcon->setPos( m_settingsIcon->pos().x() - standardPadding() - iconWidth, standardPadding() );
    m_reloadIcon->show();

    QPoint editIconPos( m_reloadIcon->pos().x() - standardPadding() - iconWidth, standardPadding() );
    m_editIcon->setPos( editIconPos );
    m_closeIcon->setPos( editIconPos );

    m_saveIcon->setPos( m_editIcon->pos().x() - standardPadding() - iconWidth, standardPadding() );

    m_proxy->setPos( standardPadding(), m_titleLabel->pos().y() + m_titleLabel->boundingRect().height() + standardPadding() );

    const QSize textBrowserSize( size().width() - 2 * standardPadding(), boundingRect().height() - m_proxy->pos().y() - standardPadding() );

    m_proxy->setMinimumSize( textBrowserSize );
    m_proxy->setMaximumSize( textBrowserSize );

    update();
}

void LyricsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name )

    m_hasLyrics = false;

    if( data.size() == 0 ) return;

    //debug() << "got lyrics data: " << data;

    m_titleText = i18n( "Lyrics" );
    m_titleLabel->show();

    setBusy( false );
    
    if( data.contains( "noscriptrunning" ) )
    {
        m_lyrics->setPlainText( i18n( "No lyrics script is running." ) );
        showLyrics();
    }
    else if( data.contains( "stopped" ) )
    {
        m_lyrics->clear();
        showLyrics();
    }
    else if( data.contains( "fetching" ) )
    {

        if( canAnimate() )
            setBusy( true );

        m_titleText = i18n( "Lyrics : Fetching ..." );
        m_lyrics->setPlainText( i18n( "Lyrics are being fetched." ) );

        showLyrics();
    }
    else if( data.contains( "error" ) )
    {
        m_lyrics->setPlainText( i18n( "Could not download lyrics.\nPlease check your Internet connection.\nError message:\n%1", data["error"].toString() ) );
        showLyrics();
    }
    else if( data.contains( "suggested" ) )
    {
        QVariantList suggested = data[ "suggested" ].toList();
        // build simple HTML to show a list
        QString html;
        foreach( const QVariant &suggestion, suggested )
        {
            const QString sug = suggestion.toString();
            const QStringList pieces = sug.split( " - " );
            const QString link = QString( "<a href=\"%1|%2|%3\">%4 - %5</a><br>" ).arg( pieces[ 0 ] ).arg( pieces[ 1 ] ).arg( pieces[ 2 ] ).arg( pieces[ 1 ] ).arg( pieces[ 0 ] );
            html += link;
        }

        // remove the last <br> to save space
        html.remove( html.lastIndexOf( "<br>" ), 4 );

        m_suggested->setHtml( html );
        showSuggested();
    }
    else if( data.contains( "html" ) )
    {
        m_hasLyrics = true;
        m_isRichText = true;
        // show pure html in the text area
        m_lyrics->setHtml( data[ "html" ].toString() );
        m_titleText = QString( "%1 : %2" ).arg( i18n( "Lyrics" ) ).arg( data[ "html" ].toString().section( "<title>", 1, 1 ).section( "</title>", 0, 0 ) );

        showLyrics();
        emit sizeHintChanged(Qt::MaximumSize);
    }
    else if( data.contains( "lyrics" ) )
    {
        m_hasLyrics = true;
        m_isRichText = false;
        QVariantList lyrics  = data[ "lyrics" ].toList();

        m_titleText = QString( " %1 : %2 - %3" ).arg( i18n( "Lyrics" ) ).arg( lyrics[ 0 ].toString() ).arg( lyrics[ 1 ].toString() );
        //  need padding for title
        m_lyrics->setPlainText( lyrics[ 3 ].toString().trimmed() );
        showLyrics();

        // the following line is needed to fix the bug of the lyrics applet sometimes not being correctly resized.
        // I don't have the courage to put this into Applet::setCollapseOff(), maybe that would break other applets.
        emit sizeHintChanged(Qt::MaximumSize);
    }
    else if( data.contains( "notfound" ) )
    {
        m_lyrics->setPlainText( i18n( "There were no lyrics found for this track" ) );
        showLyrics();
    }

    update();
    constraintsEvent();
}

bool LyricsApplet::hasHeightForWidth() const
{
    return false;
}

void
LyricsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );
    p->setRenderHint( QPainter::Antialiasing );

    // tint the whole applet
    addGradientToAppletBackground( p );

    // draw rounded rect around title (only if not animating )
    if ( !m_titleLabel->isAnimating() )
        drawRoundedRectAroundText( p, m_titleLabel );

    QColor background;

    if( m_lyrics->isReadOnly() )
    {
        background = The::paletteHandler()->backgroundColor();
    }
    else
    {
        // different background color when we're in edit mode
        background = The::paletteHandler()->alternateBackgroundColor();
    }

    const QTextBrowser *browser = static_cast< QTextBrowser* >( m_proxy->widget() );

    p->save();

    const int frameWidth         = browser->frameWidth();
    const QScrollBar *hScrollBar = browser->horizontalScrollBar();
    const QScrollBar *vScrollBar = browser->verticalScrollBar();
    const qreal hScrollBarHeight = hScrollBar->isVisible() ? hScrollBar->height() + 2 : 0;
    const qreal vScrollBarWidth  = vScrollBar->isVisible() ? vScrollBar->width()  + 2 : 0;
    const QSizeF proxySize( m_proxy->size().width()  - vScrollBarWidth  - frameWidth * 2,
                            m_proxy->size().height() - hScrollBarHeight - frameWidth * 2 );
    const QPointF proxyPos( m_proxy->pos().x() + frameWidth,
                            m_proxy->pos().y() + frameWidth );
    const QRectF proxyRect( proxyPos, proxySize );

    QPainterPath path;
    path.addRoundedRect( proxyRect, 2, 2 );
    p->fillPath( path, background );
    p->restore();
}

void
LyricsApplet::paletteChanged( const QPalette & palette )
{
    Q_UNUSED( palette )
}

void
LyricsApplet::suggestionChosen( const QUrl& link )
{
    QStringList pieces = link.toString().split( '|' );
    ScriptManager::instance()->notifyFetchLyricsByUrl( pieces[ 1 ], pieces[ 0 ], pieces[ 2 ] );
}

void
LyricsApplet::refreshLyrics()
{
    Meta::TrackPtr curtrack = The::engineController()->currentTrack();

    if( !curtrack || !curtrack->artist() )
        return;

    bool refetch = true;
    if( m_hasLyrics )
    {
        const QString text( i18nc( "@info", "Do you really want to refetch lyrics for this track ? All changes you may have made will be lost.") );
        refetch = KMessageBox::warningContinueCancel( 0, text, i18n( "Refetch lyrics" ) ) == KMessageBox::Continue;
    }

    if( refetch )
        ScriptManager::instance()->notifyFetchLyrics( curtrack->artist()->name(), curtrack->name() );
}

void
LyricsApplet::changeLyricsFont()
{
    QFont font = ui_Settings.fontChooser->font();

    m_lyrics->setFont( font );

    KConfigGroup config = Amarok::config("Lyrics Applet");
    config.writeEntry( "Font", font.toString() );

    debug() << "Setting Lyrics Applet font: " << font.family() << " " << font.pointSize();
    // resize with new font
    // collapseToMin();
}

void
LyricsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );
    ui_Settings.fontChooser->setFont( m_lyrics->currentFont() );

    parent->addPage( settings, i18n( "Lyrics Settings" ), "preferences-system");

    connect( parent, SIGNAL( accepted() ), this, SLOT( changeLyricsFont() ) );
}

void
LyricsApplet::keyPressEvent( QKeyEvent *e )
{
    if( m_lyrics->isVisible() )
    {
        switch( e->key() )
        {
        case Qt::Key_Escape :
            closeLyrics();
            break;

        case Qt::Key_F2 :
            editLyrics();
            break;
        }

        if( e->matches( QKeySequence::Save ) )
            saveLyrics();

        e->accept();
    }
}

void
LyricsApplet::editLyrics()
{
    if( !m_hasLyrics )
    {
        m_lyrics->clear();
    }

    setEditing( true );
    determineActionIconsState();
    m_lyrics->ensureCursorVisible();
    setCollapseOff();
}

void
LyricsApplet::closeLyrics()
{
    if( m_hasLyrics )
    {
        QScrollBar *vbar = m_lyrics->verticalScrollBar();
        int savedPosition = vbar->isVisible() ? vbar->value() : vbar->minimum();

        if( m_isRichText )
            m_lyrics->setHtml( The::engineController()->currentTrack()->cachedLyrics() );
        else
            m_lyrics->setPlainText( The::engineController()->currentTrack()->cachedLyrics() );

        vbar->setSliderPosition( savedPosition );
        setCollapseOff();

        showLyrics();
        emit sizeHintChanged(Qt::MaximumSize);
    }
    else
    {
        m_lyrics->clear();
    }

    setEditing( false );
    determineActionIconsState();
}

void
LyricsApplet::saveLyrics()
{
    Meta::TrackPtr curtrack = The::engineController()->currentTrack();

    if( curtrack )
    {
        if( !m_lyrics->toPlainText().isEmpty() )
        {
            const QString lyrics = m_isRichText ? m_lyrics->toHtml() : m_lyrics->toPlainText();
            curtrack->setCachedLyrics( lyrics );
            setCollapseOff();
            m_hasLyrics = true;
        }
        else
        {
            curtrack->setCachedLyrics( QString() );
            m_hasLyrics = false;
        }
        emit sizeHintChanged(Qt::MaximumSize);
    }

    setEditing( false );
    determineActionIconsState();
}

void
LyricsApplet::setEditing( const bool isEditing )
{
    m_lyrics->setReadOnly( !isEditing );
    update();
    // collapseToMin();
}


void LyricsApplet::collapseToMin()
{
    QTextBrowser *browser = static_cast< QTextBrowser* >( m_proxy->widget() );
    if( !browser )
        return;

    // use a dummy item to get the lyrics layout being displayed
    QGraphicsTextItem testItem;
    testItem.setTextWidth( browser->document()->size().width() );
    testItem.setFont( browser->currentFont() );
    testItem.setHtml( browser->toHtml() );

    const qreal frameWidth     = browser->frameWidth();
    const qreal testItemHeight = testItem.boundingRect().height();
    const qreal headerHeight   = m_titleLabel->pos().y() + m_titleLabel->boundingRect().height() + standardPadding();
    const qreal contentHeight  = headerHeight + frameWidth + testItemHeight + frameWidth + standardPadding();

    // only show vertical scrollbar if there are lyrics and is needed
    browser->setVerticalScrollBarPolicy( m_hasLyrics ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff );

    // maybe we were just added, don't have a view yet
    if( !containment()->view() )
        return;
    
    const qreal containerOffset = mapToView( containment()->view(), boundingRect() ).topLeft().y();
    const qreal containerHeight = containment()->size().height() - containerOffset;
    const qreal collapsedHeight = ( contentHeight > containerHeight ) ? containerHeight : contentHeight;
    setCollapseHeight( collapsedHeight );
    setCollapseOn();
    updateConstraints();
}

void LyricsApplet::determineActionIconsState()
{
    const bool isEditing = !m_lyrics->isReadOnly();

    // If we're editing, hide and disable the edit icon
    m_editIcon->action()->setEnabled( !isEditing );
    m_editIcon->action()->setVisible( !isEditing );

    // If we're editing, show and enable the close icon
    m_closeIcon->action()->setEnabled( isEditing );
    m_closeIcon->action()->setVisible( isEditing );

    // If we're editing, show and enable the save icon
    m_saveIcon->action()->setEnabled( isEditing );
    m_saveIcon->action()->setVisible( isEditing );
}

void LyricsApplet::showLyrics()
{
    determineActionIconsState();
    m_proxy->setWidget( m_lyrics );
    m_lyrics->show();
}

void LyricsApplet::showSuggested()
{
    m_editIcon->action()->setEnabled( false );
    m_closeIcon->action()->setEnabled( false );
    m_saveIcon->action()->setEnabled( false );
    m_proxy->setWidget( m_suggested );
    m_suggested->show();
}

#include "LyricsApplet.moc"

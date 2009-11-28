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

#include <QAction>
#include <QGraphicsSimpleTextItem>
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
    m_lyricsProxy->setWidget( 0 );
    delete m_lyricsProxy;
    m_lyricsProxy = 0;
    delete m_lyrics;
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

    connect( m_editIcon, SIGNAL( activated() ), this, SLOT( editLyrics() ) );

    QAction* closeAction = new QAction( this );
    closeAction->setIcon( KIcon( "document-close" ) );
    closeAction->setVisible( false );
    closeAction->setEnabled( false );
    closeAction->setText( i18n( "Close" ) );
    m_closeIcon = addAction( closeAction );

    connect( m_closeIcon, SIGNAL( activated() ), this, SLOT( closeLyrics() ) );

    QAction* saveAction = new QAction( this );
    saveAction->setIcon( KIcon( "document-save" ) );
    saveAction->setVisible( false );
    saveAction->setEnabled( false );
    saveAction->setText( i18n( "Save Lyrics" ) );
    m_saveIcon = addAction( saveAction );

    connect( m_saveIcon, SIGNAL( activated() ), this, SLOT( saveLyrics() ) );

    QAction* reloadAction = new QAction( this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( true );
    reloadAction->setText( i18n( "Reload Lyrics" ) );
    m_reloadIcon = addAction( reloadAction );

    connect( m_reloadIcon, SIGNAL( activated() ), this, SLOT( refreshLyrics() ) );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setVisible( true );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    m_settingsIcon = addAction( settingsAction );

    connect( m_settingsIcon, SIGNAL( activated() ), this, SLOT( showConfigurationInterface() ) );


    m_lyricsProxy = new QGraphicsProxyWidget( this );
    m_lyricsProxy->setAttribute( Qt::WA_NoSystemBackground );
    m_lyrics = new QTextBrowser;
    m_lyrics->setAttribute( Qt::WA_NoSystemBackground );
    m_lyrics->setOpenExternalLinks( true );
    m_lyrics->setUndoRedoEnabled( true );
    m_lyrics->setAutoFillBackground( false );
    m_lyrics->setWordWrapMode( QTextOption::WordWrap );
    m_lyrics->viewport()->setAttribute( Qt::WA_NoSystemBackground );
    m_lyrics->setTextInteractionFlags( Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard );
    setEditing( false );

    m_lyricsProxy->setWidget( m_lyrics );

    // Read config
    KConfigGroup config = Amarok::config("Lyrics Applet");
    QFont font( config.readEntry( "Font", QString() ),
                config.readEntry( "Size", -1 ) );
    m_lyrics->setFont( font );

    // only show when we need to let the user
    // choose between suggestions
    m_suggested = new QGraphicsTextItem( this );
    connect( m_suggested, SIGNAL( linkActivated( const QString& ) ), this, SLOT( suggestionChosen( const QString& ) ) );
    m_suggested->setTextInteractionFlags( Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard );
    m_suggested->hide();
    
    connect( dataEngine( "amarok-lyrics" ), SIGNAL( sourceAdded( const QString& ) ), this, SLOT( connectSource( const QString& ) ) );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );

    constraintsEvent();
    updateConstraints();
    connectSource( "lyrics" );
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

    m_suggested->setTextWidth( size().width() - 2 * standardPadding() );
    m_suggested->setPos( standardPadding(), m_suggested->pos().y() );

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

    m_lyricsProxy->setPos( standardPadding(), m_titleLabel->pos().y() + m_titleLabel->boundingRect().height() + standardPadding() );
    
    QSize lyricsSize( size().width() - 2 * standardPadding(), boundingRect().height() - m_lyricsProxy->pos().y() - standardPadding() );
    
    m_lyricsProxy->setMinimumSize( lyricsSize );
    m_lyricsProxy->setMaximumSize( lyricsSize );

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
        m_suggested->hide();
        m_lyrics->show();
        m_lyrics->setPlainText( i18n( "No lyrics script is running." ) );
    }
    else if( data.contains( "stopped" ) )
    {
        m_lyrics->clear();
    }
    else if( data.contains( "fetching" ) )
    {
        setBusy( true );

        m_suggested->hide();
        m_lyrics->show();
        m_titleText = i18n( "Lyrics : Fetching ..." );
        m_lyrics->setPlainText( i18n( "Lyrics are being fetched." ) );
    }
    else if( data.contains( "error" ) )
    {
        m_suggested->hide();
        m_lyrics->show();
        m_lyrics->setPlainText( i18n( "Could not download lyrics.\nPlease check your Internet connection.\nError message:\n%1", data["error"].toString() ) );
    }
    else if( data.contains( "suggested" ) )
    {
        m_lyrics->hide();
        QVariantList suggested = data[ "suggested" ].toList();
        // build simple HTML to show
        // a list
        QString html = QString( "<br><br>" );
        foreach( const QVariant &suggestion, suggested )
        {
            const QString sug = suggestion.toString();
            const QStringList pieces = sug.split( " - " );
            const QString link = QString( "<a href=\"%1|%2|%3\">%4 - %5</a><br>" ).arg( pieces[ 0 ] ).arg( pieces[ 1 ] ).arg( pieces[ 2 ] ).arg( pieces[ 1 ] ).arg( pieces[ 0 ] );
            html += link;
        }
        m_suggested->setHtml( html );
        m_suggested->show();
        // copy it here as we use the suggested height rather than m_lyrics
        QGraphicsTextItem testItem;
        testItem.setHtml( m_suggested->toHtml() );
        testItem.setTextWidth( m_lyrics->size().width() );

        qreal contentHeight = testItem.boundingRect().height();
        contentHeight += 40;
        setCollapseHeight( contentHeight );
        setCollapseOn();
        setEditing( false );

        updateConstraints();
        update();

        return;
    }
    else if( data.contains( "html" ) )
    {
        m_hasLyrics = true;
        m_isRichText = true;
        // show pure html in the text area
        m_suggested->hide();
        m_lyrics->setHtml( data[ "html" ].toString() );
        m_lyrics->show();
        m_titleText = QString( "%1 : %2" ).arg( i18n( "Lyrics" ) ).arg( data[ "html" ].toString().section( "<title>", 1, 1 ).section( "</title>", 0, 0 ) );
        emit sizeHintChanged(Qt::MaximumSize);
    }
    else if( data.contains( "lyrics" ) )
    {
        m_hasLyrics = true;
        m_isRichText = false;
        m_suggested->hide();
        m_lyrics->show();
        QVariantList lyrics  = data[ "lyrics" ].toList();

        m_titleText = QString( " %1 : %2 - %3" ).arg( i18n( "Lyrics" ) ).arg( lyrics[ 0 ].toString() ).arg( lyrics[ 1 ].toString() );
        //  need padding for title
        m_lyrics->setPlainText( lyrics[ 3 ].toString().trimmed() );

        // the following line is needed to fix the bug of the lyrics applet sometimes not being correctly resized.
        // I don't have the courage to put this into Applet::setCollapseOff(), maybe that would break other applets.
        emit sizeHintChanged(Qt::MaximumSize);
    }
    else if( data.contains( "notfound" ) )
    {
        m_suggested->hide();
        m_lyrics->show();
        m_lyrics->setPlainText( i18n( "There were no lyrics found for this track" ) );
    }

    collapseToMin();

    setEditing( false );

    updateConstraints();
    update();
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

    p->save();

    const int frameWidth         = m_lyrics->frameWidth();
    const QScrollBar *hScrollBar = m_lyrics->horizontalScrollBar();
    const QScrollBar *vScrollBar = m_lyrics->verticalScrollBar();
    const qreal hScrollBarHeight = hScrollBar->isVisible() ? hScrollBar->height() + 2 : 0;
    const qreal vScrollBarWidth  = vScrollBar->isVisible() ? vScrollBar->width()  + 2 : 0;
    const QSizeF lyricsSize( m_lyricsProxy->size().width()  - vScrollBarWidth  - frameWidth * 2,
                             m_lyricsProxy->size().height() - hScrollBarHeight - frameWidth * 2 );
    const QPointF lyricsPos( m_lyricsProxy->pos().x() + frameWidth,
                             m_lyricsProxy->pos().y() + frameWidth );
    const QRectF lyricsRect( lyricsPos, lyricsSize );

    QPainterPath path;
    path.addRoundedRect( lyricsRect, 2, 2 );
    p->fillPath( path, background );
    p->restore();
}

void
LyricsApplet::paletteChanged( const QPalette & palette )
{
    Q_UNUSED( palette )
}

void
LyricsApplet::suggestionChosen( const QString& link )
{
    QStringList pieces = link.split( '|' );
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
    config.writeEntry( "Font", font.family() );
    config.writeEntry( "Size", font.pointSize() );

    debug() << "Setting Lyrics Applet font: " << font.family() << " " << font.pointSize();
    // resize with new font
    collapseToMin();
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

        case Qt::Key_Return :
        case Qt::Key_Enter :
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
        m_lyrics->show();

        vbar->setSliderPosition( savedPosition );
        setCollapseOff();
        emit sizeHintChanged(Qt::MaximumSize);
    }
    else
    {
        m_lyrics->clear();
    }

    setEditing( false );
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
}

void
LyricsApplet::setEditing( const bool isEditing )
{
    m_lyrics->setReadOnly( !isEditing );

    // If we're editing, hide and disable the edit icon
    m_editIcon->action()->setEnabled( !isEditing );
    m_editIcon->action()->setVisible( !isEditing );

    // If we're editing, show and enable the close icon
    m_closeIcon->action()->setEnabled( isEditing );
    m_closeIcon->action()->setVisible( isEditing );

    // If we're editing, show and enable the save icon
    m_saveIcon->action()->setEnabled( isEditing );
    m_saveIcon->action()->setVisible( isEditing );

    update();
    collapseToMin();
}


void LyricsApplet::collapseToMin()
{
    QGraphicsTextItem testItem;
    testItem.setHtml( m_lyrics->toHtml() );

    const QFontMetrics fm( m_lyrics->currentFont() );
    const qreal contentHeight = m_titleLabel->boundingRect().height()
                              + testItem.boundingRect().height()
                              + standardPadding()
                              + fm.height();

    // only show vertical scrollbar if there are lyrics and is needed
    m_lyrics->setVerticalScrollBarPolicy( m_hasLyrics ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff );

    setCollapseHeight( contentHeight );
    setCollapseOn();
}


#include "LyricsApplet.moc"

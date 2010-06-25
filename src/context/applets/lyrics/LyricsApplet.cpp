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

#define DEBUG_PREFIX "LyricsApplet"

#include "LyricsApplet.h"

#include "core/support/Amarok.h"
#include "App.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "context/widgets/TextScrollingWidget.h"
#include "dialogs/ScriptManager.h"
#include "core/meta/Meta.h"
#include "PaletteHandler.h"
#include "Theme.h"

#include <KConfigDialog>
#include <KGlobalSettings>
#include <KStandardDirs>
#include <KMessageBox>
#include <KTabWidget>
#include <KTextBrowser>

#include <Plasma/IconWidget>
#include <Plasma/Containment>
#include <Plasma/TextBrowser>
#include <Plasma/TreeView>

#include <QAction>
#include <QLinearGradient>
#include <QPainter>
#include <QPoint>
#include <QScrollBar>
#include <QStandardItemModel>

LyricsApplet::LyricsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_titleText( i18n( "Lyrics" ) )
    , m_titleLabel( 0 )
    , m_saveIcon( 0 )
    , m_editIcon( 0 )
    , m_reloadIcon( 0 )
    , m_closeIcon( 0 )
    , m_hasLyrics( false )
    , m_isRichText( true )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

LyricsApplet::~LyricsApplet()
{
}

void LyricsApplet::init()
{
    // Call the base implementation.
    Context::Applet::init();

    // properly set the size, asking for the whole cv size.
    resize( 500, -1 );

    m_titleLabel = new TextScrollingWidget( this );
    QFont bigger = m_titleLabel->font();
    bigger.setPointSize( bigger.pointSize() + 2 );
    m_titleLabel->setFont( bigger );
    m_titleLabel->setText( i18n( "Lyrics" ) );

    QAction* editAction = new QAction( this );
    editAction->setIcon( KIcon( "document-edit" ) );
    editAction->setVisible( true );
    editAction->setEnabled( false );
    editAction->setText( i18n( "Edit Lyrics" ) );
    m_editIcon = addAction( editAction );

    connect( m_editIcon, SIGNAL(clicked()), this, SLOT(editLyrics()) );

    QAction* closeAction = new QAction( this );
    closeAction->setIcon( KIcon( "document-close" ) );
    closeAction->setVisible( false );
    closeAction->setEnabled( false );
    closeAction->setText( i18n( "Close" ) );
    m_closeIcon = addAction( closeAction );

    connect( m_closeIcon, SIGNAL(clicked()), this, SLOT(closeLyrics()) );

    QAction* saveAction = new QAction( this );
    saveAction->setIcon( KIcon( "document-save" ) );
    saveAction->setVisible( false );
    saveAction->setEnabled( false );
    saveAction->setText( i18n( "Save Lyrics" ) );
    m_saveIcon = addAction( saveAction );

    connect( m_saveIcon, SIGNAL(clicked()), this, SLOT(saveLyrics()) );

    QAction* reloadAction = new QAction( this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( true );
    reloadAction->setText( i18n( "Reload Lyrics" ) );
    m_reloadIcon = addAction( reloadAction );

    connect( m_reloadIcon, SIGNAL(clicked()), this, SLOT(refreshLyrics()) );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setVisible( true );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    m_settingsIcon = addAction( settingsAction );

    connect( m_settingsIcon, SIGNAL(clicked()), this, SLOT(showConfigurationInterface()) );

    m_browser = new Plasma::TextBrowser( this );
    KTextBrowser *browserWidget = m_browser->nativeWidget();
    browserWidget->setFrameShape( QFrame::StyledPanel );
    browserWidget->setAttribute( Qt::WA_NoSystemBackground );
    browserWidget->setOpenExternalLinks( true );
    browserWidget->setUndoRedoEnabled( true );
    browserWidget->setAutoFillBackground( false );
    browserWidget->setWordWrapMode( QTextOption::WordWrap );
    browserWidget->viewport()->setAttribute( Qt::WA_NoSystemBackground );
    browserWidget->setTextInteractionFlags( Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard );

    m_suggestView = new Plasma::TreeView( this );
    m_suggestView->setModel( new QStandardItemModel( this ) );
    QTreeView *suggestTree = m_suggestView->nativeWidget();
    suggestTree->setFrameShape( QFrame::StyledPanel );
    suggestTree->setFrameShadow( QFrame::Sunken );
    suggestTree->setAttribute( Qt::WA_NoSystemBackground );
    suggestTree->setAlternatingRowColors( true );
    suggestTree->setAnimated( true );
    suggestTree->setRootIsDecorated( false );
    suggestTree->setTextElideMode( Qt::ElideRight );
    suggestTree->setSelectionBehavior( QAbstractItemView::SelectRows );
    suggestTree->setSelectionMode( QAbstractItemView::SingleSelection );
    suggestTree->setSortingEnabled( true );
    suggestTree->setUniformRowHeights( true );
    m_suggestView->hide();

    // Read config
    QFont font;
    if( font.fromString( Amarok::config("Lyrics Applet").readEntry("Font", QString()) ) )
        browserWidget->setFont( font );

    connect( suggestTree, SIGNAL(activated(QModelIndex)), this, SLOT(suggestionChosen(QModelIndex)) );
    connect( dataEngine("amarok-lyrics"), SIGNAL(sourceAdded(QString)), this, SLOT(connectSource(QString)) );
    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(paletteChanged(QPalette)) );

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
    const qreal iconWidth = m_settingsIcon->size().width();
    const qreal padding = standardPadding();

    // label position
    m_titleLabel->setScrollingText( m_titleText );
    m_titleLabel->setPos( ( size().width() - m_titleLabel->boundingRect().width() ) / 2 , padding + 3 );

    // settings icon position
    m_settingsIcon->setPos( size().width() - iconWidth - padding, padding );

    // reload icon position
    m_reloadIcon->setPos( m_settingsIcon->pos().x() - padding - iconWidth, padding );
    m_reloadIcon->show();

    // edit and close icon positions
    QPoint editIconPos( m_reloadIcon->pos().x() - padding - iconWidth, padding );
    m_editIcon->setPos( editIconPos );
    m_closeIcon->setPos( editIconPos );

    // save icon position
    m_saveIcon->setPos( m_editIcon->pos().x() - padding - iconWidth, padding );

    // proxy position and height depending on whether suggestions are showing
    qreal proxyY = m_titleLabel->pos().y() + m_titleLabel->boundingRect().height() + padding;
    qreal proxyH = boundingRect().height() - proxyY - padding;

    if( m_suggestView->isVisible() )
    {
        // suggest view position
        QPointF suggestViewPos( padding, proxyY );
        proxyH = proxyH / 3.0 - proxyY;
        qreal suggestViewRatio = m_browser->isVisible() ? 3.0 : 1.0;
        qreal suggestViewHeight( boundingRect().height() / suggestViewRatio - proxyY - padding );
        QSizeF suggestViewSize( size().width() - 2 * padding, suggestViewHeight );
        m_suggestView->setPos( suggestViewPos );
        m_suggestView->setMinimumSize( suggestViewSize );
        m_suggestView->setMaximumSize( suggestViewSize );
        proxyY = suggestViewPos.y() + suggestViewSize.height() + padding;
        proxyH = boundingRect().height() - proxyY - padding;
    }

    // browser position
    QPointF textBrowserPos( padding, proxyY );
    QSizeF textBrowserSize( size().width() - 2 * padding, proxyH );
    m_browser->setPos( textBrowserPos );
    m_browser->setMinimumSize( textBrowserSize );
    m_browser->setMaximumSize( textBrowserSize );

    update();
}

void LyricsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name )

    m_hasLyrics = false;
    m_suggestView->hide();

    if( data.size() == 0 ) return;

    //debug() << "got lyrics data: " << data;

    m_titleText = i18n( "Lyrics" );
    m_titleLabel->show();

    setBusy( false );

    if( data.contains( "noscriptrunning" ) )
    {
        m_browser->nativeWidget()->setPlainText( i18n( "No lyrics script is running." ) );
        showLyrics();
    }
    else if( data.contains( "stopped" ) )
    {
        m_browser->nativeWidget()->clear();
        showLyrics();
    }
    else if( data.contains( "fetching" ) )
    {

        if( canAnimate() )
            setBusy( true );

        m_titleText = i18n( "Lyrics : Fetching ..." );
        m_browser->nativeWidget()->setPlainText( i18n( "Lyrics are being fetched." ) );

        showLyrics();
    }
    else if( data.contains( "error" ) )
    {
        m_browser->nativeWidget()->setPlainText( i18n( "Could not download lyrics.\nPlease check your Internet connection.\nError message:\n%1", data["error"].toString() ) );
        showLyrics();
    }
    else if( data.contains( "suggested" ) )
    {
        QVariantList suggested = data[ "suggested" ].toList();
        m_titleText = QString( "Suggested Lyrics URLs" );
        showSuggested( suggested );
    }
    else if( data.contains( "html" ) )
    {
        m_hasLyrics = true;
        m_isRichText = true;
        // show pure html in the text area
        m_browser->nativeWidget()->setHtml( data[ "html" ].toString() );
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
        m_browser->nativeWidget()->setPlainText( lyrics[ 3 ].toString().trimmed() );
        showLyrics();

        // the following line is needed to fix the bug of the lyrics applet sometimes not being correctly resized.
        // I don't have the courage to put this into Applet::setCollapseOff(), maybe that would break other applets.
        emit sizeHintChanged(Qt::MaximumSize);
    }
    else if( data.contains( "notfound" ) )
    {
        m_browser->nativeWidget()->setPlainText( i18n( "There were no lyrics found for this track" ) );
        showLyrics();
    }

    update();
    // collapseToMin();
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
    if( m_browser->nativeWidget()->isReadOnly() )
        background = The::paletteHandler()->backgroundColor();
    else // different background color when we're in edit mode
        background = The::paletteHandler()->alternateBackgroundColor();

    p->save();
    const QScrollBar *hScrollBar = m_browser->nativeWidget()->horizontalScrollBar();
    const QScrollBar *vScrollBar = m_browser->nativeWidget()->verticalScrollBar();
    qreal hScrollBarHeight = hScrollBar->isVisible() ? hScrollBar->height() : 0;
    qreal vScrollBarWidth  = vScrollBar->isVisible() ? vScrollBar->width()  : 0;
    int frameWidth = m_browser->nativeWidget()->frameWidth();
    QPointF proxyPos( m_browser->pos().x() + frameWidth, m_browser->pos().y() + frameWidth );
    QSizeF proxySize( m_browser->size().width()  - vScrollBarWidth  - frameWidth * 2,
                      m_browser->size().height() - hScrollBarHeight - frameWidth * 2 );
    QRectF proxyRect( proxyPos, proxySize );

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
LyricsApplet::refreshLyrics()
{
    Meta::TrackPtr curtrack = The::engineController()->currentTrack();

    if( !curtrack || !curtrack->artist() )
        return;

    bool refetch = true;
    if( m_hasLyrics )
    {
        const QString text( i18nc( "@info", "Do you really want to refetch lyrics for this track? All changes you may have made will be lost.") );
        refetch = KMessageBox::warningContinueCancel( 0, text, i18n( "Refetch lyrics" ) ) == KMessageBox::Continue;
    }

    if( refetch )
        ScriptManager::instance()->notifyFetchLyrics( curtrack->artist()->name(), curtrack->name() );
}

void
LyricsApplet::changeLyricsFont()
{
    QFont font = ui_Settings.fontChooser->font();

    m_browser->nativeWidget()->setFont( font );

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
    ui_Settings.fontChooser->setFont( m_browser->nativeWidget()->currentFont() );

    parent->enableButtonApply( true );
    parent->addPage( settings, i18n( "Lyrics Settings" ), "preferences-system" );

    connect( parent, SIGNAL(accepted()), this, SLOT(changeLyricsFont()) );
    connect( parent, SIGNAL(applyClicked()), this, SLOT(changeLyricsFont()) );
}

void
LyricsApplet::keyPressEvent( QKeyEvent *e )
{
    if( m_browser->nativeWidget()->isVisible() )
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
    else
    {
        Context::Applet::keyPressEvent( e );
    }
}

void
LyricsApplet::editLyrics()
{
    if( !m_hasLyrics )
    {
        m_browser->nativeWidget()->clear();
    }

    setEditing( true );
    determineActionIconsState();
    m_browser->nativeWidget()->ensureCursorVisible();
    setCollapseOff();
}

void
LyricsApplet::closeLyrics()
{
    if( m_hasLyrics )
    {
        QScrollBar *vbar = m_browser->nativeWidget()->verticalScrollBar();
        int savedPosition = vbar->isVisible() ? vbar->value() : vbar->minimum();

        if( m_isRichText )
            m_browser->nativeWidget()->setHtml( The::engineController()->currentTrack()->cachedLyrics() );
        else
            m_browser->nativeWidget()->setPlainText( The::engineController()->currentTrack()->cachedLyrics() );

        vbar->setSliderPosition( savedPosition );
        setCollapseOff();

        showLyrics();
        emit sizeHintChanged(Qt::MaximumSize);
    }
    else
    {
        m_browser->nativeWidget()->clear();
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
        if( !m_browser->nativeWidget()->toPlainText().isEmpty() )
        {
            const QString lyrics = m_isRichText ? m_browser->nativeWidget()->toHtml() : m_browser->nativeWidget()->toPlainText();
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
    m_browser->nativeWidget()->setReadOnly( !isEditing );
    update();
    // collapseToMin();
}


// FIXME: This does not work in all cases yet.
// There are two known cases where this is broken:
// -The lyrics applet is visible to the user, showing only a
//  small amount of text. Then the user hides (hiding != removing)
//  the applet. Once the track changes and the new track has long
//  lyrics the scrollbar is clipped.
// -The lyrics applet's position is moved. Say the lyrics applet would
//  take all the space. Now there's the current track applet above the
//  lyrics applet -> once the user moves the lyrics applet before the
//  current track applet the lyrics applet won't resize.
void LyricsApplet::collapseToMin()
{
    QTextBrowser *browser = qobject_cast< KTextBrowser* >( m_browser->nativeWidget() );
    if( !browser )
        return;

    qreal testItemHeight = 0;

    // Check if the text browser contains any content (which is
    // visible to the user).
    if ( !browser->toPlainText().trimmed().isEmpty() )
    {
        // use a dummy item to get the lyrics layout being displayed
        QGraphicsTextItem testItem;
        testItem.setTextWidth( browser->document()->size().width() );
        testItem.setFont( browser->currentFont() );
        testItem.setHtml( browser->toHtml() );

        // Get the height of the test item.
        testItemHeight = testItem.boundingRect().height();
    }

    const qreal frameWidth     = browser->frameWidth();
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
    const bool isEditing = !m_browser->nativeWidget()->isReadOnly();

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
    m_suggestView->hide();
}

void LyricsApplet::showSuggested( const QVariantList &suggestions )
{
    m_editIcon->action()->setEnabled( false );
    m_closeIcon->action()->setEnabled( false );
    m_saveIcon->action()->setEnabled( false );

    QStandardItemModel *model = qobject_cast<QStandardItemModel*>( m_suggestView->model() );
    model->clear();
    model->setHorizontalHeaderLabels( QStringList() << i18n("Title") << i18n("Artist") << i18n("URL") );
    QStandardItem *parentItem = model->invisibleRootItem();
    foreach( const QVariant &suggestion, suggestions )
    {
        const QStringList &s  = suggestion.toStringList();
        QStandardItem *title  = new QStandardItem( s.at(0) );
        QStandardItem *artist = new QStandardItem( s.at(1) );
        QStandardItem *url    = new QStandardItem( s.at(2) );
        parentItem->appendRow( QList<QStandardItem*>() << title << artist << url );
    }
    m_suggestView->show();
}

void
LyricsApplet::suggestionChosen( const QModelIndex &index )
{
    if( !index.isValid() )
        return;

    int row = index.row();
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>( m_suggestView->model() );
    const QString &title  = model->item( row, 0 )->text();
    const QString &artist = model->item( row, 1 )->text();
    const QString &url    = model->item( row, 2 )->text();
    if( !url.isEmpty() )
        ScriptManager::instance()->notifyFetchLyricsByUrl( artist, title, url );
    m_suggestView->hide();
}

#include "LyricsApplet.moc"

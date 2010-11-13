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

#include "EngineController.h"
#include "PaletteHandler.h"
#include "context/widgets/TextScrollingWidget.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "dialogs/ScriptManager.h"
#include "context/LyricsManager.h"

#include <KConfigDialog>
#include <KGlobalSettings>
#include <KMessageBox>
#include <KTextBrowser>

#include <Plasma/Containment>
#include <Plasma/IconWidget>
#include <Plasma/Label>
#include <Plasma/TextBrowser>
#include <Plasma/TreeView>

#include <QAction>
#include <QLabel>
#include <QGraphicsLinearLayout>
#include <QPainter>
#include <QPoint>
#include <QScrollBar>
#include <QStandardItemModel>

class LyricsAppletPrivate
{
public:
    LyricsAppletPrivate( LyricsApplet *parent )
        : titleText( i18n( "Lyrics" ) )
        , titleLabel( 0 )
        , saveIcon( 0 )
        , editIcon( 0 )
        , reloadIcon( 0 )
        , closeIcon( 0 )
        , settingsIcon( 0 )
        , browser( 0 )
        , suggestView( 0 )
        , currentTrack( 0 )
        , hasLyrics( false )
        , isRichText( true )
        , showBrowser( false )
        , showInfoLabel( false )
        , showSuggestions( false )
        , q_ptr( parent ) {}
    ~LyricsAppletPrivate() {}

    // member functions
    void setEditing( const bool isEditing );
    void collapseToMin();
    void determineActionIconsState();
    void clearLyrics();
    void refetchLyrics();
    void showLyrics( const QString &text, bool isRichText );
    void showSuggested( const QVariantList &suggestions );
    void showUnsavedChangesWarning( Meta::TrackPtr );
    const QString currentText();

    // private slots
    void _editLyrics();
    void _changeLyricsFont();
    void _closeLyrics();
    void _saveLyrics();
    void _suggestionChosen( const QModelIndex &index );
    void _unsetCursor();
    void _trackDataChanged( Meta::TrackPtr );
    void _lyricsChangedMessageButtonPressed( const MessageButton );
    void _refetchMessageButtonPressed( const MessageButton );

    // data / widgets
    QString titleText;
    TextScrollingWidget *titleLabel;

    QGraphicsLinearLayout *headerLayout;
    Plasma::IconWidget *saveIcon;
    Plasma::IconWidget *editIcon;
    Plasma::IconWidget *reloadIcon;
    Plasma::IconWidget *closeIcon;
    Plasma::IconWidget *settingsIcon;

    Plasma::TextBrowser *browser;
    Plasma::TreeView    *suggestView;

    Plasma::Label *infoLabel;

    Ui::lyricsSettings ui_settings;

    Meta::TrackPtr currentTrack;
    Meta::TrackPtr modifiedTrack;
    QString modifiedLyrics;

    bool hasLyrics;
    bool isRichText;
    bool showBrowser;
    bool showInfoLabel;
    bool showSuggestions;

private:
    LyricsApplet *const q_ptr;
    Q_DECLARE_PUBLIC( LyricsApplet )
};

void
LyricsAppletPrivate::setEditing( const bool isEditing )
{
    browser->nativeWidget()->setReadOnly( !isEditing );
    KTextBrowser *textBrowser = browser->nativeWidget();
    QPalette::ColorRole bg = textBrowser->isReadOnly() ? QPalette::Base : QPalette::AlternateBase;
    textBrowser->viewport()->setBackgroundRole( bg );
}

void
LyricsAppletPrivate::collapseToMin()
{
    DEBUG_BLOCK
    Q_Q( LyricsApplet );

    // Simply drop out if one of the conditions is met:
    // -The applet is currently not visible.
    // -One of the widgets is 0.
    if ( !q->isVisible() || !browser || !suggestView || !infoLabel )
        return;

    KTextBrowser *textBrowser = browser->nativeWidget();
    QTreeView *treeView = suggestView->nativeWidget();
    QLabel *label = infoLabel->nativeWidget();

    qreal contentItemHeight = 0;

    if( showBrowser && textBrowser )
    {
        // use a dummy item to get the lyrics layout being displayed
        QGraphicsTextItem testItem;
        testItem.setTextWidth( textBrowser->document()->size().width() );
        testItem.setFont( textBrowser->currentFont() );
        testItem.setHtml( textBrowser->toHtml() );

        // Add the height of the test item to the calculated height.
        contentItemHeight += testItem.boundingRect().height();
    }

    if( showSuggestions && treeView )
    {
        // Add the height of the suggestion view to the
        // calculated test item height.
        contentItemHeight += treeView->sizeHint().height();
    }

    if( showInfoLabel && label )
    {
        QGraphicsTextItem testItem;
        testItem.setPlainText( label->text() );
        testItem.setFont( label->font() );

        // Add the height of the test item to the calculated height.
        // We multiply the height with 2 as there's some spacing between
        // the text and the header.
        contentItemHeight += testItem.boundingRect().height() * 2;
    }

    const qreal padding        = q->standardPadding();
    const qreal frameWidth     = textBrowser->frameWidth();
    const qreal headerHeight   = titleLabel->pos().y() + titleLabel->boundingRect().height() + padding;
    const qreal contentHeight  = headerHeight + frameWidth + contentItemHeight + frameWidth + padding;

    // only show vertical scrollbar if there are lyrics and is needed
    textBrowser->setVerticalScrollBarPolicy( hasLyrics ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff );

    // maybe we were just added, don't have a view yet
    if( !q || !q->containment() || !q->containment()->view() )
        return;

    const qreal containerOffset = q->mapToView( q->containment()->view(), q->boundingRect() ).topLeft().y();
    const qreal containerHeight = q->containment()->size().height() - containerOffset;
    const qreal collapsedHeight = ( contentHeight > containerHeight ) ? containerHeight : contentHeight;

    q->setCollapseHeight( collapsedHeight );
    q->setCollapseOn();
    q->updateConstraints();
}

void
LyricsAppletPrivate::determineActionIconsState()
{
    bool isEditing = !browser->nativeWidget()->isReadOnly();

    editIcon->action()->setEnabled( showBrowser & !isEditing );
    editIcon->action()->setVisible( showBrowser & !isEditing );

    closeIcon->action()->setEnabled( showBrowser & isEditing );
    closeIcon->action()->setVisible( showBrowser & isEditing );

    saveIcon->action()->setEnabled( showBrowser & isEditing );
    saveIcon->action()->setVisible( showBrowser & isEditing );

    reloadIcon->action()->setEnabled( showBrowser & !isEditing );
    reloadIcon->action()->setVisible( showBrowser & !isEditing );

    // remove all header widgets and add back below
    int count = headerLayout->count();
    while( --count >= 0 )
    {
        QGraphicsLayoutItem *child = headerLayout->itemAt( 0 );
        headerLayout->removeItem( child );
    }

    QList<QGraphicsWidget*> widgets; // preserve widget order
    widgets << titleLabel;
    if( saveIcon->action()->isVisible() )   widgets << saveIcon;
    if( editIcon->action()->isVisible() )   widgets << editIcon;
    if( reloadIcon->action()->isVisible() ) widgets << reloadIcon;
    if( closeIcon->action()->isVisible() )  widgets << closeIcon;
    widgets << settingsIcon;
    QList<QGraphicsWidget*>::const_iterator it, itEnd;
    for( it = widgets.constBegin(), itEnd = widgets.constEnd(); it != itEnd; ++it  )
        headerLayout->addItem( *it );
}

void
LyricsAppletPrivate::clearLyrics()
{
    browser->nativeWidget()->clear();
}

void
LyricsAppletPrivate::showLyrics( const QString &text, bool isRichText )
{
    clearLyrics();
    if( isRichText )
        browser->nativeWidget()->setHtml( text );
    else
        browser->nativeWidget()->setPlainText( text );
    showInfoLabel = false;
    showSuggestions = false;
    showBrowser = true;
    determineActionIconsState();
    collapseToMin();
}

void
LyricsAppletPrivate::showSuggested( const QVariantList &suggestions )
{
    editIcon->action()->setEnabled( false );
    closeIcon->action()->setEnabled( false );
    saveIcon->action()->setEnabled( false );

    QStandardItemModel *model = qobject_cast<QStandardItemModel*>( suggestView->model() );
    model->clear();
    model->setHorizontalHeaderLabels( QStringList() << i18n("Title") << i18n("Artist") );
    QStandardItem *parentItem = model->invisibleRootItem();
    foreach( const QVariant &suggestion, suggestions )
    {
        const QStringList &s  = suggestion.toStringList();
        QStandardItem *title  = new QStandardItem( s.at(0) );
        QStandardItem *artist = new QStandardItem( s.at(1) );
        title->setToolTip( title->text() );
        title->setEditable( false );
        artist->setToolTip( artist->text() );
        artist->setEditable( false );
        const QString &url = s.at( 2 );
        title->setData( url ); // url is set in the user role +1 of title
        parentItem->appendRow( QList<QStandardItem*>() << title << artist );
    }

    Q_Q( LyricsApplet );
    qreal width = q->size().width() - 2 * q->standardPadding();
    QHeaderView *header = suggestView->nativeWidget()->header();
    header->resizeSection( 0, width * 2 / 3 );
    header->setStretchLastSection( true );
    showSuggestions = true;
    collapseToMin();
}

const QString
LyricsAppletPrivate::currentText()
{
    return isRichText ? browser->nativeWidget()->toHtml() : browser->nativeWidget()->toPlainText();
}

void
LyricsAppletPrivate::refetchLyrics()
{
    ScriptManager::instance()->notifyFetchLyrics( currentTrack->artist()->name(),
                                                  currentTrack->name() );
}

void
LyricsAppletPrivate::showUnsavedChangesWarning( Meta::TrackPtr newTrack )
{
    Q_Q( LyricsApplet );

    // Set the track which was modified and store the current
    // lyircs from the UI.
    modifiedTrack = currentTrack;
    modifiedLyrics = currentText();

    QString artistName = modifiedTrack->artist() ? modifiedTrack->artist()->name() : i18nc( "Used if the current track has no artist.", "Unknown" );
    QString warningMessage;

    // Check if the track has changed.
    if( newTrack != modifiedTrack )
    {
        // Show a warning that the track has changed while the user was editing the lyrics for the current track.
        warningMessage = i18n( "While you were editing the lyrics of <b>%1 - %2</b> the track has changed. Do you want to save your changes?",
                                artistName,
                                modifiedTrack->prettyName() );
    }
    else
    {
        // Show a warning that the lyrics for the track were modified (for example by a script).
        warningMessage = i18n( "The lyrics of <b>%1 - %2</b> changed while you were editing them. Do you want to save your changes?",
                               artistName,
                               modifiedTrack->prettyName() );
    }

    // Show the warning message.
    q->showWarning( warningMessage, SLOT( _lyricsChangedMessageButtonPressed( MessageButton ) ) );

    // Make the contents readonly again.
    // Since the applet is now blocked the user can not enable this again.
    // Thus we can make sure that we won't overwrite modifiedTrack.
    setEditing( false );
}

void
LyricsAppletPrivate::_refetchMessageButtonPressed( const MessageButton button )
{
    // Check if the user pressed "Yes".
    if( button == Plasma::ButtonYes )
        // Refetch the lyrics.
        refetchLyrics();
}

void
LyricsAppletPrivate::_lyricsChangedMessageButtonPressed( const MessageButton button )
{
    // Check if the user pressed "Yes".
    if( button == Plasma::ButtonYes )
        // Update the lyrics of the track.
        modifiedTrack->setCachedLyrics( modifiedLyrics );
}

void
LyricsAppletPrivate::_changeLyricsFont()
{
    QFont font = ui_settings.fontChooser->font();
    browser->nativeWidget()->setFont( font );
    KConfigGroup config = Amarok::config("Lyrics Applet");
    config.writeEntry( "Font", font.toString() );
    debug() << "Setting Lyrics Applet font: " << font.family() << " " << font.pointSize();
    // resize with new font
    collapseToMin();
}

void
LyricsAppletPrivate::_editLyrics()
{
    if( !hasLyrics )
        clearLyrics();

    setEditing( true );
    determineActionIconsState();
    browser->nativeWidget()->ensureCursorVisible();
}

void
LyricsAppletPrivate::_closeLyrics()
{
    if( hasLyrics )
    {
        QScrollBar *vbar = browser->nativeWidget()->verticalScrollBar();
        int savedPosition = vbar->isVisible() ? vbar->value() : vbar->minimum();

        if( isRichText )
            browser->nativeWidget()->setHtml( currentTrack->cachedLyrics() );
        else
            browser->nativeWidget()->setPlainText( currentTrack->cachedLyrics() );

        vbar->setSliderPosition( savedPosition );

        determineActionIconsState();
        showSuggestions = false;
        showBrowser = true;
        // emit sizeHintChanged(Qt::MaximumSize);
    }
    else
    {
        clearLyrics();
    }

    setEditing( false );
    determineActionIconsState();
}

void
LyricsAppletPrivate::_saveLyrics()
{
    if( currentTrack )
    {
        if( !LyricsManager::self()->isEmpty( browser->nativeWidget()->toPlainText() ) )
        {
            currentTrack->setCachedLyrics( currentText() );
            hasLyrics = true;
        }
        else
        {
            currentTrack->setCachedLyrics( QString() );
            hasLyrics = false;
        }
        // emit sizeHintChanged(Qt::MaximumSize);
    }

    setEditing( false );
    determineActionIconsState();
}

void
LyricsAppletPrivate::_suggestionChosen( const QModelIndex &index )
{
    DEBUG_BLOCK
    Q_Q( LyricsApplet );
    if( !index.isValid() )
        return;

    int row = index.row();
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>( suggestView->model() );
    const QString &title  = model->item( row, 0 )->text();
    const QString &artist = model->item( row, 1 )->text();
    const QString &url    = model->item( row, 0 )->data().toString();
    if( !url.isEmpty() )
    {
        debug() << "clicked suggestion" << url;
        ScriptManager::instance()->notifyFetchLyricsByUrl( artist, title, url );
        suggestView->setCursor( Qt::BusyCursor );
        QTimer::singleShot( 10000, q, SLOT(_unsetCursor()) );
    }
}

void
LyricsAppletPrivate::_unsetCursor()
{
    if( suggestView->hasCursor() )
        suggestView->unsetCursor();
}

void
LyricsAppletPrivate::_trackDataChanged( Meta::TrackPtr track )
{
    DEBUG_BLOCK

    // Check if we previously had a track.
    // If the lyrics currently shown in the browser (which
    // additionally is in edit mode) are different from the
    // lyrics of the track we have to show a warning.
    if( currentTrack &&
        currentTrack->cachedLyrics() != currentText() &&
        !browser->nativeWidget()->isReadOnly() )
    {
        showUnsavedChangesWarning( track );
    }

    // Update the current track.
    currentTrack = track;
}

LyricsApplet::LyricsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , d_ptr( new LyricsAppletPrivate( this ) )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

LyricsApplet::~LyricsApplet()
{
    delete d_ptr;
}

void
LyricsApplet::init()
{
    Q_D( LyricsApplet );

    // Call the base implementation.
    Context::Applet::init();

    // properly set the size, asking for the whole cv size.
    resize( 500, -1 );

    d->titleLabel = new TextScrollingWidget( this );
    QFont bigger = d->titleLabel->font();
    bigger.setPointSize( bigger.pointSize() + 2 );
    d->titleLabel->setFont( bigger );
    d->titleLabel->setText( i18n( "Lyrics" ) );
    d->titleLabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
    d->titleLabel->setDrawBackground( true );

    QAction* editAction = new QAction( this );
    editAction->setIcon( KIcon( "document-edit" ) );
    editAction->setVisible( true );
    editAction->setEnabled( false );
    editAction->setText( i18n( "Edit Lyrics" ) );
    d->editIcon = addAction( editAction );

    connect( d->editIcon, SIGNAL(clicked()), this, SLOT(_editLyrics()) );

    QAction* closeAction = new QAction( this );
    closeAction->setIcon( KIcon( "document-close" ) );
    closeAction->setVisible( false );
    closeAction->setEnabled( false );
    closeAction->setText( i18n( "Close" ) );
    d->closeIcon = addAction( closeAction );

    connect( d->closeIcon, SIGNAL(clicked()), this, SLOT(_closeLyrics()) );

    QAction* saveAction = new QAction( this );
    saveAction->setIcon( KIcon( "document-save" ) );
    saveAction->setVisible( false );
    saveAction->setEnabled( false );
    saveAction->setText( i18n( "Save Lyrics" ) );
    d->saveIcon = addAction( saveAction );

    connect( d->saveIcon, SIGNAL(clicked()), this, SLOT(_saveLyrics()) );

    QAction* reloadAction = new QAction( this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( true );
    reloadAction->setText( i18n( "Reload Lyrics" ) );
    d->reloadIcon = addAction( reloadAction );

    connect( d->reloadIcon, SIGNAL(clicked()), this, SLOT(refreshLyrics()) );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setVisible( true );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    d->settingsIcon = addAction( settingsAction );

    connect( d->settingsIcon, SIGNAL(clicked()), this, SLOT(showConfigurationInterface()) );

    d->headerLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    d->headerLayout->addItem( d->titleLabel );
    d->headerLayout->addItem( d->saveIcon );
    d->headerLayout->addItem( d->editIcon );
    d->headerLayout->addItem( d->reloadIcon );
    d->headerLayout->addItem( d->closeIcon );
    d->headerLayout->addItem( d->settingsIcon );
    d->headerLayout->setContentsMargins( 0, 4, 0, 2 );
    d->headerLayout->setMinimumWidth( 0 );

    d->browser = new Plasma::TextBrowser( this );
    KTextBrowser *browserWidget = d->browser->nativeWidget();
    browserWidget->setFrameShape( QFrame::StyledPanel );
    browserWidget->setAttribute( Qt::WA_NoSystemBackground );
    browserWidget->setOpenExternalLinks( true );
    browserWidget->setUndoRedoEnabled( true );
    browserWidget->setAutoFillBackground( false );
    browserWidget->setWordWrapMode( QTextOption::WordWrap );
    browserWidget->viewport()->setAutoFillBackground( true );
    browserWidget->setTextInteractionFlags( Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard );
    d->browser->hide();

    d->suggestView = new Plasma::TreeView( this );
    d->suggestView->setModel( new QStandardItemModel( this ) );
    QTreeView *suggestTree = d->suggestView->nativeWidget();
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
    d->suggestView->hide();

    d->infoLabel = new Plasma::Label( this );
    d->infoLabel->setAlignment( Qt::AlignCenter );
    d->infoLabel->nativeWidget()->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    d->infoLabel->hide();

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical );
    layout->addItem( d->headerLayout );
    layout->addItem( d->browser );
    setLayout( layout );

    // Read config
    QFont font;
    if( font.fromString( Amarok::config("Lyrics Applet").readEntry("Font", QString()) ) )
        browserWidget->setFont( font );

    EngineController* engine = The::engineController();

    connect( engine, SIGNAL( trackChanged( Meta::TrackPtr ) ), this, SLOT( _trackDataChanged( Meta::TrackPtr ) ) );
    connect( engine, SIGNAL( trackMetadataChanged( Meta::TrackPtr ) ), this, SLOT( _trackDataChanged( Meta::TrackPtr ) ) );
    connect( suggestTree, SIGNAL(activated(QModelIndex)), this, SLOT(_suggestionChosen(QModelIndex)) );
    connect( dataEngine("amarok-lyrics"), SIGNAL(sourceAdded(QString)), this, SLOT(connectSource(QString)) );
    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(paletteChanged(QPalette)) );

    // Update the palette.
    paletteChanged( The::paletteHandler()->palette() );

    d->setEditing( false );
    d->determineActionIconsState();
    connectSource( "lyrics" );
}

void
LyricsApplet::connectSource( const QString& source )
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

void
LyricsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_D( LyricsApplet );
    Context::Applet::constraintsEvent( constraints );
    d->titleLabel->setScrollingText( d->titleText );
    update();
}

void
LyricsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name )
    Q_D( LyricsApplet );

    unsetCursor();
    d->hasLyrics = false;
    d->showSuggestions = false;
    d->showInfoLabel = false;
    setBusy( false );

    if( data.contains( "noscriptrunning" ) )
    {
        d->titleText = i18n( "Lyrics: No script is running" );
        d->infoLabel->setText( i18n( "Lyrics can not be fetched as no script is running" ) );
        d->showInfoLabel = true;
        d->showBrowser = false;
    }
    else if( data.contains( "stopped" ) )
    {
        d->titleText = i18n( "Lyrics" );
        d->showBrowser = false;
    }
    else if( data.contains( "fetching" ) )
    {
        if( canAnimate() )
            setBusy( true );
        d->titleText = i18n( "Lyrics: Fetching ..." );
        d->infoLabel->setText( i18n( "Lyrics are being fetched" ) );
        d->showInfoLabel = true;
        d->showBrowser = false;
    }
    else if( data.contains( "error" ) )
    {
        d->titleText = i18n( "Lyrics: Fetch error" );
        d->infoLabel->setText( i18n( "Could not download lyrics.\n"
                                     "Please check your Internet connection.\n"
                                     "Error message:\n"
                                     "%1", data["error"].toString() ) );
        d->showInfoLabel = true;
        d->showBrowser = false;
    }
    else if( data.contains( "suggested" ) )
    {
        QVariantList suggested = data[ "suggested" ].toList();
        d->titleText = i18n( "Lyrics: Suggested URLs" );
        d->showSuggested( suggested );
    }
    else if( data.contains( "html" ) )
    {
        d->hasLyrics = true;
        d->isRichText = true;
        // show pure html in the text area
        d->titleText = QString( "%1: %2" )
            .arg( i18n( "Lyrics" ) )
            .arg( data[ "html" ].toString().section( "<title>", 1, 1 ).section( "</title>", 0, 0 ) );
        d->showLyrics( data["html"].toString(), true );
        emit sizeHintChanged(Qt::MaximumSize);
    }
    else if( data.contains( "lyrics" ) )
    {
        d->hasLyrics = true;
        d->isRichText = false;
        QVariantList lyrics  = data[ "lyrics" ].toList();

        d->titleText = QString( "%1: %2 - %3" )
            .arg( i18n( "Lyrics" ) )
            .arg( lyrics[0].toString() ).arg( lyrics[1].toString() );
        d->showLyrics( lyrics[3].toString().trimmed(), false );

        // the following line is needed to fix the bug of the lyrics applet sometimes not being correctly resized.
        // I don't have the courage to put this into Applet::setCollapseOff(), maybe that would break other applets.
        emit sizeHintChanged(Qt::MaximumSize);
    }
    else if( data.contains( "notfound" ) )
    {
        d->titleText = i18n( "Lyrics: Not found" );
        d->infoLabel->setText( i18n( "There were no lyrics found for this track" ) );
        d->showInfoLabel = true;
        d->showBrowser = false;
    }

    QGraphicsLinearLayout *lo = static_cast<QGraphicsLinearLayout*>( layout() );
    d->showSuggestions ? lo->insertItem( 1, d->suggestView ) : lo->removeItem( d->suggestView );
    d->showInfoLabel ? lo->insertItem( 1, d->infoLabel ) : lo->removeItem( d->infoLabel );
    d->showBrowser ? lo->addItem( d->browser ) : lo->removeItem( d->browser );

    d->showSuggestions ? d->suggestView->show() : d->suggestView->hide();
    d->showInfoLabel ? d->infoLabel->show() : d->infoLabel->hide();
    d->showBrowser ? d->browser->show() : d->browser->hide();

    d->determineActionIconsState();
    d->collapseToMin();
    constraintsEvent();
}

bool
LyricsApplet::hasHeightForWidth() const
{
    return false;
}

void
LyricsApplet::paletteChanged( const QPalette &palette )
{
    Q_D( LyricsApplet );
    KTextBrowser *textBrowser = d->browser->nativeWidget();
    QPalette::ColorRole bg = textBrowser->isReadOnly() ? QPalette::Base : QPalette::AlternateBase;
    textBrowser->viewport()->setBackgroundRole( bg );

    // Use the text-color from KDE's colorscheme as we're already using it's background color.
    // Not using it might cause "conflicts" (where the text could become unreadable).
    textBrowser->setTextColor( palette.text().color() );
}

void
LyricsApplet::refreshLyrics()
{
    Q_D( LyricsApplet );
    if( !d->currentTrack || !d->currentTrack->artist() )
        return;

    if( d->hasLyrics )
    {
        // Ask the user if he really wants to refetch the lyrics.
        const QString text( i18nc( "@info", "Do you really want to refetch lyrics for this track? All changes you may have made will be lost.") );
        showWarning( text, SLOT( _refetchMessageButtonPressed( MessageButton ) ) );
    }
    else
    {
        // As we don't have lyrics yet we will
        // refetch without asking the user.
        d->refetchLyrics();
    }
}

void
LyricsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    Q_D( LyricsApplet );
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    d->ui_settings.setupUi( settings );
    d->ui_settings.fontChooser->setFont( d->browser->nativeWidget()->currentFont() );

    parent->enableButtonApply( true );
    parent->addPage( settings, i18n( "Lyrics Settings" ), "preferences-system" );

    connect( parent, SIGNAL(accepted()), this, SLOT(_changeLyricsFont()) );
    connect( parent, SIGNAL(applyClicked()), this, SLOT(_changeLyricsFont()) );
}

void
LyricsApplet::keyPressEvent( QKeyEvent *e )
{
    Q_D( LyricsApplet );
    if( d->browser->nativeWidget()->isVisible() )
    {
        bool propagate( true );
        switch( e->key() )
        {
        case Qt::Key_Escape :
            d->_closeLyrics();
            propagate = false;
            break;

        case Qt::Key_F2 :
            d->_editLyrics();
            propagate = false;
            break;
        }

        if( e->matches( QKeySequence::Save ) )
        {
            d->_saveLyrics();
            propagate = false;
        }

        if( !propagate )
        {
            e->accept();
            return;
        }
    }
    Context::Applet::keyPressEvent( e );
}

QVariant
LyricsApplet::itemChange( GraphicsItemChange change, const QVariant &value )
{
    Q_D( LyricsApplet );

    // There are at least two cases where we have to
    // collapse again:
    // -The track changed while the applet was not visible
    // -The user moved the applet around.
    switch( change )
    {
        // Fall-through for all cases here, as in all
        // cases a collapseToMin() should be triggered.
        case QGraphicsItem::ItemPositionHasChanged:
        case QGraphicsItem::ItemVisibleHasChanged:
        case QGraphicsItem::ItemSceneHasChanged:
        case QGraphicsItem::ItemScenePositionHasChanged:
            d->collapseToMin();
            break;

        // We'll simply do nothing for all other changes.
        default:
            break;
    }

    return Context::Applet::itemChange( change, value );
}

#include "LyricsApplet.moc"

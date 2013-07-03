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
#include "context/widgets/AppletHeader.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "context/LyricsManager.h"
#include "LyricsBrowser.h"
#include "LyricsSuggestionsListWidget.h"
#include "scriptmanager/ScriptManager.h"

#include <KConfigDialog>
#include <KStandardDirs>
#include <KTextBrowser>

#include <Plasma/IconWidget>

#include <QAction>
#include <QGraphicsLinearLayout>
#include <QScrollBar>
#include <QTimer>

class LyricsAppletPrivate
{
public:
    LyricsAppletPrivate( LyricsApplet *parent )
        : saveIcon( 0 )
        , editIcon( 0 )
        , autoScrollIcon( 0 )
        , reloadIcon( 0 )
        , closeIcon( 0 )
        , settingsIcon( 0 )
        , browser( 0 )
        , suggestView( 0 )
        , currentTrack( 0 )
        , alignment( Qt::AlignLeft )
        , hasLyrics( false )
        , showBrowser( false )
        , showSuggestions( false )
        , isShowingUnsavedWarning( false )
        , userAutoScrollOffset( 0 )
        , oldSliderPosition( 0 )
        , q_ptr( parent ) {}
    ~LyricsAppletPrivate() {}

    // member functions
    void setEditing( bool isEditing );
    void determineActionIconsState();
    void refetchLyrics();
    void showLyrics( const QString &text );
    void showSuggested( const QVariantList &suggestions );
    void showUnsavedChangesWarning( Meta::TrackPtr );

    // private slots
    void _editLyrics();
    void _changeLyricsAlignment();
    void _changeLyricsFont();
    void _closeLyrics();
    void _saveLyrics();
    void _toggleAutoScroll();
    void _suggestionChosen( const LyricsSuggestion &suggestion );
    void _unsetCursor();
    void _trackDataChanged( Meta::TrackPtr );
    void _trackPositionChanged( qint64 position, bool userSeek );

    void _lyricsChangedMessageButtonPressed( const Plasma::MessageButton button );
    void _refetchMessageButtonPressed( const Plasma::MessageButton button );

    Plasma::IconWidget *saveIcon;
    Plasma::IconWidget *editIcon;
    Plasma::IconWidget *autoScrollIcon;
    Plasma::IconWidget *reloadIcon;
    Plasma::IconWidget *closeIcon;
    Plasma::IconWidget *settingsIcon;

    LyricsBrowser      *browser;
    LyricsSuggestionsListWidget *suggestView;

    Ui::lyricsSettings ui_settings;

    Meta::TrackPtr currentTrack;
    Meta::TrackPtr modifiedTrack;
    QString modifiedLyrics;

    Qt::Alignment alignment;

    bool hasLyrics;
    bool showBrowser;
    bool autoScroll;
    bool showSuggestions;
    bool isShowingUnsavedWarning;
    int  userAutoScrollOffset;
    int  oldSliderPosition;

private:
    LyricsApplet *const q_ptr;
    Q_DECLARE_PUBLIC( LyricsApplet )
};

void
LyricsAppletPrivate::setEditing( bool isEditing )
{
    browser->setReadOnly( !isEditing );
}

void
LyricsAppletPrivate::determineActionIconsState()
{
    bool isEditing = !browser->isReadOnly();

    editIcon->action()->setEnabled( !isEditing );
    closeIcon->action()->setEnabled( isEditing );
    saveIcon->action()->setEnabled( isEditing );
    autoScrollIcon->action()->setEnabled( !isEditing );
    reloadIcon->action()->setEnabled( !isEditing );
}

void
LyricsAppletPrivate::showLyrics( const QString &text )
{
    browser->clear();
    browser->setLyrics( text );
    showSuggestions = false;
    showBrowser = true;
    determineActionIconsState();
}

void
LyricsAppletPrivate::showSuggested( const QVariantList &suggestions )
{
    editIcon->action()->setEnabled( false );
    closeIcon->action()->setEnabled( false );
    saveIcon->action()->setEnabled( false );

    suggestView->clear();
    foreach( const QVariant &suggestion, suggestions )
    {
        QStringList s( suggestion.toStringList() );
        QString title( s.at(0) );
        QString artist( s.at(1) );
        KUrl url( s.at(2) );
        LyricsSuggestion lyricsSuggestion = { url, title, artist };
        suggestView->add( lyricsSuggestion );
    }
    showSuggestions = true;
}

void
LyricsAppletPrivate::refetchLyrics()
{
    DEBUG_BLOCK
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
    modifiedLyrics = browser->lyrics();

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
    q->showWarning( warningMessage, SLOT(_lyricsChangedMessageButtonPressed(Plasma::MessageButton)) );

    // Make the contents readonly again.
    // Since the applet is now blocked the user can not enable this again.
    // Thus we can make sure that we won't overwrite modifiedTrack.
    setEditing( false );

    isShowingUnsavedWarning = false;
}

void LyricsAppletPrivate::_refetchMessageButtonPressed( const Plasma::MessageButton button )
{
    DEBUG_BLOCK
    // Check if the user pressed "Yes".
    if( button == Plasma::ButtonYes )
        // Refetch the lyrics.
        refetchLyrics();
}

void LyricsAppletPrivate::_lyricsChangedMessageButtonPressed( const Plasma::MessageButton button )
{
    DEBUG_BLOCK
    // Check if the user pressed "Yes".
    if( button == Plasma::ButtonYes )
        // Update the lyrics of the track.
        modifiedTrack->setCachedLyrics( modifiedLyrics );

    modifiedLyrics.clear();
}

void
LyricsAppletPrivate::_changeLyricsAlignment()
{
    if( ui_settings.alignLeft->isChecked() )
        alignment = Qt::AlignLeft;
    else if( ui_settings.alignCenter->isChecked() )
        alignment = Qt::AlignCenter;
    else if( ui_settings.alignRight->isChecked() )
        alignment = Qt::AlignRight;
    Amarok::config("Lyrics Applet").writeEntry( "Alignment", int(alignment) );
    browser->setAlignment( alignment );
}

void
LyricsAppletPrivate::_changeLyricsFont()
{
    QFont font = ui_settings.fontChooser->font();
    browser->nativeWidget()->setFont( font );
    KConfigGroup config = Amarok::config("Lyrics Applet");
    config.writeEntry( "Font", font.toString() );
    debug() << "Setting Lyrics Applet font: " << font.family() << " " << font.pointSize();
}

void
LyricsAppletPrivate::_editLyrics()
{
    if( !hasLyrics )
        browser->clear();

    Q_Q( LyricsApplet );
    if( q->isCollapsed() )
        q->setCollapseOff();

    // disable autoscroll when starting editing
    if (autoScroll)
        _toggleAutoScroll();

    if( !browser->isVisible() )
    {
        browser->show();
        suggestView->hide();
        suggestView->clear();
        QGraphicsLinearLayout *lo = static_cast<QGraphicsLinearLayout*>( q->layout() );
        lo->removeItem( suggestView );
        lo->addItem( browser );
    }

    browser->setAlignment( Qt::AlignLeft );
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

        showLyrics( currentTrack->cachedLyrics() );
        vbar->setSliderPosition( savedPosition );
        // emit sizeHintChanged(Qt::MaximumSize);
    }
    else
    {
        browser->clear();
    }

    setEditing( false );
    browser->setAlignment( alignment );
    determineActionIconsState();
}

void
LyricsAppletPrivate::_saveLyrics()
{
    if( currentTrack )
    {
        if( !LyricsManager::self()->isEmpty( browser->nativeWidget()->toPlainText() ) )
        {
            currentTrack->setCachedLyrics( browser->lyrics() );
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
    browser->setAlignment( alignment );
    determineActionIconsState();
}

void
LyricsAppletPrivate::_toggleAutoScroll()
{
    Q_Q( LyricsApplet );
    Plasma::IconWidget *icon = qobject_cast<Plasma::IconWidget*>(q->sender());
    DEBUG_ASSERT( icon, return ) // that should not happen

    autoScroll = !autoScroll;
    icon->setPressed( autoScroll );
    Amarok::config( "Lyrics Applet" ).writeEntry( "AutoScroll", autoScroll );
}

void
LyricsAppletPrivate::_suggestionChosen( const LyricsSuggestion &suggestion )
{
    DEBUG_BLOCK
    KUrl url = suggestion.url;
    if( !url.isValid() )
        return;

    QString title = suggestion.title;
    QString artist = suggestion.artist;

    Q_Q( LyricsApplet );
    debug() << "clicked suggestion" << url;
    ScriptManager::instance()->notifyFetchLyricsByUrl( artist, title, url.url() );
    suggestView->setCursor( Qt::BusyCursor );
    QTimer::singleShot( 10000, q, SLOT(_unsetCursor()) );
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
    userAutoScrollOffset = 0;
    oldSliderPosition = 0;

    // Check if we previously had a track.
    // If the lyrics currently shown in the browser (which
    // additionally is in edit mode) are different from the
    // lyrics of the track we have to show a warning.
    if( !isShowingUnsavedWarning && currentTrack &&
        !browser->isReadOnly() &&
        (currentTrack->cachedLyrics() != browser->lyrics()) )
    {
        isShowingUnsavedWarning = true;
        showUnsavedChangesWarning( track );
    }

    // Update the current track.
    currentTrack = track;
}

void
LyricsAppletPrivate::_trackPositionChanged( qint64 position, bool userSeek )
{
    Q_UNUSED( userSeek );
    EngineController *engine = The::engineController();
    QScrollBar *vbar = browser->nativeWidget()->verticalScrollBar();
    if( engine->trackPositionMs() != 0 &&  !vbar->isSliderDown() && autoScroll )
    {
        userAutoScrollOffset = userAutoScrollOffset + vbar->value() - oldSliderPosition;

        //prevent possible devision by 0 (example streams).
        if( engine->trackLength() == 0 )
            return;
        // Scroll to try and keep the current position in the lyrics centred.
        int newSliderPosition =
            position * (vbar->maximum() + vbar->pageStep()) / engine->trackLength() -
            vbar->pageStep() / 2 + userAutoScrollOffset;
        vbar->setSliderPosition( newSliderPosition );

        oldSliderPosition = vbar->value();
    }
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
    DEBUG_BLOCK

    Q_D( LyricsApplet );

    // Call the base implementation.
    Context::Applet::init();

    enableHeader( true );
    setHeaderText( i18n( "Lyrics" ) );

    setCollapseOffHeight( -1 );
    setCollapseHeight( m_header->height() );
    setMinimumHeight( collapseHeight() );
    setPreferredHeight( collapseHeight() );

    QAction* editAction = new QAction( this );
    editAction->setIcon( KIcon( "document-edit" ) );
    editAction->setEnabled( false );
    editAction->setText( i18n( "Edit Lyrics" ) );
    d->editIcon = addLeftHeaderAction( editAction );
    connect( d->editIcon, SIGNAL(clicked()), this, SLOT(_editLyrics()) );

    QAction* saveAction = new QAction( this );
    saveAction->setIcon( KIcon( "document-save" ) );
    saveAction->setEnabled( false );
    saveAction->setText( i18n( "Save Lyrics" ) );
    d->saveIcon = addLeftHeaderAction( saveAction );
    connect( d->saveIcon, SIGNAL(clicked()), this, SLOT(_saveLyrics()) );

    QAction* closeAction = new QAction( this );
    closeAction->setIcon( KIcon( "document-close" ) );
    closeAction->setEnabled( false );
    closeAction->setText( i18n( "Close" ) );
    d->closeIcon = addLeftHeaderAction( closeAction );
    connect( d->closeIcon, SIGNAL(clicked()), this, SLOT(_closeLyrics()) );

    QAction* autoScrollAction = new QAction( this );
    autoScrollAction->setIcon( KIcon( QPixmap( KStandardDirs::locate( "data", "amarok/images/playlist-sorting-16.png" ) ) ) );
    autoScrollAction->setEnabled( true );
    autoScrollAction->setText( i18n( "Scroll automatically" ) );
    d->autoScrollIcon = addRightHeaderAction( autoScrollAction );
    connect( d->autoScrollIcon, SIGNAL(clicked()), this, SLOT(_toggleAutoScroll()) );

    QAction* reloadAction = new QAction( this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setEnabled( true );
    reloadAction->setText( i18n( "Reload Lyrics" ) );
    d->reloadIcon = addRightHeaderAction( reloadAction );
    connect( d->reloadIcon, SIGNAL(clicked()), this, SLOT(refreshLyrics()) );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    d->settingsIcon = addRightHeaderAction( settingsAction );
    connect( d->settingsIcon, SIGNAL(clicked()), this, SLOT(showConfigurationInterface()) );

    d->browser = new LyricsBrowser( this );
    d->browser->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    d->browser->hide();

    d->suggestView = new LyricsSuggestionsListWidget( this );
    d->suggestView->hide();

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical );
    layout->addItem( m_header );
    layout->addItem( d->browser );
    setLayout( layout );

    // Read config
    const KConfigGroup &lyricsConfig = Amarok::config("Lyrics Applet");
    d->alignment = Qt::Alignment( lyricsConfig.readEntry("Alignment", int(Qt::AlignLeft)) );
    d->browser->setAlignment( d->alignment );
    d->autoScroll = lyricsConfig.readEntry( "AutoScroll", true );
    d->autoScrollIcon->setPressed( d->autoScroll );

    QFont font;
    if( font.fromString( lyricsConfig.readEntry("Font", QString()) ) )
        d->browser->setFont( font );

    EngineController* engine = The::engineController();

    connect( engine, SIGNAL(trackChanged(Meta::TrackPtr)), this, SLOT(_trackDataChanged(Meta::TrackPtr)) );
    connect( engine, SIGNAL(trackMetadataChanged(Meta::TrackPtr)), this, SLOT(_trackDataChanged(Meta::TrackPtr)) );
    connect( engine, SIGNAL(trackPositionChanged(qint64,bool)), this, SLOT(_trackPositionChanged(qint64,bool)) );
    connect( d->suggestView, SIGNAL(selected(LyricsSuggestion)), SLOT(_suggestionChosen(LyricsSuggestion)) );
    connect( dataEngine("amarok-lyrics"), SIGNAL(sourceAdded(QString)), this, SLOT(connectSource(QString)) );

    // This is needed as a track might be playing when the lyrics applet
    // is added to the ContextView.
    d->_trackDataChanged( engine->currentTrack() );
    d->_trackPositionChanged( engine->trackPositionMs(), false );

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
LyricsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_D( LyricsApplet );

    if( name != QLatin1String("lyrics") )
        return;

    unsetCursor();
    d->hasLyrics = false;
    d->showSuggestions = false;
    d->showBrowser = false;
    setBusy( false );
    QString titleText;

    if( data.contains( "noscriptrunning" ) )
    {
        titleText = i18n( "Lyrics: No script is running" );
        setCollapseOn();
    }
    else if( data.contains( "stopped" ) )
    {
        titleText = i18n( "Lyrics" );
        setCollapseOn();
    }
    else if( data.contains( "fetching" ) )
    {
        if( canAnimate() )
            setBusy( true );
        titleText = i18n( "Lyrics: Fetching ..." );
    }
    else if( data.contains( "error" ) )
    {
        titleText = i18n( "Lyrics: Fetch error" );
        setCollapseOn();
    }
    else if( data.contains( "suggested" ) )
    {
        QVariantList suggested = data[ "suggested" ].toList();
        titleText = i18n( "Lyrics: Suggested URLs" );
        d->showSuggested( suggested );
        setCollapseOff();
    }
    else if( data.contains( "html" ) || data.contains( "lyrics" ) )
    {
        const bool isHtml = data.contains( QLatin1String("html") );
        const QString key = isHtml ? QLatin1String("html") : QLatin1String("lyrics");
        const QVariant var = data.value( key );
        if( var.canConvert<LyricsData>() )
        {
            d->hasLyrics = true;
            d->browser->setRichText( isHtml );
            LyricsData lyrics = var.value<LyricsData>();
            QString trimmed = lyrics.text.trimmed();

            if( trimmed != d->browser->lyrics() )
            {
                d->showLyrics( trimmed );
            }
            else // lyrics are the same, make sure browser is showing
            {
                d->showSuggestions = false;
                d->showBrowser = true;
            }

            titleText = i18nc( "Lyrics: <artist> - <title>", "Lyrics: %1 - %2", lyrics.artist, lyrics.title );
            setCollapseOff();
        }
    }
    else if( data.contains( "notfound" ) || data.contains( "notFound" ) )
    {
        titleText = i18n( "Lyrics: Not found" );
        setCollapseOn();
    }
    else
    {
        warning() << "should not be here:" << data;
        titleText = headerText();
    }

    setHeaderText( titleText );

    QGraphicsLinearLayout *lo = static_cast<QGraphicsLinearLayout*>( layout() );
    d->showSuggestions ? lo->insertItem( 1, d->suggestView ) : lo->removeItem( d->suggestView );
    d->showBrowser     ? lo->addItem( d->browser )           : lo->removeItem( d->browser );

    d->suggestView->setVisible( d->showSuggestions );
    d->browser->setVisible( d->showBrowser );

    if( !d->showSuggestions )
        d->suggestView->clear();

    d->determineActionIconsState();
}

bool
LyricsApplet::hasHeightForWidth() const
{
    return false;
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
        showWarning( text, SLOT(_refetchMessageButtonPressed(Plasma::MessageButton)) );
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

    parent->setButtons( KDialog::Ok | KDialog::Cancel );

    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    d->ui_settings.setupUi( settings );
    d->ui_settings.fontChooser->setFont( d->browser->nativeWidget()->currentFont() );

    switch( d->alignment )
    {
    default:
    case Qt::AlignLeft:
        d->ui_settings.alignLeft->setChecked( true );
        break;

    case Qt::AlignRight:
        d->ui_settings.alignRight->setChecked( true );
        break;

    case Qt::AlignCenter:
        d->ui_settings.alignCenter->setChecked( true );
        break;
    }

    parent->addPage( settings, i18n( "Lyrics Settings" ), "preferences-system" );

    connect( parent, SIGNAL(accepted()), this, SLOT(_changeLyricsFont()) );
    connect( parent, SIGNAL(accepted()), this, SLOT(_changeLyricsAlignment()) );
    connect( parent, SIGNAL(applyClicked()), this, SLOT(_changeLyricsFont()) );
    connect( parent, SIGNAL(applyClicked()), this, SLOT(_changeLyricsAlignment()) );
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

#include "LyricsApplet.moc"

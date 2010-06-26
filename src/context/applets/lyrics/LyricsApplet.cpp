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
        , hasLyrics( false )
        , isRichText( true )
        , q_ptr( parent ) {}
    ~LyricsAppletPrivate() {}

    // member functions
    void setEditing( const bool isEditing );
    void collaspeToMin();
    void determineActionIconsState();
    void showLyrics( const QString &text, bool isRichText );
    void showSuggested( const QVariantList &suggestions );

    // private slots
    void _editLyrics();
    void _changeLyricsFont();
    void _closeLyrics();
    void _saveLyrics();
    void _suggestionChosen( const QModelIndex &index );
    void _unsetCursor();

    // data / widgets
    QString titleText;
    TextScrollingWidget *titleLabel;

    Plasma::IconWidget *saveIcon;
    Plasma::IconWidget *editIcon;
    Plasma::IconWidget *reloadIcon;
    Plasma::IconWidget *closeIcon;
    Plasma::IconWidget *settingsIcon;

    Plasma::TextBrowser *browser;
    Plasma::TreeView    *suggestView;

    Plasma::Label *infoLabel;

    Ui::lyricsSettings ui_settings;

    bool hasLyrics;
    bool isRichText;

private:
    LyricsApplet *const q_ptr;
    Q_DECLARE_PUBLIC( LyricsApplet )
};

void
LyricsAppletPrivate::setEditing( const bool isEditing )
{
    Q_Q( LyricsApplet );
    browser->nativeWidget()->setReadOnly( !isEditing );
    q->update();
    // d->collaspeToMin();
}

void
LyricsAppletPrivate::collaspeToMin()
{
    Q_Q( LyricsApplet );
    KTextBrowser *textBrowser = browser->nativeWidget();
    if( !textBrowser )
        return;

    // use a dummy item to get the lyrics layout being displayed
    QGraphicsTextItem testItem;
    testItem.setTextWidth( textBrowser->document()->size().width() );
    testItem.setFont( textBrowser->currentFont() );
    testItem.setHtml( textBrowser->toHtml() );

    const qreal padding        = q->standardPadding();
    const qreal frameWidth     = textBrowser->frameWidth();
    const qreal testItemHeight = testItem.boundingRect().height();
    const qreal headerHeight   = titleLabel->pos().y() + titleLabel->boundingRect().height() + padding;
    const qreal contentHeight  = headerHeight + frameWidth + testItemHeight + frameWidth + padding;

    // only show vertical scrollbar if there are lyrics and is needed
    textBrowser->setVerticalScrollBarPolicy( hasLyrics ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff );

    // maybe we were just added, don't have a view yet
    if( !q->containment()->view() )
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
    bool isVisible = browser->nativeWidget()->isVisible();
    bool isEditing = !browser->nativeWidget()->isReadOnly();

    editIcon->action()->setEnabled( isVisible & !isEditing );

    closeIcon->action()->setEnabled( isVisible & isEditing );
    closeIcon->action()->setVisible( isVisible & isEditing );

    saveIcon->action()->setEnabled( isVisible & isEditing );
    saveIcon->action()->setVisible( isVisible & isEditing );

    reloadIcon->action()->setEnabled( isVisible & !isEditing );
}

void
LyricsAppletPrivate::showLyrics( const QString &text, bool isRichText )
{
    browser->nativeWidget()->clear();
    if( isRichText )
        browser->nativeWidget()->setHtml( text );
    else
        browser->nativeWidget()->setPlainText( text );
    determineActionIconsState();
    suggestView->hide();
    browser->show();
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
    suggestView->show();
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
    // collaspeToMin();
}

void
LyricsAppletPrivate::_editLyrics()
{
    Q_Q( LyricsApplet );
    if( !hasLyrics )
        browser->nativeWidget()->clear();

    setEditing( true );
    determineActionIconsState();
    browser->nativeWidget()->ensureCursorVisible();
    q->setCollapseOff();
}

void
LyricsAppletPrivate::_closeLyrics()
{
    Q_Q( LyricsApplet );
    if( hasLyrics )
    {
        QScrollBar *vbar = browser->nativeWidget()->verticalScrollBar();
        int savedPosition = vbar->isVisible() ? vbar->value() : vbar->minimum();

        if( isRichText )
            browser->nativeWidget()->setHtml( The::engineController()->currentTrack()->cachedLyrics() );
        else
            browser->nativeWidget()->setPlainText( The::engineController()->currentTrack()->cachedLyrics() );

        vbar->setSliderPosition( savedPosition );
        q->setCollapseOff();

        determineActionIconsState();
        suggestView->hide();
        browser->show();
        // emit sizeHintChanged(Qt::MaximumSize);
    }
    else
    {
        browser->nativeWidget()->clear();
    }

    setEditing( false );
    determineActionIconsState();
}

void
LyricsAppletPrivate::_saveLyrics()
{
    Q_Q( LyricsApplet );
    Meta::TrackPtr curtrack = The::engineController()->currentTrack();

    if( curtrack )
    {
        if( !browser->nativeWidget()->toPlainText().isEmpty() )
        {
            const QString lyrics = isRichText ? browser->nativeWidget()->toHtml() : browser->nativeWidget()->toPlainText();
            curtrack->setCachedLyrics( lyrics );
            q->setCollapseOff();
            hasLyrics = true;
        }
        else
        {
            curtrack->setCachedLyrics( QString() );
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
        ScriptManager::instance()->notifyFetchLyricsByUrl( artist, title, url );
        suggestView->setCursor( Qt::BusyCursor );
        QTimer::singleShot( 10000, q, SLOT(_unsetCursor()) );
    }
    suggestView->hide();
}

void
LyricsAppletPrivate::_unsetCursor()
{
    if( suggestView->hasCursor() )
        suggestView->unsetCursor();
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

    d->browser = new Plasma::TextBrowser( this );
    KTextBrowser *browserWidget = d->browser->nativeWidget();
    browserWidget->setFrameShape( QFrame::StyledPanel );
    browserWidget->setAttribute( Qt::WA_NoSystemBackground );
    browserWidget->setOpenExternalLinks( true );
    browserWidget->setUndoRedoEnabled( true );
    browserWidget->setAutoFillBackground( false );
    browserWidget->setWordWrapMode( QTextOption::WordWrap );
    browserWidget->viewport()->setAttribute( Qt::WA_NoSystemBackground );
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

    // Read config
    QFont font;
    if( font.fromString( Amarok::config("Lyrics Applet").readEntry("Font", QString()) ) )
        browserWidget->setFont( font );

    connect( suggestTree, SIGNAL(activated(QModelIndex)), this, SLOT(_suggestionChosen(QModelIndex)) );
    connect( dataEngine("amarok-lyrics"), SIGNAL(sourceAdded(QString)), this, SLOT(connectSource(QString)) );
    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(paletteChanged(QPalette)) );

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
    Q_UNUSED( constraints );
    Q_D( LyricsApplet );

    prepareGeometryChange();

    // Assumes all icons are of equal width
    const qreal iconWidth = d->settingsIcon->size().width();
    const qreal padding = standardPadding();

    // Y offsets for title label and icons so their centers line up
    const qreal titleY = padding + 3;
    const qreal iconY = titleY + (d->titleLabel->boundingRect().height() - iconWidth) / 2;

    // label position
    d->titleLabel->setScrollingText( d->titleText );
    d->titleLabel->setPos( ( size().width() - d->titleLabel->boundingRect().width() ) / 2 , titleY );

    // settings icon position
    d->settingsIcon->setPos( size().width() - iconWidth - padding, iconY );

    // reload icon position
    d->reloadIcon->setPos( d->settingsIcon->pos().x() - padding - iconWidth, iconY );
    d->reloadIcon->show();

    // edit and close icon positions
    QPoint editIconPos( d->reloadIcon->pos().x() - padding - iconWidth, iconY );
    d->editIcon->setPos( editIconPos );
    d->closeIcon->setPos( editIconPos );

    // save icon position
    d->saveIcon->setPos( d->editIcon->pos().x() - padding - iconWidth, iconY );

    // proxy position and height depending on whether suggestions are showing
    qreal proxyY = d->titleLabel->pos().y() + d->titleLabel->boundingRect().height() + padding;
    qreal proxyH = boundingRect().height() - proxyY - padding;

    if( d->suggestView->isVisible() )
    {
        // suggest view position
        QPointF suggestViewPos( padding, proxyY );
        proxyH = proxyH / 3.0 - proxyY;
        qreal suggestViewRatio = d->browser->isVisible() ? 3.0 : 1.0;
        qreal suggestViewHeight( boundingRect().height() / suggestViewRatio - proxyY - padding );
        QSizeF suggestViewSize( size().width() - 2 * padding, suggestViewHeight );
        d->suggestView->setPos( suggestViewPos );
        d->suggestView->setMinimumSize( suggestViewSize );
        d->suggestView->setMaximumSize( suggestViewSize );
        proxyY = suggestViewPos.y() + suggestViewSize.height() + padding;
        proxyH = boundingRect().height() - proxyY - padding;
    }

    if( d->infoLabel->isVisible() )
    {
        // info label position
        QSizeF infoSize( (boundingRect().width() / 3), (proxyH / 3) );
        QPointF infoPos( (boundingRect().width() - infoSize.width()) / 2, (proxyH - infoSize.height()) / 2 );
        d->infoLabel->setPos( infoPos );
        d->infoLabel->setMinimumSize( infoSize );
        d->infoLabel->setMaximumSize( infoSize );
    }

    // browser position
    QPointF textBrowserPos( padding, proxyY );
    QSizeF textBrowserSize( size().width() - 2 * padding, proxyH );
    d->browser->setPos( textBrowserPos );
    d->browser->setMinimumSize( textBrowserSize );
    d->browser->setMaximumSize( textBrowserSize );
    update();
}

void
LyricsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name )
    Q_D( LyricsApplet );

    unsetCursor();
    d->hasLyrics = false;
    d->suggestView->hide();
    d->browser->hide();
    d->infoLabel->hide();
    setBusy( false );

    if( data.contains( "noscriptrunning" ) )
    {
        d->titleText = i18n( "Lyrics: No script is running" );
    }
    else if( data.contains( "stopped" ) )
    {
        d->titleText = i18n( "Lyrics" );
    }
    else if( data.contains( "fetching" ) )
    {
        if( canAnimate() )
            setBusy( true );
        d->titleText = i18n( "Lyrics: Fetching ..." );
        d->infoLabel->setText( i18n( "Lyrics are being fetched" ) );
        d->infoLabel->show();
    }
    else if( data.contains( "error" ) )
    {
        d->titleText = i18n( "Lyrics: Fetch error" );
        d->infoLabel->setText( i18n( "Could not download lyrics.\n"
                                     "Please check your Internet connection.\n"
                                     "Error message:\n"
                                     "%1", data["error"].toString() ) );
        d->infoLabel->show();
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
        d->infoLabel->show();
    }

    d->determineActionIconsState();
    constraintsEvent();
    update();
}

bool
LyricsApplet::hasHeightForWidth() const
{
    return false;
}

void
LyricsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );
    Q_D( LyricsApplet );

    p->setRenderHint( QPainter::Antialiasing );

    // tint the whole applet
    addGradientToAppletBackground( p );

    // draw rounded rect around title (only if not animating )
    if ( !d->titleLabel->isAnimating() )
        drawRoundedRectAroundText( p, d->titleLabel );

    if( !d->browser->isVisible() )
        return;

    QColor background;
    if( d->browser->nativeWidget()->isReadOnly() )
        background = The::paletteHandler()->backgroundColor();
    else // different background color when we're in edit mode
        background = The::paletteHandler()->alternateBackgroundColor();

    p->save();
    const QScrollBar *hScrollBar = d->browser->nativeWidget()->horizontalScrollBar();
    const QScrollBar *vScrollBar = d->browser->nativeWidget()->verticalScrollBar();
    qreal hScrollBarHeight = hScrollBar->isVisible() ? hScrollBar->height() : 0;
    qreal vScrollBarWidth  = vScrollBar->isVisible() ? vScrollBar->width()  : 0;
    int frameWidth = d->browser->nativeWidget()->frameWidth();
    QPointF proxyPos( d->browser->pos().x() + frameWidth, d->browser->pos().y() + frameWidth );
    QSizeF proxySize( d->browser->size().width()  - vScrollBarWidth  - frameWidth * 2,
                      d->browser->size().height() - hScrollBarHeight - frameWidth * 2 );
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
    Q_D( LyricsApplet );
    Meta::TrackPtr curtrack = The::engineController()->currentTrack();

    if( !curtrack || !curtrack->artist() )
        return;

    bool refetch = true;
    if( d->hasLyrics )
    {
        const QString text( i18nc( "@info", "Do you really want to refetch lyrics for this track? All changes you may have made will be lost.") );
        refetch = KMessageBox::warningContinueCancel( 0, text, i18n( "Refetch lyrics" ) ) == KMessageBox::Continue;
    }

    if( refetch )
        ScriptManager::instance()->notifyFetchLyrics( curtrack->artist()->name(), curtrack->name() );
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
        switch( e->key() )
        {
        case Qt::Key_Escape :
            d->_closeLyrics();
            break;

        case Qt::Key_F2 :
            d->_editLyrics();
            break;
        }

        if( e->matches( QKeySequence::Save ) )
            d->_saveLyrics();

        e->accept();
    }
    else
    {
        Context::Applet::keyPressEvent( e );
    }
}

#include "LyricsApplet.moc"

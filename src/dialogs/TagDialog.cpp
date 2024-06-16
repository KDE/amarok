/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>                         *
 * Copyright (c) 2005-2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>             *
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2009 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
 * Copyright (c) 2009 Pierre Dumuid <pmdumuid@gmail.com>                                *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "TagDialog"

#include "TagDialog.h"

#include "MainWindow.h"
#include "SvgHandler.h"
#include "core/collections/QueryMaker.h"
#include "core/logger/Logger.h"
#include "core/meta/Statistics.h"
#include "core/meta/TrackEditor.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "covermanager/CoverFetchingActions.h"
#include "dialogs/MusicBrainzTagger.h"
#include "widgets/CoverLabel.h"
#include "widgets/FilenameLayoutWidget.h"
#include "ui_TagDialogBase.h" // needs to be after including CoverLabel, silly
#include "TagGuesserDialog.h"

#include <QLineEdit>
#include <QMenu>
#include <QTimer>

#include <KRun>

#include <thread>


namespace Meta {
namespace Field {
    const QString LABELS = "labels";
    const QString LYRICS = "lyrics";
    const QString TYPE = "type";
    const QString COLLECTION = "collection";
    const QString NOTE = "note";
}
}

TagDialog::TagDialog( const Meta::TrackList &tracks, QWidget *parent )
    : QDialog( parent )
    , m_perTrack( true )
    , m_currentTrackNum( 0 )
    , m_changed( false )
    , m_queryMaker( nullptr )
    , ui( new Ui::TagDialogBase() )
{
    DEBUG_BLOCK

    for( Meta::TrackPtr track : tracks )
        addTrack( track );

    ui->setupUi( this );
    resize( minimumSizeHint() );
    initUi();
    setCurrentTrack( 0 );
}

TagDialog::TagDialog( Meta::TrackPtr track, QWidget *parent )
    : QDialog( parent )
    , m_perTrack( true )
    , m_currentTrackNum( 0 )
    , m_changed( false )
    , m_queryMaker( nullptr )
    , ui( new Ui::TagDialogBase() )
{
    DEBUG_BLOCK

    addTrack( track );
    ui->setupUi( this );
    resize( minimumSizeHint() );
    initUi();
    setCurrentTrack( 0 );

    QTimer::singleShot( 0, this, &TagDialog::show );
}

TagDialog::TagDialog( Collections::QueryMaker *qm )
    : QDialog( The::mainWindow() )
    , m_perTrack( true )
    , m_currentTrackNum( 0 )
    , m_changed( false )
    , m_queryMaker( qm )
    , ui( new Ui::TagDialogBase() )
{
    DEBUG_BLOCK

    ui->setupUi( this );
    resize( minimumSizeHint() );

    qm->setQueryType( Collections::QueryMaker::Track );
    connect( qm, &Collections::QueryMaker::newArtistsReady, this, &TagDialog::artistsReady, Qt::QueuedConnection );
    connect( qm, &Collections::QueryMaker::newTracksReady, this, &TagDialog::tracksReady, Qt::QueuedConnection );
    connect( qm, &Collections::QueryMaker::newAlbumsReady, this, &TagDialog::albumsReady, Qt::QueuedConnection );
    connect( qm, &Collections::QueryMaker::newComposersReady, this, &TagDialog::composersReady, Qt::QueuedConnection );
    connect( qm, &Collections::QueryMaker::newGenresReady, this, &TagDialog::genresReady, Qt::QueuedConnection );
    connect( qm, &Collections::QueryMaker::newLabelsReady, this, &TagDialog::labelsReady, Qt::QueuedConnection );
    connect( qm, &Collections::QueryMaker::queryDone, this, &TagDialog::queryDone, Qt::QueuedConnection );
    qm->run();
}

TagDialog::~TagDialog()
{
    DEBUG_BLOCK

    Amarok::config( "TagDialog" ).writeEntry( "CurrentTab", ui->qTabWidget->currentIndex() );

    if( m_currentAlbum )
        unsubscribeFrom( m_currentAlbum );

    // kRichTextEdit creates a signal during deletion which causes getTagsFromUi to access deleted objects BUG: 428769
    ui->kRichTextEdit_lyrics->disconnect();

    delete ui;
}

void
TagDialog::metadataChanged( const Meta::AlbumPtr &album )
{
    if( m_currentAlbum )
        return;

    // If the metadata of the current album has changed, reload the cover
    if( album == m_currentAlbum )
        updateCover();

    // TODO: if the lyrics changed: should we show a warning and ask the user
    // if he wants to use the new lyrics?
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
////////////////////////////////////////////////////////////////////////////////

void
TagDialog::addTrack( Meta::TrackPtr &track )
{
    if( !m_tracks.contains( track ) )
    {
        m_tracks.append( track );
        m_storedTags.insert( track, getTagsFromTrack( track ) );
    }
}

void
TagDialog::tracksReady( const Meta::TrackList &tracks )
{
    for( Meta::TrackPtr track : tracks )
        addTrack( track );
}

void
TagDialog::queryDone()
{
    delete m_queryMaker;
    if( !m_tracks.isEmpty() )
    {
        initUi();
        setCurrentTrack( 0 );

        QTimer::singleShot( 0, this, &TagDialog::show );
    }
    else
    {
        deleteLater();
    }
}

void
TagDialog::albumsReady( const Meta::AlbumList &albums )
{
    for( const Meta::AlbumPtr &album : albums )
    {
        if( !album->name().isEmpty() )
            m_albums << album->name();

        if( album->hasAlbumArtist() && !album->albumArtist()->name().isEmpty() )
            m_albumArtists << album->albumArtist()->name();
    }
}

void
TagDialog::artistsReady( const Meta::ArtistList &artists )
{
    for( const Meta::ArtistPtr &artist : artists )
    {
        if( !artist->name().isEmpty() )
            m_artists << artist->name();
    }
}

void
TagDialog::composersReady( const Meta::ComposerList &composers )
{
    for( const Meta::ComposerPtr &composer : composers )
    {
        if( !composer->name().isEmpty() )
            m_composers << composer->name();
    }
}

void
TagDialog::genresReady( const Meta::GenreList &genres )
{
    for( const Meta::GenrePtr &genre : genres )
    {
        if( !genre->name().isEmpty() )  // Where the heck do the empty genres come from?
            m_genres << genre->name();
    }
}


void
TagDialog::labelsReady( const Meta::LabelList &labels )
{
    for( const Meta::LabelPtr &label : labels )
    {
        if( !label->name().isEmpty() )
            m_allLabels << label->name();
    }
}

void
TagDialog::dataQueryDone()
{
    // basically we want to ignore the fact that the fields are being
    // edited because we do it not the user, so it results in empty
    // tags being saved to files---data loss is BAD!
    bool oldChanged = m_changed;

    //we simply clear the completion data of all comboboxes
    //then load the current track again. that's more work than necessary
    //but the performance impact should be negligible
    // we do this because if we insert items and the contents of the textbox
    // are not in the list, it clears the textbox. which is bad --lfranchi 2.22.09
    QString saveText( ui->kComboBox_artist->lineEdit()->text() );
    QStringList artists = m_artists.values();
    artists.sort();
    ui->kComboBox_artist->clear();
    ui->kComboBox_artist->insertItems( 0, artists );
    ui->kComboBox_artist->completionObject()->setItems( artists );
    ui->kComboBox_artist->lineEdit()->setText( saveText );

    saveText = ui->kComboBox_album->lineEdit()->text();
    QStringList albums = m_albums.values();
    albums.sort();
    ui->kComboBox_album->clear();
    ui->kComboBox_album->insertItems( 0, albums );
    ui->kComboBox_album->completionObject()->setItems( albums );
    ui->kComboBox_album->lineEdit()->setText( saveText );

    saveText = ui->kComboBox_albumArtist->lineEdit()->text();
    QStringList albumArtists = m_albumArtists.values();
    albumArtists.sort();
    ui->kComboBox_albumArtist->clear();
    ui->kComboBox_albumArtist->insertItems( 0, albumArtists );
    ui->kComboBox_albumArtist->completionObject()->setItems( albumArtists );
    ui->kComboBox_albumArtist->lineEdit()->setText( saveText );

    saveText = ui->kComboBox_composer->lineEdit()->text();
    QStringList composers = m_composers.values();
    composers.sort();
    ui->kComboBox_composer->clear();
    ui->kComboBox_composer->insertItems( 0, composers );
    ui->kComboBox_composer->completionObject()->setItems( composers );
    ui->kComboBox_composer->lineEdit()->setText( saveText );

    saveText = ui->kComboBox_genre->lineEdit()->text();
    QStringList genres = m_genres.values();
    genres.sort();
    ui->kComboBox_genre->clear();
    ui->kComboBox_genre->insertItems( 0, genres );
    ui->kComboBox_genre->completionObject()->setItems( genres );
    ui->kComboBox_genre->lineEdit()->setText( saveText );

    saveText = ui->kComboBox_label->lineEdit()->text();
    QStringList labels = m_allLabels.values();
    labels.sort();
    ui->kComboBox_label->clear();
    ui->kComboBox_label->insertItems( 0, labels );
    ui->kComboBox_label->completionObject()->setItems( labels );
    ui->kComboBox_label->lineEdit()->setText( saveText );

    m_changed = oldChanged;
}

void
TagDialog::removeLabelPressed() //SLOT
{
    if( ui->labelsList->selectionModel()->hasSelection() )
    {
        QModelIndexList idxList = ui->labelsList->selectionModel()->selectedRows();
        QStringList selection;

        for( int x = 0; x < idxList.size(); ++x )
        {
            QString label = idxList.at(x).data( Qt::DisplayRole ).toString();
            selection.append( label );
        }

        m_labelModel->removeLabels( selection );

        ui->labelsList->selectionModel()->reset();
        labelSelected();

        checkChanged();
    }
}

void
TagDialog::addLabelPressed() //SLOT
{
    QString label = ui->kComboBox_label->currentText();

    if( !label.isEmpty() )
    {
        m_labelModel->addLabel( label );
        ui->kComboBox_label->setCurrentIndex( -1 );
        ui->kComboBox_label->completionObject()->insertItems( QStringList( label ) );

        if ( !ui->kComboBox_label->contains( label ) )
            ui->kComboBox_label->addItem( label );

        checkChanged();
    }
}

void
TagDialog::cancelPressed() //SLOT
{
    QApplication::restoreOverrideCursor(); // restore the cursor before closing the dialog (The musicbrainz dialog might have set it)
    reject();
}


void
TagDialog::accept() //SLOT
{
    ui->pushButton_ok->setEnabled( false ); //visual feedback
    saveTags();

    QDialog::accept();
}


inline void
TagDialog::openPressed() //SLOT
{
    new KRun( QUrl::fromLocalFile(m_path), this );
}


inline void
TagDialog::previousTrack() //SLOT
{
    setCurrentTrack( m_currentTrackNum - 1 );
}


inline void
TagDialog::nextTrack() //SLOT
{
    setCurrentTrack( m_currentTrackNum + 1 );
}

inline void
TagDialog::perTrack( bool enabled ) //SLOT
{
    if( enabled == m_perTrack )
        return;

    setTagsToTrack();
    setPerTrack( enabled );
    setTagsToUi();
}


void
TagDialog::checkChanged() //SLOT
{
    QVariantMap oldTags;
    if( m_perTrack )
        oldTags = m_storedTags.value( m_currentTrack );
    else
        oldTags = getTagsFromMultipleTracks();
    QVariantMap newTags = getTagsFromUi( oldTags );

    ui->pushButton_ok->setEnabled( m_changed || !newTags.isEmpty() );
}

inline void
TagDialog::labelModified() //SLOT
{
    ui->addButton->setEnabled( ui->kComboBox_label->currentText().length()>0 );
}

inline void
TagDialog::labelSelected() //SLOT
{
    ui->removeButton->setEnabled( ui->labelsList->selectionModel()->hasSelection() );
}

//creates a QDialog and executes the FilenameLayoutWidget. Grabs a filename scheme, extracts tags (via TagGuesser) from filename and fills the appropriate fields on TagDialog.
void
TagDialog::guessFromFilename() //SLOT
{
    TagGuesserDialog dialog( m_currentTrack->playableUrl().path(), this );

    if( dialog.exec() == QDialog::Accepted )
    {
        dialog.onAccept();

        int cur = 0;

        QMap<qint64,QString> tags = dialog.guessedTags();

        if( !tags.isEmpty() )
        {

            if( tags.contains( Meta::valTitle ) )
                ui->kLineEdit_title->setText( tags[Meta::valTitle] );

            if( tags.contains( Meta::valArtist ) )
            {
                cur = ui->kComboBox_artist->currentIndex();
                ui->kComboBox_artist->setItemText( cur, tags[Meta::valArtist] );
            }

            if( tags.contains( Meta::valAlbum ) )
            {
                cur = ui->kComboBox_album->currentIndex();
                ui->kComboBox_album->setItemText( cur, tags[Meta::valAlbum] );
            }

            if( tags.contains( Meta::valAlbumArtist ) )
            {
                cur = ui->kComboBox_albumArtist->currentIndex();
                ui->kComboBox_albumArtist->setItemText( cur, tags[Meta::valAlbumArtist] );
            }

            if( tags.contains( Meta::valTrackNr ) )
                ui->qSpinBox_track->setValue( tags[Meta::valTrackNr].toInt() );

            if( tags.contains( Meta::valComment ) )
                ui->qPlainTextEdit_comment->setPlainText( tags[Meta::valComment] );

            if( tags.contains( Meta::valYear ) )
                ui->qSpinBox_year->setValue( tags[Meta::valYear].toInt() );

            if( tags.contains( Meta::valComposer ) )
            {
                cur = ui->kComboBox_composer->currentIndex();
                ui->kComboBox_composer->setItemText( cur, tags[Meta::valComposer] );
            }

            if( tags.contains( Meta::valGenre ) )
            {
                cur = ui->kComboBox_genre->currentIndex();
                ui->kComboBox_genre->setItemText( cur, tags[Meta::valGenre] );
            }

            if( tags.contains( Meta::valDiscNr ) )
            {
                ui->qSpinBox_discNumber->setValue( tags[Meta::valDiscNr].toInt() );
            }
        }
        else
        {
            debug() << "guessing tags from filename failed" << Qt::endl;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// PRIVATE
////////////////////////////////////////////////////////////////////////////////

void TagDialog::initUi()
{
    DEBUG_BLOCK
    // delete itself when closing
    setAttribute( Qt::WA_DeleteOnClose );

    KConfigGroup config = Amarok::config( "TagDialog" );

    ui->qTabWidget->addTab( ui->summaryTab   , i18n( "Summary" ) );
    ui->qTabWidget->addTab( ui->tagsTab      , i18n( "Tags" ) );
    ui->qTabWidget->addTab( ui->lyricsTab    , i18n( "Lyrics" ) );
    ui->qTabWidget->addTab( ui->labelsTab , i18n( "Labels" ) );

    ui->kComboBox_label->completionObject()->setIgnoreCase( true );
    ui->kComboBox_label->setCompletionMode( KCompletion::CompletionPopup );

    m_labelModel = new LabelListModel( QStringList(), this );
    ui->labelsList->setModel( m_labelModel );
    ui->labelsTab->setEnabled( true );

    ui->qTabWidget->setCurrentIndex( config.readEntry( "CurrentTab", 0 ) );

    ui->kComboBox_artist->completionObject()->setIgnoreCase( true );
    ui->kComboBox_artist->setCompletionMode( KCompletion::CompletionPopup );

    ui->kComboBox_album->completionObject()->setIgnoreCase( true );
    ui->kComboBox_album->setCompletionMode( KCompletion::CompletionPopup );

    ui->kComboBox_albumArtist->completionObject()->setIgnoreCase( true );
    ui->kComboBox_albumArtist->setCompletionMode( KCompletion::CompletionPopup );

    ui->kComboBox_composer->completionObject()->setIgnoreCase( true );
    ui->kComboBox_composer->setCompletionMode( KCompletion::CompletionPopup );

    ui->kComboBox_genre->completionObject()->setIgnoreCase( true );
    ui->kComboBox_genre->setCompletionMode( KCompletion::CompletionPopup );

    ui->kComboBox_label->completionObject()->setIgnoreCase( true );
    ui->kComboBox_label->setCompletionMode( KCompletion::CompletionPopup );

    ui->addButton->setEnabled( false );
    ui->removeButton->setEnabled( false );

    // set an icon for the open-in-konqui button
    ui->pushButton_open->setIcon( QIcon::fromTheme( QStringLiteral("folder-amarok") ) );

    connect( ui->pushButton_guessTags, &QAbstractButton::clicked, this, &TagDialog::guessFromFilename );

    // Connects for modification check
    // only set to overwrite-on-save if the text has changed
    connect( ui->kLineEdit_title,       &QLineEdit::textChanged, this, &TagDialog::checkChanged );
    connect( ui->kComboBox_composer,    QOverload<int>::of(&QComboBox::activated), this, &TagDialog::checkChanged );
    connect( ui->kComboBox_composer,    &QComboBox::editTextChanged, this, &TagDialog::checkChanged );
    connect( ui->kComboBox_artist,      QOverload<int>::of(&QComboBox::activated), this, &TagDialog::checkChanged );
    connect( ui->kComboBox_artist,      &QComboBox::editTextChanged, this, &TagDialog::checkChanged );
    connect( ui->kComboBox_album,       QOverload<int>::of(&QComboBox::activated), this, &TagDialog::checkChanged );
    connect( ui->kComboBox_album,       &QComboBox::editTextChanged, this, &TagDialog::checkChanged );
    connect( ui->kComboBox_albumArtist, QOverload<int>::of(&QComboBox::activated), this, &TagDialog::checkChanged );
    connect( ui->kComboBox_albumArtist, &QComboBox::editTextChanged, this, &TagDialog::checkChanged );
    connect( ui->kComboBox_genre,       QOverload<int>::of(&QComboBox::activated), this, &TagDialog::checkChanged );
    connect( ui->kComboBox_genre,       &QComboBox::editTextChanged, this, &TagDialog::checkChanged );
    connect( ui->kLineEdit_Bpm,         &QLineEdit::textChanged, this, &TagDialog::checkChanged );
    connect( ui->ratingWidget,          QOverload<int>::of(&KRatingWidget::ratingChanged), this, &TagDialog::checkChanged );
    connect( ui->qSpinBox_track,        QOverload<int>::of(&QSpinBox::valueChanged), this, &TagDialog::checkChanged );
    connect( ui->qSpinBox_year,         QOverload<int>::of(&QSpinBox::valueChanged), this, &TagDialog::checkChanged );
    connect( ui->qSpinBox_score,        QOverload<int>::of(&QSpinBox::valueChanged), this, &TagDialog::checkChanged );
    connect( ui->qPlainTextEdit_comment, &QPlainTextEdit::textChanged, this, &TagDialog::checkChanged );
    connect( ui->kRichTextEdit_lyrics,  &QTextEdit::textChanged, this, &TagDialog::checkChanged );
    connect( ui->qSpinBox_discNumber,   QOverload<int>::of(&QSpinBox::valueChanged), this, &TagDialog::checkChanged );

    connect( ui->pushButton_cancel,   &QAbstractButton::clicked, this, &TagDialog::cancelPressed );
    connect( ui->pushButton_ok,       &QAbstractButton::clicked, this, &TagDialog::accept );
    connect( ui->pushButton_open,     &QAbstractButton::clicked, this, &TagDialog::openPressed );
    connect( ui->pushButton_previous, &QAbstractButton::clicked, this, &TagDialog::previousTrack );
    connect( ui->pushButton_next,     &QAbstractButton::clicked, this, &TagDialog::nextTrack );
    connect( ui->checkBox_perTrack,   &QCheckBox::toggled, this, &TagDialog::perTrack );

    connect( ui->addButton,           &QAbstractButton::clicked, this, &TagDialog::addLabelPressed );
    connect( ui->removeButton,        &QAbstractButton::clicked, this, &TagDialog::removeLabelPressed );
    connect( ui->kComboBox_label,     QOverload<int>::of(&KComboBox::activated), this, &TagDialog::labelModified );
    connect( ui->kComboBox_label,     &QComboBox::editTextChanged, this, &TagDialog::labelModified );
    connect( ui->kComboBox_label,     QOverload<const QString&>::of(&KComboBox::returnPressed), this, &TagDialog::addLabelPressed );
    connect( ui->kComboBox_label,     QOverload<const QString&>::of(&KComboBox::returnPressed), this, &TagDialog::checkChanged );
    connect( ui->labelsList,          &QListView::pressed, this, &TagDialog::labelSelected );

    ui->pixmap_cover->setContextMenuPolicy( Qt::CustomContextMenu );
    connect( ui->pixmap_cover, &CoverLabel::customContextMenuRequested, this, &TagDialog::showCoverMenu );

    connect( ui->pushButton_musicbrainz, &QAbstractButton::clicked, this, &TagDialog::musicbrainzTagger );

    if( m_tracks.count() > 1 )
        setPerTrack( false );
    else
        setPerTrack( true );

    ui->pushButton_ok->setEnabled( false );

    startDataQueries();
}

void
TagDialog::setCurrentTrack( int num )
{
    if( num < 0 || num >= m_tracks.count() )
        return;

    if( m_currentTrack ) // even in multiple tracks mode we don't want to write back
        setTagsToTrack();

    // there is a logical problem here.
    // if the track itself changes (e.g. because it get's a new album)
    // then we don't re-subscribe
    if( m_currentAlbum )
        unsubscribeFrom( m_currentAlbum );

    m_currentTrack = m_tracks.at( num );
    m_currentAlbum = m_currentTrack->album();
    m_currentTrackNum = num;

    if( m_currentAlbum )
        subscribeTo( m_currentAlbum );

    setControlsAccessability();
    updateButtons();
    setTagsToUi();
}

void
TagDialog::startDataQuery( Collections::QueryMaker::QueryType type, const QMetaMethod &signal,
                           const QMetaMethod &slot )
{
    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    qm->setQueryType( type );

    connect( qm, &Collections::QueryMaker::queryDone, this, &TagDialog::dataQueryDone, Qt::QueuedConnection );
    connect( qm, signal, this, slot, Qt::QueuedConnection );

    qm->setAutoDelete( true );
    qm->run();
}

void
TagDialog::startDataQueries()
{
    startDataQuery( Collections::QueryMaker::Artist,
                    QMetaMethod::fromSignal(&Collections::QueryMaker::newArtistsReady),
                    QMetaMethod::fromSignal(&TagDialog::artistsReady) );
    startDataQuery( Collections::QueryMaker::Album,
                    QMetaMethod::fromSignal(&Collections::QueryMaker::newAlbumsReady),
                    QMetaMethod::fromSignal(&TagDialog::albumsReady) );
    startDataQuery( Collections::QueryMaker::Composer,
                    QMetaMethod::fromSignal(&Collections::QueryMaker::newComposersReady),
                    QMetaMethod::fromSignal(&TagDialog::composersReady) );
    startDataQuery( Collections::QueryMaker::Genre,
                    QMetaMethod::fromSignal(&Collections::QueryMaker::newGenresReady),
                    QMetaMethod::fromSignal(&TagDialog::genresReady) );
    startDataQuery( Collections::QueryMaker::Label,
                    QMetaMethod::fromSignal(&Collections::QueryMaker::newLabelsReady),
                    QMetaMethod::fromSignal(&TagDialog::labelsReady) );
}


inline const QString
TagDialog::unknownSafe( const QString &s ) const
{
    return ( s.isNull() || s.isEmpty() || s == "?" || s == "-" )
           ? i18nc( "The value for this tag is not known", "Unknown" )
           : s;
}

inline const QString
TagDialog::unknownSafe( int i ) const
{
    return ( i == 0 )
           ? i18nc( "The value for this tag is not known", "Unknown" )
           : QString::number( i );
}

void
TagDialog::showCoverMenu( const QPoint &pos )
{
    if( !m_currentAlbum )
        return; // TODO: warning or something?

    QAction *displayCoverAction = new DisplayCoverAction( this, m_currentAlbum );
    QAction *unsetCoverAction   = new UnsetCoverAction( this, m_currentAlbum );

    if( !m_currentAlbum->hasImage() )
    {
        displayCoverAction->setEnabled( false );
        unsetCoverAction->setEnabled( false );
    }

    QMenu *menu = new QMenu( this );
    menu->addAction( displayCoverAction );
    menu->addAction( new FetchCoverAction( this, m_currentAlbum ) );
    menu->addAction( new SetCustomCoverAction( this, m_currentAlbum ) );
    menu->addAction( unsetCoverAction );

    menu->exec( ui->pixmap_cover->mapToGlobal(pos) );
    delete menu;
}

void
TagDialog::setTagsToUi( const QVariantMap &tags )
{
    bool oldChanged = m_changed;

    // -- the windows title
    if( m_perTrack )
    {
        setWindowTitle( i18n("Track Details: %1 by %2",
                                                           m_currentTrack->name(),  m_currentTrack->artist() ? m_currentTrack->artist()->name() : QString() ) );

    }
    else
    {
        setWindowTitle( i18ncp( "The amount of tracks being edited", "1 Track", "Information for %1 Tracks", m_tracks.count() ) );

    }

    // -- the title in the summary tab

    if( m_perTrack )
    {
        QString niceTitle;

        const QFontMetrics fnt =  ui->trackArtistAlbumLabel->fontMetrics();
        const int len = ui->trackArtistAlbumLabel->width();
        QString curTrackAlbName;
        QString curArtistName;

        QString curTrackName = fnt.elidedText( m_currentTrack->name().toHtmlEscaped(), Qt::ElideRight, len );
        QString curTrackPretName = fnt.elidedText( m_currentTrack->prettyName().toHtmlEscaped(), Qt::ElideRight, len );

        if( m_currentAlbum )
            curTrackAlbName = fnt.elidedText( m_currentAlbum->name().toHtmlEscaped(), Qt::ElideRight, len );
        if( m_currentTrack->artist() )
            curArtistName = fnt.elidedText( m_currentTrack->artist()->name().toHtmlEscaped(), Qt::ElideRight, len );


        if( m_currentAlbum && m_currentAlbum->name().isEmpty() )
        {
            if( !m_currentTrack->name().isEmpty() )
            {
                if( !m_currentTrack->artist()->name().isEmpty() )
                    niceTitle = i18n( "<b>%1</b> by <b>%2</b>", curTrackName,  curArtistName );
                else
                    niceTitle = i18n( "<b>%1</b>", curTrackName );
            }
            else
                niceTitle = curTrackPretName;
        }
        else if( m_currentAlbum )
            niceTitle = i18n( "<b>%1</b> by <b>%2</b> on <b>%3</b>" , curTrackName, curArtistName, curTrackAlbName );
        else if( m_currentTrack->artist() )
            niceTitle = i18n( "<b>%1</b> by <b>%2</b>" , curTrackName, curArtistName );
        else
            niceTitle = i18n( "<b>%1</b>" , curTrackName );

        ui->trackArtistAlbumLabel->setText( niceTitle );
    }
    else
    {
        ui->trackArtistAlbumLabel->setText( i18np( "Editing 1 file", "Editing %1 files", m_tracks.count() ) );

    }

    // -- the rest

    ui->kLineEdit_title->setText( tags.value( Meta::Field::TITLE ).toString() );
    selectOrInsertText( tags.value( Meta::Field::ARTIST ).toString(), ui->kComboBox_artist );
    selectOrInsertText( tags.value( Meta::Field::ALBUM ).toString(), ui->kComboBox_album );
    selectOrInsertText( tags.value( Meta::Field::ALBUMARTIST ).toString(), ui->kComboBox_albumArtist );
    selectOrInsertText( tags.value( Meta::Field::COMPOSER ).toString(), ui->kComboBox_composer );
    ui->qPlainTextEdit_comment->setPlainText( tags.value( Meta::Field::COMMENT ).toString() );
    selectOrInsertText( tags.value( Meta::Field::GENRE ).toString(), ui->kComboBox_genre );
    ui->qSpinBox_track->setValue( tags.value( Meta::Field::TRACKNUMBER ).toInt() );
    ui->qSpinBox_discNumber->setValue( tags.value( Meta::Field::DISCNUMBER ).toInt() );
    ui->qSpinBox_year->setValue( tags.value( Meta::Field::YEAR ).toInt() );
    ui->kLineEdit_Bpm->setText( tags.value( Meta::Field::BPM ).toString() );

    ui->qLabel_length->setText( unknownSafe( Meta::msToPrettyTime( tags.value( Meta::Field::LENGTH ).toLongLong() ) ) );
    ui->qLabel_bitrate->setText( Meta::prettyBitrate( tags.value( Meta::Field::BITRATE ).toInt() ) );
    ui->qLabel_samplerate->setText( unknownSafe( tags.value( Meta::Field::SAMPLERATE ).toInt() ) );
    ui->qLabel_size->setText( Meta::prettyFilesize( tags.value( Meta::Field::FILESIZE ).toLongLong() ) );
    ui->qLabel_format->setText( unknownSafe( tags.value( Meta::Field::TYPE ).toString() ) );

    ui->qSpinBox_score->setValue( tags.value( Meta::Field::SCORE ).toInt() );
    ui->ratingWidget->setRating( tags.value( Meta::Field::RATING ).toInt() );
    ui->ratingWidget->setMaxRating( 10 );
    int playcount = tags.value( Meta::Field::PLAYCOUNT ).toInt();
    ui->qLabel_playcount->setText( unknownSafe( playcount ) );

    QDateTime firstPlayed = tags.value( Meta::Field::FIRST_PLAYED ).toDateTime();
    ui->qLabel_firstPlayed->setText( Amarok::verboseTimeSince( firstPlayed ) );

    QDateTime lastPlayed = tags.value( Meta::Field::LAST_PLAYED ).toDateTime();
    ui->qLabel_lastPlayed->setText( Amarok::verboseTimeSince( lastPlayed ) );

    ui->qLabel_collection->setText( tags.contains( Meta::Field::COLLECTION ) ?
                                    tags.value( Meta::Field::COLLECTION ).toString() :
                                    i18nc( "The collection this track is part of", "None") );

    // special handling - we want to hide this if empty
    if( tags.contains( Meta::Field::NOTE ) )
    {
        ui->noteLabel->show();
        ui->qLabel_note->setText( tags.value( Meta::Field::NOTE ).toString() );
        ui->qLabel_note->show();
    }
    else
    {
        ui->noteLabel->hide();
        ui->qLabel_note->hide();
    }

    ui->kRichTextEdit_lyrics->setTextOrHtml( tags.value( Meta::Field::LYRICS ).toString() );

    m_labelModel->setLabels( tags.value( Meta::Field::LABELS ).toStringList() );
    ui->labelsList->update();

    updateCover();

    setControlsAccessability();

    // If it's a local file, write the directory to m_path, else disable the "open in konqui" button
    QString urlString = tags.value( Meta::Field::URL ).toString();
    QUrl url = QUrl::fromUserInput( urlString );
    //QUrl::PreferLocalFile will give localpath or proper url for remote.
    ui->kLineEdit_location->setText( url.toDisplayString( QUrl::PreferLocalFile ) );
    if( url.isLocalFile() )
    {
        ui->locationLabel->show();
        ui->kLineEdit_location->show();
        QFileInfo fi( urlString );
        m_path = fi.isDir() ? urlString : url.adjusted(QUrl::RemoveFilename).path();
        ui->pushButton_open->setEnabled( true );
    }
    else
    {
        m_path.clear();
        ui->pushButton_open->setEnabled( false );
    }

    m_changed = oldChanged;
    ui->pushButton_ok->setEnabled( m_changed );
}

void
TagDialog::setTagsToUi()
{
    if( m_perTrack )
        setTagsToUi( m_storedTags.value( m_currentTrack ) );
    else
        setTagsToUi( getTagsFromMultipleTracks() );
}


QVariantMap
TagDialog::getTagsFromUi( const QVariantMap &tags ) const
{
    QVariantMap map;

    if( ui->kLineEdit_title->text() != tags.value( Meta::Field::TITLE ).toString() )
        map.insert( Meta::Field::TITLE, ui->kLineEdit_title->text() );
    if( ui->kComboBox_artist->currentText() != tags.value( Meta::Field::ARTIST ).toString() )
        map.insert( Meta::Field::ARTIST, ui->kComboBox_artist->currentText() );
    if( ui->kComboBox_album->currentText() != tags.value( Meta::Field::ALBUM ).toString() )
        map.insert( Meta::Field::ALBUM, ui->kComboBox_album->currentText() );
    if( ui->kComboBox_albumArtist->currentText() != tags.value( Meta::Field::ALBUMARTIST ).toString() )
        map.insert( Meta::Field::ALBUMARTIST, ui->kComboBox_albumArtist->currentText() );
    if( ui->kComboBox_composer->currentText() != tags.value( Meta::Field::COMPOSER ).toString() )
        map.insert( Meta::Field::COMPOSER, ui->kComboBox_composer->currentText() );
    if( ui->qPlainTextEdit_comment->toPlainText() != tags.value( Meta::Field::COMMENT ).toString() )
        map.insert( Meta::Field::COMMENT, ui->qPlainTextEdit_comment->toPlainText() );
    if( ui->kComboBox_genre->currentText() != tags.value( Meta::Field::GENRE ).toString() )
        map.insert( Meta::Field::GENRE, ui->kComboBox_genre->currentText() );
    if( ui->qSpinBox_track->value() != tags.value( Meta::Field::TRACKNUMBER ).toInt() )
        map.insert( Meta::Field::TRACKNUMBER, ui->qSpinBox_track->value() );
    if( ui->qSpinBox_discNumber->value() != tags.value( Meta::Field::DISCNUMBER ).toInt() )
        map.insert( Meta::Field::DISCNUMBER, ui->qSpinBox_discNumber->value() );
    if( ui->kLineEdit_Bpm->text().toDouble() != tags.value( Meta::Field::BPM ).toReal() )
        map.insert( Meta::Field::BPM, ui->kLineEdit_Bpm->text() );
    if( ui->qSpinBox_year->value() != tags.value( Meta::Field::YEAR ).toInt() )
        map.insert( Meta::Field::YEAR, ui->qSpinBox_year->value() );

    if( ui->qSpinBox_score->value() != tags.value( Meta::Field::SCORE ).toInt() )
        map.insert( Meta::Field::SCORE, ui->qSpinBox_score->value() );

    if( ui->ratingWidget->rating() != tags.value( Meta::Field::RATING ).toUInt() )
        map.insert( Meta::Field::RATING, ui->ratingWidget->rating() );

    if( !m_tracks.count() || m_perTrack )
    { //ignore these on MultipleTracksMode
        if ( ui->kRichTextEdit_lyrics->textOrHtml() != tags.value( Meta::Field::LYRICS ).toString() )
            map.insert( Meta::Field::LYRICS, ui->kRichTextEdit_lyrics->textOrHtml() );
    }

    QStringList uiLabelsList = m_labelModel->labels();
    QSet<QString> uiLabels(uiLabelsList.begin(), uiLabelsList.end());
    QStringList oldLabelsList = tags.value( Meta::Field::LABELS ).toStringList();
    QSet<QString> oldLabels(oldLabelsList.begin(), oldLabelsList.end());
    if( uiLabels != oldLabels )
        map.insert( Meta::Field::LABELS, QVariant( uiLabels.values() ) );

    return map;
}

QVariantMap
TagDialog::getTagsFromTrack( const Meta::TrackPtr &track ) const
{
    QVariantMap map;
    if( !track )
        return map;

    // get the shared pointers now to ensure that they don't get freed
    Meta::AlbumPtr album = track->album();
    Meta::ArtistPtr artist = track->artist();
    Meta::GenrePtr genre = track->genre();
    Meta::ComposerPtr composer = track->composer();
    Meta::YearPtr year = track->year();

    if( !track->name().isEmpty() )
        map.insert( Meta::Field::TITLE, track->name() );
    if( artist && !artist->name().isEmpty() )
        map.insert( Meta::Field::ARTIST, artist->name() );
    if( album && !track->album()->name().isEmpty() )
    {
        map.insert( Meta::Field::ALBUM, album->name() );
        if( album->hasAlbumArtist() && !album->albumArtist()->name().isEmpty() )
            map.insert( Meta::Field::ALBUMARTIST, album->albumArtist()->name() );
    }
    if( composer && !composer->name().isEmpty() )
        map.insert( Meta::Field::COMPOSER, composer->name() );
    if( !track->comment().isEmpty() )
        map.insert( Meta::Field::COMMENT, track->comment() );
    if( genre && !genre->name().isEmpty() )
        map.insert( Meta::Field::GENRE, genre->name() );
    if( track->trackNumber() )
        map.insert( Meta::Field::TRACKNUMBER, track->trackNumber() );
    if( track->discNumber() )
        map.insert( Meta::Field::DISCNUMBER, track->discNumber() );
    if( year && year->year() )
        map.insert( Meta::Field::YEAR, year->year() );
    if( track->bpm() > 0.0)
        map.insert( Meta::Field::BPM, track->bpm() );
    if( track->length() )
        map.insert( Meta::Field::LENGTH, track->length() );
    if( track->bitrate() )
        map.insert( Meta::Field::BITRATE, track->bitrate() );
    if( track->sampleRate() )
        map.insert( Meta::Field::SAMPLERATE, track->sampleRate() );
    if( track->filesize() )
        map.insert( Meta::Field::FILESIZE, track->filesize() );

    Meta::ConstStatisticsPtr statistics = track->statistics();
    map.insert( Meta::Field::SCORE, statistics->score() );
    map.insert( Meta::Field::RATING, statistics->rating() );
    map.insert( Meta::Field::PLAYCOUNT, statistics->playCount() );
    map.insert( Meta::Field::FIRST_PLAYED, statistics->firstPlayed() );
    map.insert( Meta::Field::LAST_PLAYED, statistics->lastPlayed() );
    map.insert( Meta::Field::URL, track->prettyUrl() );

    map.insert( Meta::Field::TYPE, track->type() );

    if( track->inCollection() )
        map.insert( Meta::Field::COLLECTION, track->collection()->prettyName() );

    if( !track->notPlayableReason().isEmpty() )
        map.insert( Meta::Field::NOTE, i18n( "The track is not playable. %1",
                                             track->notPlayableReason() ) );

    QStringList labelNames;
    for( const Meta::LabelPtr &label : track->labels() )
    {
        labelNames << label->name();
    }
    map.insert( Meta::Field::LABELS, labelNames );

    map.insert( Meta::Field::LYRICS, track->cachedLyrics() );

    return map;
}

QVariantMap
TagDialog::getTagsFromMultipleTracks() const
{
    QVariantMap map;
    if( m_tracks.isEmpty() )
        return map;

    //Check which fields are the same for all selected tracks
    QSet<QString> mismatchingTags;

    Meta::TrackPtr first = m_tracks.first();
    map = getTagsFromTrack( first );

    QString directory = first->playableUrl().adjusted(QUrl::RemoveFilename).path();
    int scoreCount = 0;
    double scoreSum = map.value( Meta::Field::SCORE ).toDouble();
    if( map.value( Meta::Field::SCORE ).toDouble() )
        scoreCount++;

    int ratingCount = 0;
    int ratingSum = map.value( Meta::Field::RATING ).toInt();
    if( map.value( Meta::Field::RATING ).toInt() )
        ratingCount++;

    QDateTime firstPlayed = first->statistics()->firstPlayed();
    QDateTime lastPlayed = first->statistics()->lastPlayed();

    qint64 length = first->length();
    qint64 size = first->filesize();
    QStringList validLabels = map.value( Meta::Field::LABELS ).toStringList();

    for( int i = 1; i < m_tracks.count(); i++ )
    {
        Meta::TrackPtr track = m_tracks[i];
        QVariantMap tags = m_storedTags.value( track );

        // -- figure out which tags do not match.

        // - occur not in every file
        QStringList mapkeys=map.keys();
        QStringList tagskeys=tags.keys();
        QSet<QString> mapKeysSet(mapkeys.begin(), mapkeys.end());
        QSet<QString> tagsKeysSet(tagskeys.begin(), tagskeys.end());
        mismatchingTags |= mapKeysSet - tagsKeysSet;
        mismatchingTags |= tagsKeysSet - mapKeysSet;

        // - not the same in every file
        foreach( const QString &key, (mapKeysSet & tagsKeysSet) )
        {
            if( map.value( key ) != tags.value( key ) )
                mismatchingTags.insert( key );
        }

        // -- special handling for values

        // go up in the directories until we find a common one
        QString newDirectory = track->playableUrl().adjusted(QUrl::RemoveFilename).path();

        while( newDirectory != directory )
        {
            if( newDirectory.length() > directory.length() )
            {
                QDir up( newDirectory ); up.cdUp();
                QString d = up.path();
                if( d == newDirectory ) // nothing changed
                {
                    directory.clear();
                    break;
                }
                newDirectory = d;
            }
            else
            {
                QDir up( directory ); up.cdUp();
                QString d = up.path();
                if( d == directory ) // nothing changed
                {
                    directory.clear();
                    break;
                }
                directory = d;
            }
        }
        if( !track->playableUrl().isLocalFile() )
            directory.clear();

        // score and rating (unrated if rating == 0)
        scoreSum += tags.value( Meta::Field::SCORE ).toDouble();
        if( tags.value( Meta::Field::SCORE ).toDouble() )
            scoreCount++;

        ratingSum += tags.value( Meta::Field::RATING ).toInt();
        if( tags.value( Meta::Field::RATING ).toInt() )
            ratingCount++;

        Meta::StatisticsPtr statistics = track->statistics();
        if( statistics->firstPlayed().isValid() &&
            (!firstPlayed.isValid() || statistics->firstPlayed() < firstPlayed) )
            firstPlayed = statistics->firstPlayed();

        if( statistics->lastPlayed().isValid() &&
            (!lastPlayed.isValid() || statistics->lastPlayed() > lastPlayed) )
            lastPlayed = statistics->lastPlayed();

        length += track->length();
        size += track->filesize();

        // Only show labels present in all of the tracks
        QStringList labels = tags.value( Meta::Field::LABELS ).toStringList();
        for ( int x = 0; x < validLabels.count(); x++ )
        {
            if ( !labels.contains( validLabels.at( x ) ) )
                validLabels.removeAt( x );
        }

    }

    for( const QString &key : mismatchingTags )
        map.remove( key );

    map.insert( Meta::Field::URL, directory );
    if( scoreCount > 0 )
        map.insert( Meta::Field::SCORE, scoreSum / scoreCount );
    if( ratingCount > 0 )
        // the extra fuzz is for emulating rounding to nearest integer
        map.insert( Meta::Field::RATING, ( ratingSum + ratingCount / 2 ) / ratingCount );

    map.insert( Meta::Field::FIRST_PLAYED, firstPlayed );
    map.insert( Meta::Field::LAST_PLAYED, lastPlayed );

    map.insert( Meta::Field::LENGTH, length );
    map.insert( Meta::Field::FILESIZE, size );

    map.insert( Meta::Field::LABELS, validLabels );

    return map;
}

void
TagDialog::setTagsToTrack( const Meta::TrackPtr &track, const QVariantMap &tags )
{
    for( const QString &key : tags.keys() )
    {
        m_storedTags[ track ].insert( key, tags.value( key ) );
    }
}

void
TagDialog::setTagsToMultipleTracks( QVariantMap tags )
{
    tags.remove( Meta::Field::LABELS );

    for( const Meta::TrackPtr &track : m_tracks )
    {
        setTagsToTrack( track, tags );
    }
}

void
TagDialog::setTagsToTrack()
{
    QVariantMap oldTags;
    if( m_perTrack )
        oldTags = m_storedTags.value( m_currentTrack );
    else
        oldTags = getTagsFromMultipleTracks();
    QVariantMap newTags = getTagsFromUi( oldTags );

    if( !newTags.isEmpty() )
    {
        m_changed = true;
        if( m_perTrack )
            setTagsToTrack( m_currentTrack, newTags );
        else
        {
            setTagsToMultipleTracks( newTags );

            // -- special handling for labels
            if( newTags.contains( Meta::Field::LABELS ) )
            {
                // determine the differences
                QStringList oldTagsList = oldTags.value( Meta::Field::LABELS ).toStringList();
                QSet<QString> oldLabelsSet(oldTagsList.begin(), oldTagsList.end());
                QStringList newTagsList = newTags.value( Meta::Field::LABELS ).toStringList();
                QSet<QString> newLabelsSet(newTagsList.begin(), newTagsList.end());

                QSet<QString> labelsToRemove = oldLabelsSet - newLabelsSet;
                QSet<QString> labelsToAdd = newLabelsSet - oldLabelsSet;

                // apply the differences for each track
                for( const Meta::TrackPtr &track : m_tracks )
                {
                    QStringList labelsList = m_storedTags[track].value( Meta::Field::LABELS ).toStringList();
                    QSet<QString> labelsSet(labelsList.begin(), labelsList.end());
                    labelsSet += labelsToAdd;
                    labelsSet -= labelsToRemove;

                    m_storedTags[ track ].insert( Meta::Field::LABELS, QVariant( labelsSet.values() ) );
                }
            }
        }
    }
}


void
TagDialog::setPerTrack( bool isEnabled )
{
    debug() << "setPerTrack" << m_tracks.count() << isEnabled;
    if( m_tracks.count() < 2 )
        isEnabled = true;

    /* force an update so that we can use this function in the initialization
    if( m_perTrack == isEnabled )
        return;
    */

    m_perTrack = isEnabled;

    setControlsAccessability();
    updateButtons();
}


void
TagDialog::updateButtons()
{
    ui->pushButton_ok->setEnabled( m_changed );

    ui->checkBox_perTrack->setVisible( m_tracks.count() > 1 );
    ui->pushButton_previous->setVisible( m_tracks.count() > 1 );
    ui->pushButton_next->setVisible( m_tracks.count() > 1 );

    ui->checkBox_perTrack->setChecked( m_perTrack );
    ui->pushButton_previous->setEnabled( m_perTrack && m_currentTrackNum > 0 );
    ui->pushButton_next->setEnabled( m_perTrack && m_currentTrackNum < m_tracks.count()-1 );
}

void
TagDialog::updateCover()
{
    DEBUG_BLOCK

    if( !m_currentTrack )
        return;

    // -- get the album
    Meta::AlbumPtr album = m_currentAlbum;
    if( !m_perTrack )
    {
        for( Meta::TrackPtr track : m_tracks )
        {
            if( track->album() != album )
                album = nullptr;
        }
    }

    // -- set the ui
    const int s = 100; // Image preview size
    ui->pixmap_cover->setMinimumSize( s, s );
    ui->pixmap_cover->setMaximumSize( s, s );

    if( !album )
    {
        ui->pixmap_cover->setVisible( false );
    }
    else
    {
        ui->pixmap_cover->setVisible( true );
        ui->pixmap_cover->setPixmap( The::svgHandler()->imageWithBorder( album, s ) );
        QString artist = m_currentTrack->artist() ? m_currentTrack->artist()->name() : QString();
        ui->pixmap_cover->setInformation( artist, album->name() );
    }
}


void
TagDialog::setControlsAccessability()
{
    bool editable = m_currentTrack ? bool( m_currentTrack->editor() ) : true;

    ui->qTabWidget->setTabEnabled( ui->qTabWidget->indexOf(ui->lyricsTab),
                                   m_perTrack );

    ui->kLineEdit_title->setEnabled( m_perTrack && editable );
    ui->kLineEdit_title->setClearButtonEnabled( m_perTrack && editable );

#define enableOrDisable( X ) \
    ui->X->setEnabled( editable ); \
    qobject_cast<QLineEdit*>(ui->X->lineEdit())->setClearButtonEnabled( editable )

    enableOrDisable( kComboBox_artist );
    enableOrDisable( kComboBox_albumArtist );
    enableOrDisable( kComboBox_composer );
    enableOrDisable( kComboBox_album );
    enableOrDisable( kComboBox_genre );

#undef enableOrDisable

    ui->qSpinBox_track->setEnabled( m_perTrack && editable );
    ui->qSpinBox_discNumber->setEnabled( editable );
    ui->qSpinBox_year->setEnabled( editable );
    ui->kLineEdit_Bpm->setEnabled( editable );
    ui->kLineEdit_Bpm->setClearButtonEnabled( editable );

    ui->qPlainTextEdit_comment->setEnabled( editable );
    ui->pushButton_guessTags->setEnabled( m_perTrack && editable );
    ui->pushButton_musicbrainz->setEnabled( editable );
}

void
TagDialog::saveTags()
{
    setTagsToTrack();

    for( auto &track : m_tracks )
    {
        QVariantMap data = m_storedTags[ track ];
        //there is really no need to write to the file if only info m_stored in the db has changed
        if( !data.isEmpty() )
        {
            debug() << "File info changed....";

            auto lambda = [=] () mutable
            {
                if( data.contains( Meta::Field::SCORE ) )
                    track->statistics()->setScore( data.value( Meta::Field::SCORE ).toInt() );
                if( data.contains( Meta::Field::RATING ) )
                    track->statistics()->setRating( data.value( Meta::Field::RATING ).toInt() );
                if( data.contains( Meta::Field::LYRICS ) )
                    track->setCachedLyrics( data.value( Meta::Field::LYRICS ).toString() );

                QStringList labels = data.value( Meta::Field::LABELS ).toStringList();
                QHash<QString, Meta::LabelPtr> labelMap;
                for( const auto &label : track->labels() )
                    labelMap.insert( label->name(), label );

                // labels to remove
                QStringList labelmapkeys=labelMap.keys();
                QSet<QString> labelMapKeysSet(labelmapkeys.begin(), labelmapkeys.end());
                QSet<QString> labelsSet(labels.begin(), labels.end());
                for( const auto &label : labelMapKeysSet - labelsSet )
                    track->removeLabel( labelMap.value( label ) );

                // labels to add
                for( const auto &label : labelsSet - labelMapKeysSet )
                    track->addLabel( label );

                Meta::TrackEditorPtr ec = track->editor();
                if( !ec )
                {
                    debug() << "Track" << track->prettyUrl() << "does not have Meta::TrackEditor. Skipping.";
                    return;
                }

                ec->beginUpdate();

                if( data.contains( Meta::Field::TITLE ) )
                    ec->setTitle( data.value( Meta::Field::TITLE ).toString() );
                if( data.contains( Meta::Field::COMMENT ) )
                    ec->setComment( data.value( Meta::Field::COMMENT ).toString() );
                if( data.contains( Meta::Field::ARTIST ) )
                    ec->setArtist( data.value( Meta::Field::ARTIST ).toString() );
                if( data.contains( Meta::Field::ALBUM ) )
                    ec->setAlbum( data.value( Meta::Field::ALBUM ).toString() );
                if( data.contains( Meta::Field::GENRE ) )
                    ec->setGenre( data.value( Meta::Field::GENRE ).toString() );
                if( data.contains( Meta::Field::COMPOSER ) )
                    ec->setComposer( data.value( Meta::Field::COMPOSER ).toString() );
                if( data.contains( Meta::Field::YEAR ) )
                    ec->setYear( data.value( Meta::Field::YEAR ).toInt() );
                if( data.contains( Meta::Field::TRACKNUMBER ) )
                    ec->setTrackNumber( data.value( Meta::Field::TRACKNUMBER ).toInt() );
                if( data.contains( Meta::Field::DISCNUMBER ) )
                    ec->setDiscNumber( data.value( Meta::Field::DISCNUMBER ).toInt() );
                if( data.contains( Meta::Field::BPM ) )
                    ec->setBpm( data.value( Meta::Field::BPM ).toDouble() );
                if( data.contains( Meta::Field::ALBUMARTIST ) )
                    ec->setAlbumArtist( data.value( Meta::Field::ALBUMARTIST ).toString() );

                ec->endUpdate();
                // note: the track should by itself Q_EMIT a collectionUpdated signal if needed
            };
            std::thread thread( lambda );
            thread.detach();
        }
    }
}

void
TagDialog::selectOrInsertText( const QString &text, QComboBox *comboBox )
{
    int index = comboBox->findText( text );
    if( index == -1 )
    {
        comboBox->insertItem( 0, text );    //insert at the beginning
        comboBox->setCurrentIndex( 0 );
    }
    else
    {
        comboBox->setCurrentIndex( index );
    }
}

void
TagDialog::musicbrainzTagger()
{
    DEBUG_BLOCK

    MusicBrainzTagger *dialog = new MusicBrainzTagger( m_tracks, this );
    dialog->setWindowTitle( i18n( "MusicBrainz Tagger" ) );
    connect( dialog, &MusicBrainzTagger::sendResult,
             this, &TagDialog::musicbrainzTaggerResult );
    dialog->show();
}

void
TagDialog::musicbrainzTaggerResult( const QMap<Meta::TrackPtr, QVariantMap> &result )
{
    if( result.isEmpty() )
        return;

    for( Meta::TrackPtr track : result.keys() )
    {
        setTagsToTrack( track, result.value( track ) );
    }
    m_changed = true;

    if( m_perTrack )
        setTagsToUi( m_storedTags.value( m_currentTrack ) );
    else
        setTagsToUi( getTagsFromMultipleTracks() );
}

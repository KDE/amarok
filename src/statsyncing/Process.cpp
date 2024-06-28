/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "Process.h"

#include "MainWindow.h"
#include "MetaValues.h"
#include "core/logger/Logger.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "statsyncing/Config.h"
#include "statsyncing/Controller.h"
#include "statsyncing/jobs/MatchTracksJob.h"
#include "statsyncing/jobs/SynchronizeTracksJob.h"
#include "statsyncing/models/MatchedTracksModel.h"
#include "statsyncing/models/ProvidersModel.h"
#include "statsyncing/models/SingleTracksModel.h"
#include "statsyncing/ui/ChooseProvidersPage.h"
#include "statsyncing/ui/MatchedTracksPage.h"

#include <KWindowConfig>

using namespace StatSyncing;

Process::Process( const ProviderPtrList &providers, const ProviderPtrSet &preSelectedProviders,
                  qint64 checkedFields, Process::Mode mode, QObject *parent )
    : QObject( parent )
    , m_mode( mode )
    , m_providersModel( new ProvidersModel( providers, preSelectedProviders, this ) )
    , m_checkedFields( checkedFields )
    , m_matchedTracksModel( nullptr )
    , m_dialog( new QDialog() )
{
    m_dialog->setWindowTitle( i18n( "Synchronize Statistics" ) );
    m_dialog->resize( QSize( 860, 500 ) );
    KWindowConfig::restoreWindowSize( m_dialog->windowHandle(), Amarok::config( QStringLiteral("StatSyncingDialog") ) );

    // delete this process when user hits the close button
    connect( m_dialog.data(), &QDialog::finished, this, &Process::slotSaveSizeAndDelete );

    /* we need to delete all QWidgets on application exit well before QApplication
     * is destroyed. We however don't set MainWindow as parent as this would make
     * StatSyncing dialog share taskbar entry with Amarok main window */
    connect( The::mainWindow(), &MainWindow::destroyed, this, &Process::slotDeleteDialog );
}

Process::~Process()
{
    delete m_dialog.data(); // we cannot deleteLater, dialog references m_matchedTracksModel
}

void
Process::start()
{
    if( m_mode == Interactive )
    {
        m_providersPage = new ChooseProvidersPage();
        m_providersPage->setFields( Controller::availableFields(), m_checkedFields );
        m_providersPage->setProvidersModel( m_providersModel, m_providersModel->selectionModel() );

        connect( m_providersPage.data(), &StatSyncing::ChooseProvidersPage::accepted,
                 this, &Process::slotMatchTracks );
        connect( m_providersPage.data(), &StatSyncing::ChooseProvidersPage::rejected,
                 this, &Process::slotSaveSizeAndDelete );

        for( const auto &child : m_dialog->children() )
        {
            auto widget = qobject_cast<QWidget*>( child );
            if( widget )
            {
                widget->hide(); // otherwise it may last as a ghost image
                widget->deleteLater();
            }
        }
        m_providersPage->setParent( m_dialog ); // takes ownership
        raise();
    }
    else if( m_checkedFields )
        slotMatchTracks();
}

void
Process::raise()
{
    if( m_providersPage || m_tracksPage )
    {
        m_dialog->show();
        m_dialog->activateWindow();
        m_dialog->raise();
    }
    else
        m_mode = Interactive; // schedule dialog should be shown when something happens
}

void
Process::slotMatchTracks()
{
    MatchTracksJob *job = new MatchTracksJob( m_providersModel->selectedProviders() );
    QString text = i18n( "Matching Tracks for Statistics Synchronization" );
    if( m_providersPage )
    {
        ChooseProvidersPage *page = m_providersPage.data(); // too lazy to type
        m_checkedFields = page->checkedFields();

        page->disableControls();
        page->setProgressBarText( text );
        connect( job, &StatSyncing::MatchTracksJob::totalSteps,
                 page, &StatSyncing::ChooseProvidersPage::setProgressBarMaximum );
        connect( job, &StatSyncing::MatchTracksJob::incrementProgress, page,
                 &StatSyncing::ChooseProvidersPage::progressBarIncrementProgress );
        connect( page, &StatSyncing::ChooseProvidersPage::rejected, job, &StatSyncing::MatchTracksJob::abort );
        connect( m_dialog.data(), &QDialog::finished, job, &StatSyncing::MatchTracksJob::abort );
    }
    else // background operation
    {
        Amarok::Logger::newProgressOperation( job, text, 100, job, &MatchTracksJob::abort );
    }

    connect( job, &StatSyncing::MatchTracksJob::done, this, &Process::slotTracksMatched );
    connect( job, &StatSyncing::MatchTracksJob::done, job, &StatSyncing::MatchTracksJob::deleteLater );
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(job) );
}

void
Process::slotTracksMatched( ThreadWeaver::JobPointer job )
{
    MatchTracksJob *matchJob = dynamic_cast<MatchTracksJob *>( job.data() );
    if( !matchJob )
    {
        error() << __PRETTY_FUNCTION__ << "Failed cast, should never happen";
        deleteLater();
        return;
    }
    if( !matchJob->success() )
    {
        warning() << __PRETTY_FUNCTION__ << "MatchTracksJob failed";
        deleteLater();
        return;
    }
    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    if( !controller )
    {
        warning() << __PRETTY_FUNCTION__ << "StatSyncing::Controller disappeared";
        deleteLater();
        return;
    }

    // remove fields that are not writable:
    qint64 usedFields = m_checkedFields & m_providersModel->writableTrackStatsDataUnion();
    m_options.setSyncedFields( usedFields );
    m_options.setExcludedLabels( controller->config()->excludedLabels() );
    QList<qint64> columns = QList<qint64>() << Meta::valTitle;
    for( qint64 field : Controller::availableFields() )
    {
        if( field & usedFields )
            columns << field;
    }

    m_matchedTracksModel = new MatchedTracksModel( matchJob->matchedTuples(), columns,
                                                   m_options, this );
    QList<ScrobblingServicePtr> services = controller->scrobblingServices();
    // only fill in m_tracksToScrobble if there are actual scrobbling services available
    m_tracksToScrobble = services.isEmpty() ? TrackList() : matchJob->tracksToScrobble();

    if( m_matchedTracksModel->hasConflict() || m_mode == Interactive )
    {
        m_tracksPage = new MatchedTracksPage();
        m_tracksPage->setProviders( matchJob->providers() );
        m_tracksPage->setMatchedTracksModel( m_matchedTracksModel );
        for( ProviderPtr provider : matchJob->providers() )
        {
            if( !matchJob->uniqueTracks().value( provider ).isEmpty() )
                m_tracksPage->addUniqueTracksModel( provider, new SingleTracksModel(
                    matchJob->uniqueTracks().value( provider ), columns, m_options, m_tracksPage ) );
            if( !matchJob->excludedTracks().value( provider ).isEmpty() )
                m_tracksPage->addExcludedTracksModel( provider, new SingleTracksModel(
                    matchJob->excludedTracks().value( provider ), columns, m_options, m_tracksPage ) );
        }
        m_tracksPage->setTracksToScrobble( m_tracksToScrobble, services );

        connect( m_tracksPage, &StatSyncing::MatchedTracksPage::back, this, &Process::slotBack );
        connect( m_tracksPage, &StatSyncing::MatchedTracksPage::accepted, this, &Process::slotSynchronize );
        connect( m_tracksPage, &StatSyncing::MatchedTracksPage::rejected, this, &Process::slotSaveSizeAndDelete );
        for( const auto &child : m_dialog->children() )
        {
            auto widget = qobject_cast<QWidget*>( child );
            if( widget )
            {
                widget->hide(); // otherwise it may last as a ghost image
                widget->deleteLater();
            }
        }
        m_tracksPage->setParent( m_dialog ); // takes ownership
        raise();
    }
    else // NonInteractive mode without conflict
        slotSynchronize();
}

void
Process::slotBack()
{
    m_mode = Interactive; // reset mode, we're interactive from this point
    start();
}

void
Process::slotSynchronize()
{
    // disconnect, otherwise we prematurely delete Process and thus m_matchedTracksModel
    disconnect( m_dialog.data(), &QDialog::finished, this, &Process::slotSaveSizeAndDelete );
    m_dialog.data()->close();

    SynchronizeTracksJob *job = new SynchronizeTracksJob(
            m_matchedTracksModel->matchedTuples(), m_tracksToScrobble, m_options );
    QString text = i18n( "Synchronizing Track Statistics" );
    Amarok::Logger::newProgressOperation( job, text, 100, job, &SynchronizeTracksJob::abort );
    connect( job, &StatSyncing::SynchronizeTracksJob::done, this, &Process::slotLogSynchronization );
    connect( job, &StatSyncing::SynchronizeTracksJob::done, job, &StatSyncing::SynchronizeTracksJob::deleteLater );
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(job) );
}

void
Process::slotLogSynchronization( ThreadWeaver::JobPointer job )
{
    deleteLater(); // our work is done
    SynchronizeTracksJob *syncJob = dynamic_cast<SynchronizeTracksJob *>( job.data() );
    if( !syncJob )
    {
        warning() << __PRETTY_FUNCTION__ << "syncJob is null";
        return;
    }

    int updatedTracksCount = syncJob->updatedTracksCount();
    QMap<ScrobblingServicePtr, QMap<ScrobblingService::ScrobbleError, int> > scrobbles =
            syncJob->scrobbles();

    QStringList providerNames;
    for( ProviderPtr provider : m_providersModel->selectedProviders() )
        providerNames << QStringLiteral("<b>") + provider->prettyName() + QStringLiteral("</b>");
    QString providers = providerNames.join( i18nc( "comma between list words", ", " ) );

    QStringList text = QStringList() << i18ncp( "%2 is a list of collection names",
            "Synchronization of %2 done. <b>One</b> track was updated.",
            "Synchronization of %2 done. <b>%1</b> tracks were updated.",
            updatedTracksCount, providers );

    QMap<ScrobblingService::ScrobbleError, int> scrobbleErrorCounts;
    for( const ScrobblingServicePtr &provider : scrobbles.keys() )
    {
        QString name = QStringLiteral("<b>") + provider->prettyName() + QStringLiteral("</b>");
        QMap<ScrobblingService::ScrobbleError, int> &providerScrobbles = scrobbles[ provider ];

        QMapIterator<ScrobblingService::ScrobbleError, int> it( providerScrobbles );
        while( it.hasNext() )
        {
            it.next();
            if( it.key() == ScrobblingService::NoError )
                text << i18np( "<b>One</b> track was queued for scrobbling to %2.",
                        "<b>%1</b> tracks were queued for scrobbling to %2.", it.value(), name );
            else
                scrobbleErrorCounts[ it.key() ] += it.value();
        }
    }
    if( scrobbleErrorCounts.value( ScrobblingService::TooShort ) )
        text << i18np( "<b>One</b> track's played time was too short to be scrobbled.",
                       "<b>%1</b> tracks' played time was too short to be scrobbled.",
                       scrobbleErrorCounts[ ScrobblingService::TooShort ] );
    if( scrobbleErrorCounts.value( ScrobblingService::BadMetadata ) )
        text << i18np( "<b>One</b> track had insufficient metadata to be scrobbled.",
                       "<b>%1</b> tracks had insufficient metadata to be scrobbled.",
                       scrobbleErrorCounts[ ScrobblingService::BadMetadata ] );
    if( scrobbleErrorCounts.value( ScrobblingService::FromTheFuture ) )
        text << i18np( "<b>One</b> track was reported to have been played in the future.",
                       "<b>%1</b> tracks were reported to have been played in the future.",
                       scrobbleErrorCounts[ ScrobblingService::FromTheFuture ] );
    if( scrobbleErrorCounts.value( ScrobblingService::FromTheDistantPast ) )
        text << i18np( "<b>One</b> track was last played in too distant past to be scrobbled.",
                       "<b>%1</b> tracks were last played in too distant past to be scrobbled.",
                       scrobbleErrorCounts[ ScrobblingService::FromTheDistantPast ] );
    if( scrobbleErrorCounts.value( ScrobblingService::SkippedByUser ) )
        text << i18np( "Scrobbling of <b>one</b> track was skipped as configured by the user.",
                       "Scrobbling of <b>%1</b> tracks was skipped as configured by the user.",
                       scrobbleErrorCounts[ ScrobblingService::SkippedByUser ] );

    Amarok::Logger::longMessage( text.join( QStringLiteral("<br>\n") ) );
}

void
Process::slotSaveSizeAndDelete()
{
    if( m_dialog )
    {
        KConfigGroup group = Amarok::config( QStringLiteral("StatSyncingDialog") );
        group.writeEntry( "geometry", m_dialog->saveGeometry() );
    }
    deleteLater();
}

void
Process::slotDeleteDialog()
{
    // we cannot use deleteLater(), we don't have spare eventloop iteration
    delete m_dialog.data();
}

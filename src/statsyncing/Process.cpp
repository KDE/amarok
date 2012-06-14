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

#include "MetaValues.h"
#include "ui_MatchedTracksPage.h"
#include "core/interfaces/Logger.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "statsyncing/jobs/MatchTracksJob.h"
#include "statsyncing/jobs/SynchronizeTracksJob.h"
#include "statsyncing/models/MatchedTracksModel.h"
#include "statsyncing/models/SingleTracksModel.h"
#include "statsyncing/ui/MatchedTracksPage.h"

#include <ThreadWeaver/Weaver>

using namespace StatSyncing;

Process::Process( const QList<QSharedPointer<Provider> > &providers, QObject *parent )
    : QObject( parent )
    , m_providers( providers )
    , m_matchedTracksModel( 0 )
{
    DEBUG_BLOCK
    Q_ASSERT( m_providers.count() > 0 );
    m_options.setSyncedFields( Meta::valRating | Meta::valFirstPlayed |
        Meta::valLastPlayed | Meta::valPlaycount | Meta::valLabel );
}

Process::~Process()
{
    DEBUG_BLOCK
}

void
Process::start()
{
    MatchTracksJob *job = new MatchTracksJob( m_providers );
    QString text = i18n( "Matching tracks..." );
    Amarok::Components::logger()->newProgressOperation( job, text, 100, job, SLOT(abort()) );
    connect( job, SIGNAL(done(ThreadWeaver::Job*)), SLOT(slotTracksMatched(ThreadWeaver::Job*)) );
    ThreadWeaver::Weaver::instance()->enqueue( job );
}

void
Process::raise()
{
    emit signalRaise();
}

void
Process::slotTracksMatched( ThreadWeaver::Job *job )
{
    DEBUG_BLOCK
    // won't be needed after this method returns; needs to be before early-returns to
    // prevent memory and refcounting leak
    job->deleteLater();
    MatchTracksJob *matchJob = dynamic_cast<MatchTracksJob *>( job );
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

    QList<qint64> columns = QList<qint64>() << Meta::valTitle << Meta::valRating <<
        Meta::valFirstPlayed << Meta::valLastPlayed << Meta::valPlaycount << Meta::valLabel;

    MatchedTracksPage *matchedPage = new MatchedTracksPage();
    m_matchedTracksModel = new MatchedTracksModel( matchJob->matchedTuples(), columns,
                                                   m_options, this );
    matchedPage->setMatchedTracksModel( m_matchedTracksModel );
    foreach( QSharedPointer<Provider> providerPointer, m_providers )
    {
        Provider *provider = providerPointer.data();
        if( !matchJob->uniqueTracks().value( provider ).isEmpty() )
            matchedPage->addUniqueTracksModel( provider, new SingleTracksModel(
                matchJob->uniqueTracks().value( provider ), columns, matchedPage ) );
        if( !matchJob->excludedTracks().value( provider ).isEmpty() )
            matchedPage->addExcludedTracksModel( provider, new SingleTracksModel(
                matchJob->excludedTracks().value( provider ), columns, matchedPage ) );
    }

    connect( matchedPage, SIGNAL(accepted()), SLOT(slotSynchronize()) );
    connect( matchedPage, SIGNAL(accepted()), matchedPage, SLOT(deleteLater()) );
    connect( matchedPage, SIGNAL(rejected()), SLOT(deleteLater()) );
    connect( matchedPage, SIGNAL(rejected()), matchedPage, SLOT(deleteLater()) );

    connect( this, SIGNAL(signalRaise()), matchedPage, SLOT(show()) );
    connect( this, SIGNAL(signalRaise()), matchedPage, SLOT(raise()) );
    matchedPage->show();
    matchedPage->raise();
}

void Process::slotSynchronize()
{
    SynchronizeTracksJob *job =
        new SynchronizeTracksJob( m_matchedTracksModel->matchedTuples(), m_options );
    QString text = i18n( "Synchronizing tracks..." );
    Amarok::Components::logger()->newProgressOperation( job, text, 100, job, SLOT(abort()) );
    connect( job, SIGNAL(done(ThreadWeaver::Job*)), job, SLOT(deleteLater()) );
    connect( job, SIGNAL(done(ThreadWeaver::Job*)), SLOT(deleteLater()) );
    ThreadWeaver::Weaver::instance()->enqueue( job );
}

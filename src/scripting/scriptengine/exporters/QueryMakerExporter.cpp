/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#include "QueryMakerExporter.h"

#include "core-impl/collections/support/TextualQueryFilter.h"
#include "core/collections/QueryMaker.h"
#include "scripting/scriptengine/ScriptingDefines.h"

#include <QScriptEngine>
#include <QScriptValue>
#include <QEventLoop>

using namespace AmarokScript;

using Collections::QueryMaker;

void
QueryMakerPrototype::init( QScriptEngine *engine )
{
    qScriptRegisterMetaType<QueryMaker*>( engine, toScriptValue<QueryMaker*,QueryMakerPrototype>, fromScriptValue<QueryMaker*,QueryMakerPrototype> );
}

// script invokable

void
QueryMakerPrototype::addFilter( const QString &filter )
{
    if( !m_querymaker )
        return;
    Collections::addTextualFilter( m_querymaker.data(), filter );
    m_filter += filter + " ";
}

void
QueryMakerPrototype::run()
{
    if( !m_querymaker )
        return;
    m_querymaker.data()->setQueryType( Collections::QueryMaker::Track );
    m_querymaker.data()->run();
}

Meta::TrackList
QueryMakerPrototype::blockingRun()
{
    if( !m_querymaker )
        return Meta::TrackList();
    QEventLoop loop;
    connect( m_querymaker.data(), SIGNAL(newResultReady(Meta::TrackList)), SLOT(slotResult(Meta::TrackList)) );
    connect( m_querymaker.data(), SIGNAL(queryDone()), &loop, SLOT(quit()) );
    run();
    loop.exec();
    return m_result;
}

void
QueryMakerPrototype::abort()
{
    if( m_querymaker )
        m_querymaker.data()->abortQuery();
}

//private

QueryMakerPrototype::QueryMakerPrototype( QueryMaker *queryMaker )
: QObject( 0 ) //engine ownership
, m_querymaker( queryMaker )
{
    if( !queryMaker )
        return;
    connect( queryMaker, SIGNAL(newResultReady(Meta::TrackList)), SIGNAL(newResultReady(Meta::TrackList)) );
    connect( queryMaker, SIGNAL(queryDone()), SIGNAL(queryDone()) );
    queryMaker->setAutoDelete( true );
}

QString
QueryMakerPrototype::filter() const
{
    return m_filter;
}

bool
QueryMakerPrototype::isValid() const
{
    return m_querymaker;
}

void
QueryMakerPrototype::slotResult( const Meta::TrackList &tracks )
{
    m_result << tracks;
}

QueryMakerPrototype::~QueryMakerPrototype()
{
    if( m_querymaker )
        m_querymaker.data()->deleteLater();
}

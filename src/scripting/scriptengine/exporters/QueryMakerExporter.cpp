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
#include "scripting/scriptengine/ScriptingDefines.h"

#include <QJSEngine>
#include <QJSValue>
#include <QEventLoop>

using namespace AmarokScript;

using Collections::QueryMaker;

void
QueryMakerPrototype::init( QJSEngine *engine )
{
    qRegisterMetaType<QueryMaker*>();
    QMetaType::registerConverter<QueryMaker*,QJSValue>( [=] (QueryMaker* queryMaker) { return toScriptValue<QueryMaker*, QueryMakerPrototype>( engine, queryMaker ); } );
    QMetaType::registerConverter<QJSValue,QueryMaker*>( [] (QJSValue jsValue) {
            QueryMaker* queryMaker;
            fromScriptValue<QueryMaker*, QueryMakerPrototype>( jsValue, queryMaker );
            return queryMaker;
    } );
}

// script invokable

void
QueryMakerPrototype::addFilter( const QString &filter )
{
    if( !m_querymaker )
        return;
    Collections::addTextualFilter( m_querymaker.data(), filter );
    m_filter += filter + QLatin1Char(' ');
}

void
QueryMakerPrototype::run()
{
    if( !m_querymaker )
        return;
    m_querymaker->setQueryType( Collections::QueryMaker::Track );
    m_querymaker->run();
}

Meta::TrackList
QueryMakerPrototype::blockingRun()
{
    if( !m_querymaker )
        return Meta::TrackList();
    QEventLoop loop;
    connect( m_querymaker.data(), &Collections::QueryMaker::newTracksReady, this, &QueryMakerPrototype::slotResult );
    connect( m_querymaker.data(), &Collections::QueryMaker::queryDone, &loop, &QEventLoop::quit );
    run();
    loop.exec();
    return m_result;
}

void
QueryMakerPrototype::abort()
{
    if( m_querymaker )
        m_querymaker->abortQuery();
}

//private

QueryMakerPrototype::QueryMakerPrototype( QueryMaker *queryMaker )
: QObject( nullptr ) //engine ownership
, m_querymaker( queryMaker )
{
    if( !queryMaker )
        return;
    connect( queryMaker, &Collections::QueryMaker::newTracksReady, this, &QueryMakerPrototype::newResultReady );
    connect( queryMaker, &Collections::QueryMaker::queryDone, this, &QueryMakerPrototype::queryDone );
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
        m_querymaker->deleteLater();
}

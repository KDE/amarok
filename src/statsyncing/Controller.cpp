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

#include "Controller.h"

#include "core/interfaces/Logger.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "statsyncing/Process.h"
#include "statsyncing/collection/CollectionProvider.h"

using namespace StatSyncing;

static QSet<QString> providerIds( const ProviderPtrSet &providers )
{
    QSet<QString> ret;
    foreach( QSharedPointer<Provider> provider, providers )
        ret.insert( provider->id() );
    return ret;
}

static QSet<QString> readProviders( const KConfigGroup &group, const char *key )
{
    QStringList idList = group.readEntry( key, QStringList() );
    return idList.toSet();
}

static void writeProviders( KConfigGroup &group, const char *key, const QSet<QString> &providers )
{
    group.writeEntry( key, providers.toList() );
}

Controller::Controller( QObject* parent )
    : QObject( parent )
{
}

Controller::~Controller()
{
}

void
Controller::synchronize()
{
    if( m_currentProcess )
    {
        m_currentProcess.data()->raise();
        return;
    }

    ProviderPtrList providers;
    CollectionManager *manager = CollectionManager::instance();
    QHash<Collections::Collection *, CollectionManager::CollectionStatus> collHash =
        manager->collections();
    QHashIterator<Collections::Collection *, CollectionManager::CollectionStatus> it( collHash );
    while( it.hasNext() )
    {
        it.next();
        if( it.value() == CollectionManager::CollectionEnabled )
        {
            QSharedPointer<Provider> provider( new CollectionProvider( it.key() ) );
            providers.append( provider );
        }
    }

    if( providers.count() <= 1 )
    {
        // the text intentionally doesn't cope with 0 collections
        QString text = i18n( "You only seem to have one collection. Statistics "
            "synchronization only makes sense if there is more than one collection." );
        Amarok::Components::logger()->longMessage( text );
        return;
    }

    // read saved config
    KConfigGroup group = Amarok::config( "StatSyncing" );
    qint64 fields = 0;
    QStringList fieldNames = group.readEntry( "checkedFields", QStringList( "FIRST" ) );
    if( fieldNames == QStringList( "FIRST" ) )
        fields = Meta::valRating | Meta::valFirstPlayed | Meta::valLastPlayed |
                Meta::valPlaycount | Meta::valLabel;
    else
    {
        foreach( QString fieldName, fieldNames )
            fields |= Meta::fieldForName( fieldName );
    }
    QSet<QString> checkedProviderIds = readProviders( group, "checkedProviders" );
    QSet<QString> unCheckedProviderIds = readProviders( group, "unCheckedProviders" );
    ProviderPtrSet checkedProviders;
    foreach( QSharedPointer<Provider> provider, providers )
    {
        QString id = provider->id();
        if( unCheckedProviderIds.contains( id ) )
            continue;
        if( checkedProviderIds.contains( id ) || provider->checkedByDefault() )
            checkedProviders.insert( provider );
    }

    m_currentProcess = new Process( providers, checkedProviders, fields, Process::Interactive, this );
    connect( m_currentProcess.data(), SIGNAL(saveSettings(ProviderPtrSet,ProviderPtrSet,qint64)),
             SLOT(saveSettings(ProviderPtrSet,ProviderPtrSet,qint64)) );
    m_currentProcess.data()->start();
}

void
Controller::saveSettings( const ProviderPtrSet &checkedProviders,
                          const ProviderPtrSet &unCheckedProviders,
                          qint64 checkedFields )
{
    KConfigGroup group = Amarok::config( "StatSyncing" );

    QSet<QString> checked = providerIds( checkedProviders );
    QSet<QString> savedChecked = readProviders( group, "checkedProviders" );
    QSet<QString> unChecked = providerIds( unCheckedProviders );
    QSet<QString> savedUnChecked = readProviders( group, "unCheckedProviders" );
    checked = ( checked | savedChecked ) - unChecked;
    unChecked = ( unChecked | savedUnChecked ) - checked;
    writeProviders( group, "checkedProviders", checked );
    writeProviders( group, "unCheckedProviders", unChecked );

    // prefer string representation for fwd compatibility and user-readability
    QStringList fieldNames;
    for( qint64 i = 0; i < 64; i++ )
    {
        qint64 field = 1LL << i;
        if( field & checkedFields )
            fieldNames << Meta::nameForField( field );
    }
    group.writeEntry( "checkedFields", fieldNames );
    group.sync();
}

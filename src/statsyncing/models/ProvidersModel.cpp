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

#include "ProvidersModel.h"

#include "core/meta/support/MetaConstants.h"
#include "statsyncing/Provider.h"

#include <KLocalizedString>

#include <QItemSelectionModel>

using namespace StatSyncing;

ProvidersModel::ProvidersModel( const ProviderPtrList &providers,
                                const ProviderPtrSet &preSelectedProviders, QObject *parent )
    : QAbstractListModel( parent )
    , m_providers( providers )
    , m_selectionModel( new QItemSelectionModel( this, this ) )
{
    // TODO: sort providers

    // selection defaults to model's tick state
    for( int i = 0; i < m_providers.count(); i++ )
    {
        if( preSelectedProviders.contains( m_providers.at( i ) ) )
        {
            QModelIndex idx = index( i );
            m_selectionModel->select( idx, QItemSelectionModel::Select );
        }
    }
    connect( m_selectionModel, &QItemSelectionModel::selectionChanged,
             this, &ProvidersModel::selectedProvidersChanged );
}

ProvidersModel::~ProvidersModel()
{
}

QVariant
ProvidersModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() || index.column() != 0 ||
        index.row() < 0 || index.row() >= m_providers.count() )
    {
        return QVariant();
    }
    ProviderPtr provider = m_providers.at( index.row() );
    switch( role )
    {
        case Qt::DisplayRole:
            if( provider->description().isEmpty() )
                return provider->prettyName();
            return i18nc( "%1: name, %2: description", "%1 (%2)", provider->prettyName(),
                          provider->description() );
        case Qt::DecorationRole:
            return provider->icon();
        case Qt::ToolTipRole:
            return i18n( "Can match tracks by: %1\nCan synchronize: %2",
                         fieldsToString( provider->reliableTrackMetaData() ),
                         fieldsToString( provider->writableTrackStatsData() ) );
    }
    return QVariant();
}

int
ProvidersModel::rowCount( const QModelIndex &parent ) const
{
    return parent.isValid() ? 0 : m_providers.count();
}

ProviderPtrList
ProvidersModel::selectedProviders() const
{
    ProviderPtrList ret;
    // preserve order, so do it the hard way
    for( int i = 0; i < rowCount(); i++ )
    {
        QModelIndex idx = index( i, 0 );
        if( m_selectionModel->isSelected( idx ) )
            ret << m_providers.at( i );
    }
    return ret;
}

qint64
ProvidersModel::reliableTrackMetadataIntersection() const
{
    if( selectedProviders().isEmpty() )
        return 0;
    QListIterator<ProviderPtr> it( selectedProviders() );
    qint64 fields = it.next()->reliableTrackMetaData();
    while( it.hasNext() )
        fields &= it.next()->reliableTrackMetaData();
    return fields;
}

qint64
ProvidersModel::writableTrackStatsDataUnion() const
{
    qint64 fields = 0;
    for( const ProviderPtr &provider : selectedProviders() )
    {
        fields |= provider->writableTrackStatsData();
    }
    return fields;
}

QItemSelectionModel *
ProvidersModel::selectionModel() const
{
    return m_selectionModel;
}

QString
ProvidersModel::fieldsToString( qint64 fields ) const
{
    QStringList fieldNames;
    for( qint64 i = 0; i < 64; i++ )
    {
        qint64 field = 1LL << i;
        if( !( field & fields ) )
            continue;
        QString name = Meta::i18nForField( field );
        if( !name.isEmpty() )
            fieldNames << name;
    }
    return fieldNames.join( i18nc( "comma between list words", ", " ) );
}

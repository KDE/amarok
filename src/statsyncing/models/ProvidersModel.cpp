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
                                const ProviderPtrSet &checkedProviders, QObject *parent )
    : QAbstractListModel( parent )
    , m_providers( providers )
    , m_checkedProviders( providers.toSet() & checkedProviders )
    , m_selectionModel( new QItemSelectionModel( this, this ) )
{
    // selection defaults to model's tick state
    for( int i = 0; i < rowCount(); i++ )
    {
        QModelIndex idx = index( i, 0 );
        Qt::CheckState state = Qt::CheckState( data( idx, Qt::CheckStateRole ).toInt() );
        if( state == Qt::Checked )
            m_selectionModel->select( idx, QItemSelectionModel::Select );
    }
    connect( m_selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             SIGNAL(selectedProvidersChanged()) );
}

QVariant
ProvidersModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() || index.column() != 0 ||
        index.row() < 0 || index.row() >= m_providers.count() )
    {
        return QVariant();
    }
    QSharedPointer<Provider> provider = m_providers.at( index.row() );
    switch( role )
    {
        case Qt::DisplayRole:
            return provider->prettyName();
        case Qt::DecorationRole:
            return provider->icon();
        case Qt::ToolTipRole:
            return i18n( "Can match tracks by: %1\nCan synchronize: %2",
                         fieldsToString( provider->reliableTrackMetaData() ),
                         fieldsToString( provider->writableTrackStatsData() ) );
        case Qt::CheckStateRole:
            return m_checkedProviders.contains( provider ) ? Qt::Checked : Qt::Unchecked;
    }
    return QVariant();
}

int
ProvidersModel::rowCount( const QModelIndex &parent ) const
{
    return parent.isValid() ? 0 : m_providers.count();
}

Qt::ItemFlags
ProvidersModel::flags( const QModelIndex &index ) const
{
    Q_UNUSED( index )
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

bool
ProvidersModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    if( !index.isValid() || index.column() != 0 ||
        index.row() < 0 || index.row() >= m_providers.count() ||
        role != Qt::CheckStateRole || !value.isValid() )
    {
        return false;
    }
    Qt::CheckState state = Qt::CheckState( value.toInt() );
    QSharedPointer<Provider> provider = m_providers.at( index.row() );
    if( state == Qt::Checked )
        m_checkedProviders.insert( provider );
    else
        m_checkedProviders.remove( provider );
    emit dataChanged( index, index );
    return true;
}

ProviderPtrList
ProvidersModel::checkedProviders() const
{
    ProviderPtrList ret;
    // preserve order, so do it the hard way
    foreach( QSharedPointer<Provider> provider, m_providers )
    {
        if( m_checkedProviders.contains( provider ) )
            ret << provider;
    }
    return ret;
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
    QListIterator<QSharedPointer<Provider> > it( selectedProviders() );
    qint64 fields = it.next()->reliableTrackMetaData();
    while( it.hasNext() )
        fields &= it.next()->reliableTrackMetaData();
    return fields;
}

qint64
ProvidersModel::writableTrackStatsDataIntersection() const
{
    QMap<qint64, int> map; // field to count map
    foreach( QSharedPointer<Provider> provider, selectedProviders() )
    {
        qint64 providerFields = provider->writableTrackStatsData();
        for( qint64 i = 0; i < 64; i++ )
        {
            qint64 field = 1LL << i;
            if( !( field & providerFields ) )
                continue;
            map[ field ]++;
        }
    }

    // map is ready, now take at-least-2 intersection of it.
    qint64 ret = 0;
    QMapIterator<qint64, int> it( map );
    while( it.hasNext() )
    {
        it.next();
        if( it.value() >= 2 )
            ret |= it.key();
    }
    return ret;
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

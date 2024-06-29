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

#include "Config.h"

#include "MetaValues.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Amarok.h"

#include <KConfigGroup>
#include <QIcon>
#include <KLocalizedString>

#include <QBrush>
#include <QPalette>

namespace StatSyncing
{
    struct ProviderData {
        ProviderData( const QString &id_, const QString &name_, const QIcon &icon_, bool online_, bool enabled_ )
            : id( id_ ), name( name_ ), icon( icon_ ), online( online_ ), enabled( enabled_ )
        {}

        QString id;
        QString name;
        QIcon icon;
        bool online;
        bool enabled;
    };
}

using namespace StatSyncing;

Config::Config( QObject *parent )
    : QAbstractListModel( parent )
    , m_checkedFields( 0 )
    , m_hasChanged( false )
{
    read();
}

Config::~Config()
{
}

int
Config::rowCount( const QModelIndex &parent ) const
{
    return parent.isValid() ? 0 : m_providerData.count();
}

QVariant
Config::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();
    if( index.row() < 0 || index.row() >= m_providerData.count() )
        return QVariant();
    if( index.column() != 0 )
        return QVariant();

    const ProviderData &provider = m_providerData.at( index.row() );
    switch( role )
    {
        case Qt::DisplayRole:
            return provider.name;
        case Qt::DecorationRole:
        {
            if( !provider.icon.isNull() )
                return provider.icon;
            return QIcon::fromTheme( provider.online ? QStringLiteral("image-missing") : QStringLiteral("network-disconnect") );
        }
        case Qt::CheckStateRole:
            return provider.enabled ? Qt::Checked : Qt::Unchecked;
        case ProviderIdRole:
            return provider.id;
        case Qt::ForegroundRole:
        {
            // we need to do this trick, because not having ItemIsEnabled in flags() has
            // unwanded side-effects
            QBrush brush;
            QPalette::ColorGroup group = provider.online ? QPalette::Active : QPalette::Disabled;
            brush.setColor( QPalette().color( group, QPalette::Text ) );
            return brush;
        }
        case Qt::ToolTipRole:
            return provider.online ? QString() : i18n( "This collection is currently offline" );
    }
    return QVariant();
}

bool
Config::setData( const QModelIndex &index, const QVariant &value, int role )
{
    if( !index.isValid() )
        return false;
    if( index.row() < 0 || index.row() >= m_providerData.count() )
        return false;
    if( index.column() != 0 )
        return false;
    if( role != Qt::CheckStateRole )
        return false;

    Qt::CheckState state = Qt::CheckState( value.toInt() );
    m_providerData[ index.row() ].enabled = ( state == Qt::Checked ) ? true : false;
    m_hasChanged = true;
    Q_EMIT dataChanged( index, index );
    return true;
}

Qt::ItemFlags
Config::flags( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return Qt::ItemFlags();
    if( index.row() < 0 || index.row() >= m_providerData.count() )
        return Qt::ItemFlags();
    if( index.column() != 0 )
        return Qt::ItemFlags();

    Qt::ItemFlags flags = Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
    if( !m_providerData.at( index.row() ).online )
        flags |= Qt::ItemIsSelectable;
    return flags;
}

void
Config::updateProvider( const QString &id, const QString &name, const QIcon &icon,
                        bool online, bool enabled )
{
    ProviderData providerData( id, name, icon, online, enabled );
    for( int i = 0; i < m_providerData.count(); i++ )
    {
        if( m_providerData.at( i ).id == id )
        {
            m_providerData[ i ] = providerData;
            m_hasChanged = true;
            Q_EMIT dataChanged( index( i ), index( i ) );
            return;
        }
    }
    beginInsertRows( QModelIndex(), m_providerData.count(), m_providerData.count() );
    m_providerData << providerData;
    m_hasChanged = true;
    endInsertRows();
}

void
Config::updateProvider( const QString &id, const QString &name, const QIcon &icon,
                        bool online )
{
    updateProvider( id, name, icon, online, providerEnabled( id, false ) );
}

bool
Config::forgetProvider( const QString &id )
{
    QMutableListIterator<ProviderData> it( m_providerData );
    int i = 0;
    while( it.hasNext() )
    {
        if( it.next().id == id )
        {
            if( it.value().online )
                continue; // refuse to forget online provider
            beginRemoveRows( QModelIndex(), i, i );
            it.remove();
            m_hasChanged = true;
            endRemoveRows();
            Q_EMIT providerForgotten( id );
            return true;
        }
        i++;
    }
    return false;
}

bool
Config::providerKnown( const QString &id ) const
{
    for( const ProviderData &data : m_providerData )
    {
        if( data.id == id )
            return true;
    }
    return false;
}

bool
Config::providerEnabled( const QString &id, bool aDefault ) const
{
    for( const ProviderData &data : m_providerData )
    {
        if( data.id == id )
            return data.enabled;
    }
    return aDefault;
}

bool
Config::providerOnline( const QString &id, bool aDefault ) const
{
    for( const ProviderData &data : m_providerData )
    {
        if( data.id == id )
            return data.online;
    }
    return aDefault;
}

QIcon
Config::providerIcon( const QString &id ) const
{
    for( const ProviderData &data : m_providerData )
    {
        if( data.id == id )
            return data.icon;
    }
    return QIcon();
}

qint64
Config::checkedFields() const
{
    return m_checkedFields;
}

void
Config::setCheckedFields( qint64 fields )
{
    m_checkedFields = fields;
    m_hasChanged = true;
}

QSet<QString>
Config::excludedLabels() const
{
    return m_excludedLabels;
}

void
Config::setExcludedLabels( const QSet<QString> &labels )
{
    m_excludedLabels = labels;
    m_hasChanged = true;
}

bool
Config::hasChanged() const
{
    return m_hasChanged;
}

void
Config::read()
{
    KConfigGroup group = Amarok::config( QStringLiteral("StatSyncing") );

    QStringList providerIds = group.readEntry( "providerIds", QStringList() );
    QStringList providerNames = group.readEntry( "providerNames", QStringList() );
    QList<bool> providerEnabledStatuses = group.readEntry( "providerEnabledStatuses", QList<bool>() );
    int count = qMin( providerIds.count(), providerNames.count() );
    count = qMin( count, providerEnabledStatuses.count() );

    beginResetModel();
    QList<ProviderData> newData;
    for( int i = 0; i < count; i++ )
    {
        QString id = providerIds.at( i );
        newData << ProviderData( id, providerNames.at( i ), providerIcon( id ),
                                 providerOnline( id ), providerEnabledStatuses.at( i ) );
    }
    m_providerData = newData;
    endResetModel();

    m_checkedFields = 0;
    QStringList fieldNames = group.readEntry( "checkedFields", QStringList( QStringLiteral("FIRST") ) );
    if( fieldNames == QStringList( QStringLiteral("FIRST") ) )
        m_checkedFields = Meta::valRating | Meta::valFirstPlayed | Meta::valLastPlayed |
                          Meta::valPlaycount | Meta::valLabel;
    else
    {
        for( const QString &fieldName : fieldNames )
            m_checkedFields |= Meta::fieldForName( fieldName );
    }

    QStringList list = group.readEntry( "excludedLabels", QStringList() );
    QSet<QString> addEntrySet(list.begin(), list.end());
    m_excludedLabels += addEntrySet;

    m_hasChanged = false;
}

void
Config::save()
{
    QStringList providerIds;
    QStringList providerNames;
    QList<bool> providerEnabledStatuses;
    for( const ProviderData &data : m_providerData )
    {
        providerIds << data.id;
        providerNames << data.name;
        providerEnabledStatuses << data.enabled;
    }

    KConfigGroup group = Amarok::config( QStringLiteral("StatSyncing") );
    group.writeEntry( "providerIds", providerIds );
    group.writeEntry( "providerNames", providerNames );
    group.writeEntry( "providerEnabledStatuses", providerEnabledStatuses );

    // prefer string representation for fwd compatibility and user-readability
    QStringList fieldNames;
    for( qint64 i = 0; i < 64; i++ )
    {
        qint64 field = 1LL << i;
        if( field & m_checkedFields )
            fieldNames << Meta::nameForField( field );
    }
    group.writeEntry( "checkedFields", fieldNames );

    group.writeEntry( "excludedLabels", m_excludedLabels.values() );

    group.sync();
    m_hasChanged = false;
}

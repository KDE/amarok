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

#ifndef STATSYNCING_CONFIG_H
#define STATSYNCING_CONFIG_H

#include <QModelIndex>
#include <QSet>

class QIcon;

namespace StatSyncing
{
    struct ProviderData;

    /**
     * Class holding configuration for statistics synchronization, mainly a list of known
     * and enabled providers.
     */
    class Config : public QAbstractListModel
    {
        Q_OBJECT

        public:
            enum {
                ProviderIdRole = Qt::UserRole
            };
            virtual ~Config();

            // QAbstractListModel methods:
            virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;
            virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
            virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );
            virtual Qt::ItemFlags flags( const QModelIndex &index ) const;

            // own methods:
            /**
             * Register a new provider or tell Config that provider has become
             * online/offline.
             */
            void updateProvider( const QString &id, const QString &name, const QIcon &icon,
                                 bool online, bool enabled );
            // convenience overload
            void updateProvider( const QString &id, const QString &name, const QIcon &icon,
                                 bool online );

            /**
             * @return true if successfully removed, false if it didn't exist in first
             * place or vas rejected because it is online.
             */
            bool forgetProvider( const QString &id );

            /**
             * @return true if provider with id @param id was already registered sometime
             * in the past (and not forgotten).
             */
            bool providerKnown( const QString &id ) const;

            /**
             * @return true if provider with id @param id is enabled. Returns
             * @param aDefault when such provider is not known.
             */
            bool providerEnabled( const QString &id, bool aDefault = false ) const;

            /**
             * @return true if provider with id @param id is online. Returns
             * @param aDefault when such provider is not known.
             */
            bool providerOnline( const QString &id, bool aDefault = false ) const;

            QIcon providerIcon( const QString &id ) const;

            /**
             * Binary OR of Meta::val* fields that should be synchronized in automatic
             * synchronization.
             */
            qint64 checkedFields() const;

            /**
             * Set binary OR of Meta::val* fields to synchronize.
             */
            void setCheckedFields( qint64 fields );

            /**
             * Get a list of labels that are black-listed from synchronization (not
             * touched at all).
             */
            QSet<QString> excludedLabels() const;

            /**
             * Set which labels to exclude from synchronization.
             */
            void setExcludedLabels( const QSet<QString> &labels );

            /**
             * Return true if configuration has changed since last saving or reading data.
             */
            bool hasChanged() const;

            /**
             * Reads config from disk. Discards any possible unsaved changes.
             */
            void read();

            /**
             * Saves the config back to disk.
             */
            void save();

        signals:
            void providerForgotten( const QString &id );

        private:
            friend class Controller;

            // Only StatSyncing::Controller can construct config
            Config( QObject *parent = 0 );

            QList<ProviderData> m_providerData;
            qint64 m_checkedFields;
            QSet<QString> m_excludedLabels;
            bool m_hasChanged;
    };
}

#endif // STATSYNCING_CONFIG_H

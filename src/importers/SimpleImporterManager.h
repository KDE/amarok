/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#ifndef STATSYNCING_SIMPLE_IMPORTER_MANAGER_H
#define STATSYNCING_SIMPLE_IMPORTER_MANAGER_H

#include "ImporterManager.h"

/**
  * This macro can be used to reduce the amount of code needed in order to implement
  * ImporterManager subclass and export it as a plugin. If your manager doesn't do
  * anything other than give static info, you can replace the whole class with this macro.
  * See iTunes importer for usage example (ITunesManager.cpp file).
  */
#define AMAROK_EXPORT_SIMPLE_IMPORTER_PLUGIN( libname, TYPE, PRETTY_NAME, DESCRIPTION, \
                                              ICON, ConfigWidget_T, ImporterProvider_T ) \
    class libname ## ImporterManager : public StatSyncing::ImporterManager \
    { \
    public: \
        libname ## ImporterManager( QObject *parent, const QVariantList &args ) \
            : StatSyncing::ImporterManager( parent, args ) \
        { \
        } \
    \
        ~ libname ## ImporterManager() \
        { \
        } \
    \
        QString type() const \
        { \
            return TYPE; \
        } \
    \
        QString prettyName() const \
        { \
            return PRETTY_NAME; \
        } \
    \
        QString description() const \
        { \
            return DESCRIPTION; \
        } \
    \
        QIcon icon() const \
        { \
            return ICON; \
        } \
    \
        StatSyncing::ProviderConfigWidget *configWidget( const QVariantMap &config ) \
        { \
            return new ConfigWidget_T( config ); \
        } \
    \
    protected: \
        KPluginInfo pluginInfo() const \
        { \
            return KPluginInfo( "amarok_importer-" #libname ".desktop", "services" ); \
        } \
    \
        StatSyncing::ImporterProviderPtr newInstance( const QVariantMap &config ) \
        { \
            return StatSyncing::ImporterProviderPtr( \
                                               new ImporterProvider_T( config, this ) ); \
        } \
    }; \
    \
    AMAROK_EXPORT_IMPORTER_PLUGIN( libname, libname ## ImporterManager )

#endif // STATSYNCING_SIMPLE_IMPORTER_MANAGER_H

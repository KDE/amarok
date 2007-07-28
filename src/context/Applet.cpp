/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "Applet.h"

#include <KService>
#include <KServiceTypeTrader>

namespace Context
{
/*
Applet::Applet( QObject* parent, const QStringList& args)
    : Plasma::Applet( parent, args )
{}


KConfigGroup Applet::globalAppletConfig() const
{
    if ( !d->globalConfig ) {
        QString file = KStandardDirs::locateLocal( "config", "amarokapplet_" + globalName() + "rc" );
        d->globalConfig = KSharedConfig::openConfig( file );
    }

    return KConfigGroup(d->globalConfig, "General");

} 

KPluginInfo::List Applet::knownApplets()
{
    QHash<QString, KPluginInfo> applets;
    KService::List offers = KServiceTypeTrader::self()->query("AmarokContext/Applet");
    return KPluginInfo::fromServices(offers);
}

Applet* Applet::loadApplet(const QString& appletName, uint appletId, const QStringList& args)
{
    if (appletName.isEmpty()) {
        return 0;
    }
    
    QString constraint = QString("[X-KDE-PluginInfo-Name] == '%1'").arg(appletName);
    KService::List offers = KServiceTypeTrader::self()->query("AmarokContext/Applet", constraint);
    
    if (offers.isEmpty()) {
        //TODO: what would be -really- cool is offer to try and download the applet
        //      from the network at this point
        kDebug() << "Applet::loadApplet: offers is empty for \"" << appletName << "\"" << endl;
        return 0;
    }
    
    if (appletId == 0) {
        appletId = Private::nextId();
    }
    
    QStringList allArgs;
    QString id;
    id.setNum(appletId);
    allArgs << offers.first()->storageId() << id << args;
    Applet* applet = KService::createInstance<Context::Applet>(offers.first(), 0, allArgs);
    
    if (!applet) {
        kDebug() << "Couldn't load applet \"" << appletName << "\"!" << endl;
    }
    
    return applet;
}
*/
} // Context namespace

#include "Applet.moc"

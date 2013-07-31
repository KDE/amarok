/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
 * The Amazon store in based upon the Magnatune store in Amarok,                        *
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#include "AmazonConfig.h"

#include <QMutex>

#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>

AmazonConfig* AmazonConfig::m_instance = 0;

AmazonConfig* AmazonConfig::instance()
{
    QMutex mutex;
    mutex.lock();

    if( !m_instance )
       m_instance = new AmazonConfig();

    mutex.unlock();
    return m_instance;
}

void
AmazonConfig::destroy()
{
    QMutex mutex;
    mutex.lock();

    if( m_instance )
    {
        delete m_instance;
        m_instance = 0;
    }

    mutex.unlock();
}

AmazonConfig::AmazonConfig()
{
    load();
}

AmazonConfig::~AmazonConfig()
{
}

QString
AmazonConfig::country() const
{
    // TODO/HACK: due to some reason the local value is not always up2date, so let's reload it every time
    // reason: we use this class in another lib, the KCM module, too. so there are two instances, actually...
    KConfigGroup config = KGlobal::config()->group( "Service_Amazon" );
    return config.readEntry( "country", QString() );
    // return m_country;
}

void
AmazonConfig::setCountry( QString country )
{
    m_country = country;
    save();
}

void
AmazonConfig::load()
{
    KConfigGroup config = KGlobal::config()->group( "Service_Amazon" );
    m_country = config.readEntry( "country", QString() );
}

void
AmazonConfig::save()
{
    KConfigGroup config = KGlobal::config()->group( "Service_Amazon" );
    config.writeEntry( "country", m_country );
    config.sync();
}

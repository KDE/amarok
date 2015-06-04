/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
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
 
#ifndef AMAZONURLRUNNER_H
#define AMAZONURLRUNNER_H

#include "amarokurls/AmarokUrlRunnerBase.h"

#include <QIcon>

#include <QObject>

/**
@author Nikolaj Hald Nielsen <nhn@kde.org>
@author Sven Krohlas <sven@asbest-online.de>

We support URLs like
amarok://service-amazonstore?asin=B004UQSB8I&command=addToCart&name=The%20Cure%20-%20Disintegration%20(Remastered)&price=989
to add something to the shopping cart and
amarok://navigate/MP3%20Music%20Store/?filter=something_to_search_for
to search in the mp3 database of the store.
*/
class AmazonUrlRunner : public QObject, public AmarokUrlRunnerBase
{
    Q_OBJECT
public:
    AmazonUrlRunner();

    virtual ~AmazonUrlRunner();

    virtual QString command() const;
    virtual QString prettyCommand() const;
    virtual QIcon icon() const;
    virtual bool run( AmarokUrl url );

Q_SIGNALS:
    void search( const QString &request );
};

#endif // AMAZONURLRUNNER_H

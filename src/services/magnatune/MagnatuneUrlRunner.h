/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef MAGNATUNEURLRUNNER_H
#define MAGNATUNEURLRUNNER_H

#include "amarokurls/AmarokUrlRunnerBase.h"

#include <QIcon>

#include <QObject>

/**
	@author Nikolaj Hald Nielsen <nhn@kde.org> 
*/
class MagnatuneUrlRunner : public QObject, public AmarokUrlRunnerBase
{
    Q_OBJECT
public:
    MagnatuneUrlRunner();

    virtual ~MagnatuneUrlRunner();

    QString command() const override;
    QString prettyCommand() const override;
    QIcon icon() const override;
    bool run( const AmarokUrl &url ) override;

Q_SIGNALS:
    void showFavorites();
    void showHome();
    void showRecommendations();
    void buyOrDownload( const QString &sku );
    void removeFromFavorites( const QString &sku );
};

#endif

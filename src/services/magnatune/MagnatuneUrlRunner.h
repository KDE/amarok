/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef MAGNATUNEURLRUNNER_H
#define MAGNATUNEURLRUNNER_H

#include "amarokurls/AmarokUrlRunnerBase.h"

#include <KIcon>

#include <QObject>

/**
	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 
*/
class MagnatuneUrlRunner : public QObject, public AmarokUrlRunnerBase
{
    Q_OBJECT
public:
    MagnatuneUrlRunner();

    virtual ~MagnatuneUrlRunner();

    virtual QString command() const;
    virtual KIcon icon() const;
    virtual bool run( AmarokUrl url );

signals:
    void showFavorites();
    void showHome();
    void showRecommendations();
    void buyOrDownload( const QString &sku );
    void removeFromFavorites( const QString &sku );
};

#endif

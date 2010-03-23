/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef PLAYURLGENERATOR_H
#define PLAYURLGENERATOR_H

#include "amarok_export.h"
#include "AmarokUrlGenerator.h"
#include "core/meta/Meta.h"

#include <QString>

class AmarokUrl;

/**
 * A class used to generate play urls.
 *
 * The format of a 'play' amarokurl is:
 * amarokurl://play/<Base 64 Encoded playableUrl() of the track>/<integer seconds>
 */
class AMAROK_EXPORT PlayUrlGenerator : public AmarokUrlGenerator
{
public:

    static PlayUrlGenerator * instance();

    AmarokUrl createCurrentTrackBookmark();
    AmarokUrl createTrackBookmark( Meta::TrackPtr track, qint64 miliseconds, QString name = QString() );

    QString description();
    KIcon icon();
    AmarokUrl createUrl();

private:
    PlayUrlGenerator();
    ~PlayUrlGenerator();

    static PlayUrlGenerator * s_instance;
};

#endif // PLAYURLGENERATOR_H

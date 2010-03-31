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

#ifndef PLAYURLRUNNER_H
#define PLAYURLRUNNER_H

#include "amarok_export.h"
#include "AmarokUrlRunnerBase.h"

#include <QList>
/**
 * The Runner that handles urls that plays tracks.
 * @author Casey Link <unnamedrambler@gmail.com>
 */
class AMAROK_EXPORT PlayUrlRunner : public AmarokUrlRunnerBase
{
public:
    PlayUrlRunner ();

    virtual  ~PlayUrlRunner ();

    virtual QString command () const;
    virtual QString prettyCommand() const;
    virtual bool run ( AmarokUrl url );
    virtual KIcon icon () const;

    /**
     * This function takes a url for a track, and returns a list
     * of bookmarks (represented by play amarokurls) for the track.
     * The definition of a play amarokurl is in PlayUrlGenerator
     *
     * @param url the playableUrl() of a Meta::Track
     * @return a list of bookmarks. the list is empty if no bookmarks exist.
     * @see PlayUrlGenerator
     */
    static BookmarkList bookmarksFromUrl( KUrl url );

};

#endif // PLAYURLRUNNER_H

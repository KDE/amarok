/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#ifndef GLOBALCOLLECTIONACTIONS_H
#define GLOBALCOLLECTIONACTIONS_H


#include "amarok_export.h"
#include "Meta.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"





class AMAROK_EXPORT GlobalCollectionAction : public PopupDropperAction
{
public:
    GlobalCollectionAction( const QString &text, QObject * parent );
};


class AMAROK_EXPORT GlobalCollectionGenreAction : public GlobalCollectionAction
{
    public:

        GlobalCollectionGenreAction( const QString &text, QObject * parent );
        void setGenre( Meta::GenrePtr genre );

    protected:
        Meta::GenrePtr genre();

    private:
        Meta::GenrePtr m_currentGenre;

};

class AMAROK_EXPORT GlobalCollectionArtistAction : public GlobalCollectionAction
{
    public:

        GlobalCollectionArtistAction( const QString &text, QObject * parent );
        void setArtist( Meta::ArtistPtr artist );
        
    protected:
        Meta::ArtistPtr artist();

    private:
        Meta::ArtistPtr m_currentArtist;

};


class AMAROK_EXPORT GlobalCollectionAlbumAction : public GlobalCollectionAction
{
    public:

        GlobalCollectionAlbumAction( const QString &text, QObject * parent );
        void setAlbum( Meta::AlbumPtr album );

    protected:
        Meta::AlbumPtr album();

    private:
        Meta::AlbumPtr m_currentAlbum;

};

class AMAROK_EXPORT GlobalCollectionTrackAction : public GlobalCollectionAction
{
    public:

        GlobalCollectionTrackAction( const QString &text, QObject * parent );
        void setTrack( Meta::TrackPtr track );
        
    protected:
        Meta::TrackPtr track();
        
    private:
        Meta::TrackPtr m_currentTrack;

};

class AMAROK_EXPORT GlobalCollectionYearAction : public GlobalCollectionAction
{
    public:

        GlobalCollectionYearAction( const QString &text, QObject * parent );
        void setYear( Meta::YearPtr year );

    protected:
        Meta::YearPtr year();

    private:
        Meta::YearPtr m_currentYear;

};

class GlobalCollectionComposerAction : public GlobalCollectionAction
{
    public:

        GlobalCollectionComposerAction( const QString &text, QObject * parent );
        void setComposer( Meta::ComposerPtr composer );

    protected:
        Meta::ComposerPtr composer();

    private:
        Meta::ComposerPtr m_currentComposer;

};





class GlobalCollectionActions;

namespace The {
    AMAROK_EXPORT GlobalCollectionActions* globalCollectionActions();
}

/**
This class keeps track of global context actions that should be added to all genre, artists or another meta type in all collections.

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 
*/
class AMAROK_EXPORT GlobalCollectionActions
{

    friend GlobalCollectionActions* The::globalCollectionActions();

public:



    QList<PopupDropperAction *> actionsFor( Meta::DataPtr item );

    void addGenreAction( GlobalCollectionGenreAction * action );
    void addArtistAction( GlobalCollectionArtistAction * action );
    void addAlbumAction( GlobalCollectionAlbumAction * action );
    void addTrackAction( GlobalCollectionTrackAction * action );
    void addYearAction( GlobalCollectionYearAction * action );
    void addComposerAction( GlobalCollectionComposerAction * action );
  

    
private:
    GlobalCollectionActions();
    ~GlobalCollectionActions();

    QList<PopupDropperAction *> actionsFor( Meta::GenrePtr genre );
    QList<PopupDropperAction *> actionsFor( Meta::ArtistPtr artist );
    QList<PopupDropperAction *> actionsFor( Meta::AlbumPtr album );
    QList<PopupDropperAction *> actionsFor( Meta::TrackPtr track );
    QList<PopupDropperAction *> actionsFor( Meta::YearPtr year );
    QList<PopupDropperAction *> actionsFor( Meta::ComposerPtr composer );

    QList<GlobalCollectionGenreAction *> m_genreActions;
    QList<GlobalCollectionArtistAction *> m_artistActions;
    QList<GlobalCollectionAlbumAction *> m_albumActions;
    QList<GlobalCollectionTrackAction *> m_trackActions;
    QList<GlobalCollectionYearAction *> m_yearActions;
    QList<GlobalCollectionComposerAction *> m_composerActions;



    

};

#endif

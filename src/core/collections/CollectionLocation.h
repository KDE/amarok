/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2008 Jason A. Donenfeld <Jason@zx2c4.com>                              *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef AMAROK_COLLECTIONLOCATION_H
#define AMAROK_COLLECTIONLOCATION_H

#include "core/amarokcore_export.h"
#include "core/meta/forward_declarations.h"
#include "core/transcoding/TranscodingConfiguration.h"

#include <QList>
#include <QObject>

#include <QUrl>

namespace Collections {
    class Collection;
    class QueryMaker;

/**
    This base class defines the methods necessary to allow the copying and moving
    of tracks between different collections in a generic way.

    This class should be used as follows in client code:
    - select a source and a destination CollectionLocation
    - call prepareCopy or prepareMove on the source CollectionLocation
    - forget about the rest of the workflow

    Implementations which are writable must reimplement the following methods
    - prettyLocation()
    - isWritable()
    - remove( Meta::Track )
    - copyUrlsToCollection( QMap<Meta::TrackPtr, QUrl> )

    Writable collections that are also organizable should reimplement isOrganizable().
    Organizable means that the user is able to decide (to varying degrees, the details
    depend on the actual collection) where the files are stored in the filesystem (or some
    kind of VFS). An example would be the local collection, where the user can select the directory
    structure that Amarok uses to store the files. An example for a writable collection that is not
    organizable are ipods, where the user has no control about the actual location of the music files
    (they are automatically stored in a not human-readable form).

    Implementations which are only readable can reimplement getKIOCopyableUrls( Meta::TrackList )
    if it is necessary, but can use the default implementations if possible.

    Implementations that have a string expressable location(s), such as a URL or path on disk
    should reimplement actualLocation().

    Implementations that need additional information provided by the user have to implement
    showSourceDialog() and showDestinationDialog(), depending on whether the information is required
    in the source, the destination, or both.

    The methods will be called in the following order:
    startWorkflow (source)
    showSourceDialog (source) (reimplementable)
    slotShowSourceDialogDone (source)
    slotPrepareOperation (destination)
    showDestinationDialog (destination) (reimplementable)
    slotShowDestinationDialogDone (destination)
    slotOperationPrepared (source)
    getKIOCopyableUrls (source) (reimplementable)
    slotGetKIOCopyableUrlsDone (source)
    slotStartCopy (destination)
    copyUrlsToCollection (destination) (reimplementable)
    slotCopyOperationFinished (destination)
    slotFinishCopy (source)

    To provide removal ability, it is required to reimplement removeUrlsFromCollection,
    and this function must call slotRemoveOperationFinished() when it is done.  Optionally,
    showRemoveDialog can be reimplemented to customize the warning displayed before a removal,
    and this function must call slotShowRemoveDialogDone when finished.

    The methods for remove will be called in the following order:
    startRemoveWorkflow
    showRemoveDialog (reimplementable)
    slotShowRemoveDialogDone
    slotStartRemove
    removeUrlsFromCollection (reimplementable)
    slotRemoveOperationFinished
    slotFinishRemove
*/

class AMAROK_CORE_EXPORT CollectionLocation : public QObject
{
    Q_OBJECT

    //testing only do not use these properties in anything but tests
    Q_PROPERTY( bool removeSources
                READ getRemoveSources
                WRITE setRemoveSources
                DESIGNABLE false
                SCRIPTABLE false )

    public:
        CollectionLocation();
        explicit CollectionLocation( Collections::Collection *parentCollection );
        virtual  ~CollectionLocation();

        /**
            Returns a pointer to the collection location's corresponding collection.
            @return a pointer to the collection location's corresponding collection
         */
        virtual Collections::Collection *collection() const;

        /**
            a displayable string representation of the collection location. use the return
            value of this method to display the collection location to the user.
            @return a string representation of the collection location
        */
        virtual QString prettyLocation() const;

        /**
            Returns a list of machine usable strings representingthe collection location.
            For example, a local collection would return a list of paths where tracks are
            stored, while an Ampache collection would return a list with one string
            containing the URL of an ampache server. An iPod collection and a MTP device
            collection are examples of collections that do not need to reimplement this method.
        */
        virtual QStringList actualLocation() const;

        /**
            Returns whether the collection location is writable or not. For example, a
            local collection or an ipod collection would return true, a daap collection
            or a service collection false. The value returned by this method indicates
            if it is possible to copy tracks to the collection, and if it is generally
            possible to remove/delete files from the collection.
            @return @c true if the collection location is writable
            @return @c false if the collection location is not writable
        */
        virtual bool isWritable() const;

        /**
           Returns whether the collection is organizable or not. Organizable collections
           allow move operations where the source and destination collection are the same.
           @return @c true if the collection location is organizable, false otherwise
         */
        virtual bool isOrganizable() const;

        /**
           Convenience method for copying a single track.
           @see prepareCopy( Meta::TrackList, CollectionLocation* )
        */
        void prepareCopy( Meta::TrackPtr track, CollectionLocation *destination );
        /**
           Schedule copying of @param tracks to collection location @param destination.
           This method takes ownership of the @param destination, you may not reference
           or delete it after this call. This method returns immediately and the actual
           copy is performed in the event loop and/or another thread.
        */
        void prepareCopy( const Meta::TrackList &tracks, CollectionLocation *destination );
        /**
           Convenience method for copying tracks based on QueryMaker restults,
           takes ownership of the @param qm.
           @see prepareCopy( Meta::TrackList, CollectionLocation* )
        */
        void prepareCopy( Collections::QueryMaker *qm, CollectionLocation *destination );

        /**
         * Convenience method for moving a single track.
         * @see prepareMove( Meta::TrackList, CollectionLocation* )
         */
        void prepareMove( Meta::TrackPtr track, CollectionLocation *destination );
        /**
           Schedule moving of @param tracks to collection location @param destination.
           This method takes ownership of the @param destination, you may not reference
           or delete it after this call. This method returns immediately and the actual
           move is performed in the event loop and/or another thread.
        */
        void prepareMove( const Meta::TrackList &tracks, CollectionLocation *destination );
        /**
           Convenience method for moving tracks based on QueryMaker restults,
           takes ownership of the @param qm.
           @see prepareMove( Meta::TrackList, CollectionLocation* )
        */
        void prepareMove( Collections::QueryMaker *qm, CollectionLocation *destination );

        /**
           Starts workflow for removing tracks.
         */
        void prepareRemove( const Meta::TrackList &tracks );
        /**
           Convenience method for removing tracks selected by QueryMaker,
           takes ownership of the @param qm.
           @see prepareRemove( Meta::TrackList )
         */
        void prepareRemove( Collections::QueryMaker *qm );

        /**
         * Adds or merges a track to the collection (not to the disk)
         * Inserts a set of TrackPtrs directly into the database without needing to actual move any files
         * This is a hack required by the DatabaseImporter
         * TODO: Remove this hack
         * @return true if the database entry was inserted, false otherwise
         */
        virtual bool insert( const Meta::TrackPtr &track, const QString &path );

        /**
          explicitly inform the source collection of successful transfer.
          The source collection will only remove files (if necessary)
          for which this method was called.
          */
        void transferSuccessful( const Meta::TrackPtr &track );

        /**
        * tells the source location that an error occurred during the transfer of the file
        */
        virtual void transferError( const Meta::TrackPtr &track, const QString &error );

    Q_SIGNALS:
        void startCopy( const QMap<Meta::TrackPtr, QUrl> &sources,
                        const Transcoding::Configuration & );
        void finishCopy();
        void startRemove();
        void finishRemove();
        void prepareOperation( const Meta::TrackList &tracks, bool removeSources,
                               const Transcoding::Configuration & );
        void operationPrepared();
        void aborted();

    protected:
        /**
         * aborts the workflow
         */
        void abort();

        /**
         * allows the destination location to access the source CollectionLocation.
         * note: subclasses do not take ownership  of the pointer
         */
        CollectionLocation* source() const;

        /**
          * allows the source location to access the destination CollectionLocation.
          * Pointer may be null!
          * note: subclasses do not take ownership of the pointer
          */
        CollectionLocation* destination() const;

        /**
            this method is called on the source location, and should return a list of urls
            which the destination location can copy using KIO. You must call
            slotGetKIOCopyableUrlsDone( QMap<Meta::TrackPtr, QUrl> ) after retrieving the
            urls. The order of urls passed to that method has to be the same as the order
            of the tracks passed to this method.
        */
        virtual void getKIOCopyableUrls( const Meta::TrackList &tracks );
        /**
            this method is called on the destination. reimplement it if your implementation
            is writable. You must call slotCopyOperationFinished() when you are done copying
            the files.

            Before calling slotCopyOperationFinished(), you should call
            source()->transferSuccessful() for every source track that was for sure
            successfully copied to destination collection. Only such marked tracks are
            then removed in case of a "move" action.
        */
        virtual void copyUrlsToCollection( const QMap<Meta::TrackPtr, QUrl> &sources,
                                           const Transcoding::Configuration &configuration );

        /**
           this method is called on the collection you want to remove tracks from.  it must
           be reimplemented if your collection is writable and you wish to implement
           removing tracks. You must call slotRemoveOperationFinished() when you are done
           removing the files.

           Before calling slotRemoveOperationFinished(), you should call transferSuccessful()
           for every track that was successfully deleted. CollectionLocation then scans
           directories of such tracks and allows user to remove empty ones.
        */
        virtual void removeUrlsFromCollection( const Meta::TrackList &sources );

        /**
         * this method is called on the source. It allows the source CollectionLocation to
         * show a dialog. Classes that reimplement this method must call
         * slotShowSourceDialogDone() after they have acquired all necessary information from the user.
         *
         * Default implementation calls getDestinationTranscodingConfig() which may ask
         * user. If you reimplement this you may (or not) call this base method instead
         * of calling slotShowDestinationDialogDone() to support transcoding.
         */
        virtual void showSourceDialog( const Meta::TrackList &tracks, bool removeSources );

        /**
         * Get transcoding configuration to use when transferring tracks to destination.
         * If destination collection doesn't support transcoding, JUST_COPY configuration
         * is returned, otherwise preferred configuration is read or user is asked.
         * Returns invalid configuration in case user has hit cancel or closed the dialog.
         *
         * This method is meant to be called by source collection.
         */
        virtual Transcoding::Configuration getDestinationTranscodingConfig();

        /**
         * This method is called on the destination. It allows the destination
         * CollectionLocation to show a dialog. Classes that reimplement this method
         * must call slotShowDestinationDialogDone() after they have acquired all necessary
         * information from the user.
         *
         * Default implementation calls setGoingToRemoveSources( removeSources ) so that
         * isGoingToRemoveSources() is available on destination, too and then calls
         * slotShowDestinationDialogDone()
         */
        virtual void showDestinationDialog( const Meta::TrackList &tracks,
                                            bool removeSources,
                                            const Transcoding::Configuration &configuration );

        /**
         * this methods allows the collection to show a warning dialog before tracks are removed,
         * rather than using the default provided.  Classes that reimplement this method must call
         * slotShowRemoveDialogDone() after they are finished.
        */

        virtual void showRemoveDialog( const Meta::TrackList &tracks );

        /**
         * Get nice localised string describing the current operation based on transcoding
         * configuraiton and isGoingToRemoveSources(); meant to be called by destination
         * collection.
         *
         * @return "Copy Tracks", "Transcode and Organize Tracks" etc.
         */
        QString operationText( const Transcoding::Configuration &configuration );

        /**
         * Get nice localised string that can be used as progress bar text for the current
         * operation; meant to be called by the destination collection.
         *
         * @param trackCount number of tracks in the transfer
         * @param destinationName pretty localised name of the destination collection;
         *        prettyLocation() is used if the string is empty or not specified
         *
         * @return "Transcoding and moving <trackCount> tracks to <destinationName>" etc.
         */
        QString operationInProgressText( const Transcoding::Configuration &configuration,
                                         int trackCount, QString destinationName = QString() );

        /**
        * Sets or gets whether some source files may be removed
        */
        virtual bool isGoingToRemoveSources() const;
        virtual void setGoingToRemoveSources( bool removeSources );

        /**
        * Sets or gets whether to stifle the removal confirmation
        */
        virtual bool isHidingRemoveConfirm() const;
        virtual void setHidingRemoveConfirm( bool hideRemoveConfirm );

    protected Q_SLOTS:
        /**
         * this slot has to be called from getKIOCopyableUrls( Meta::TrackList )
         * Please note: the order of urls in the argument has to be the same as in the
         * tracklist
         */
        void slotGetKIOCopyableUrlsDone( const QMap<Meta::TrackPtr, QUrl> &sources );
        void slotCopyOperationFinished();
        void slotRemoveOperationFinished();
        void slotShowSourceDialogDone();
        void slotShowRemoveDialogDone();
        void slotShowDestinationDialogDone();

    private Q_SLOTS:
        void slotShowSourceDialog();  // trick to show dialog in next mainloop iteration
        void slotPrepareOperation( const Meta::TrackList &tracks, bool removeSources,
                                   const Transcoding::Configuration &configuration );
        void slotOperationPrepared();
        void slotStartCopy( const QMap<Meta::TrackPtr, QUrl> &sources,
                            const Transcoding::Configuration &configuration );
        void slotFinishCopy();
        void slotStartRemove();
        void slotFinishRemove();
        void slotAborted();
        void resultReady( const Meta::TrackList &tracks );
        void queryDone();

    private:
        void setupConnections();
        void setupRemoveConnections();
        void startWorkflow( const Meta::TrackList &tracks, bool removeSources );
        void startRemoveWorkflow( const Meta::TrackList &tracks );
        void removeSourceTracks( const Meta::TrackList &tracks );
        void setSource( CollectionLocation *source );

        //only used in the source CollectionLocation
        CollectionLocation *m_destination;
        //only used in destination CollectionLocation
        CollectionLocation *m_source;

        Meta::TrackList getSourceTracks() const { return m_sourceTracks; }
        void setSourceTracks( Meta::TrackList tracks ) { m_sourceTracks = tracks; }
        Meta::TrackList m_sourceTracks;

        Collections::Collection *m_parentCollection;

        bool getRemoveSources() const { return m_removeSources; }
        void setRemoveSources( bool removeSources ) { m_removeSources = removeSources; }
        bool m_removeSources;
        bool m_isRemoveAction;
        bool m_noRemoveConfirmation;
        Transcoding::Configuration m_transcodingConfiguration;
        //used by the source collection to store the tracks that were successfully
        //copied by the destination and can be removed as part of a move
        Meta::TrackList m_tracksSuccessfullyTransferred;
        QMap<Meta::TrackPtr, QString> m_tracksWithError;
};

} //namespace Collections

#endif

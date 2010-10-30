/****************************************************************************************
 * Copyright (c) 2010 Canonical Ltd                                                     *
 * Author: Aurelien Gateau <aurelien.gateau@canonical.com>                              *
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

#ifndef MPRIS2_DBUS_HANDLER_H
#define MPRIS2_DBUS_HANDLER_H

#include "core/meta/Meta.h"

#include <Phonon/Global>

#include <QObject>
#include <QVariant>

class QDBusObjectPath;

namespace Amarok
{
    /**
     * This class implements the MPRIS 2.0 specification. You can find this
     * specification here:
     * http://www.mpris.org/2.0/spec/
     *
     * For now it only implements the MediaPlayer2 and MediaPlayer2.Player
     * interfaces, not the optional MediaPlayer2.TrackList interface.
     *
     * The implementation is a bit tricky because it makes use of QObject
     * dynamic properties to implement properties whose changes must be
     * announced with the org.freedesktop.DBus.Property.PropertiesChanged()
     * signal:
     *
     * - Properties which do not require emission of the PropertiesChanged()
     *   signal are implemented as static properties, declared with Q_PROPERTY.
     *
     * - Properties which require the PropertiesChanged() signal are
     *   implemented as dynamic properties: they can only be accessed via
     *   the property() method.
     *
     * The advantage of this method is that it is possible to implement a
     * generic way to track property changes: when a property is updated to
     * reflect Amarok internal state, setPropertyInternal() is called. In
     * addition to setting the value of the property, setPropertyInternal()
     * checks whether the property really changed and adds it to
     * m_changedProperties if it has.
     */
    class Mpris2DBusHandler : public QObject
    {
        Q_OBJECT
        /**
         * @defgroup org.mpris.MediaPlayer2 MPRIS 2.0 Root interface
         * @{
         */
        Q_PROPERTY( bool CanQuit READ CanQuit )
        Q_PROPERTY( bool CanRaise READ CanRaise )
        Q_PROPERTY( bool HasTrackList READ HasTrackList )
        Q_PROPERTY( QString Identity READ Identity )
        Q_PROPERTY( QString DesktopEntry READ DesktopEntry )
        Q_PROPERTY( QStringList SupportedUriSchemes READ SupportedUriSchemes )
        Q_PROPERTY( QStringList SupportedMimeTypes READ SupportedMimeTypes )
        /* @} */

        /**
         * @defgroup org.mpris.MediaPlayer2.Player MPRIS 2.0 Player interface
         * @{
         */
        Q_PROPERTY( double Rate READ Rate WRITE SetRate )
        Q_PROPERTY( qlonglong Position READ Position )
        Q_PROPERTY( double MinimumRate READ MinimumRate )
        Q_PROPERTY( double MaximumRate READ MaximumRate )
        Q_PROPERTY( bool CanControl READ CanControl )
        /* @} */

    public:
        Mpris2DBusHandler();

        void setProperty( const char *name, const QVariant &value );

        /**
         * @addtogroup org.mpris.MediaPlayer2
         * @{
         */
        void Raise();
        void Quit();

        bool CanQuit() const
        {
            return true;
        }

        bool CanRaise() const
        {
            return true;
        }

        bool HasTrackList() const
        {
            return false;
        }

        QString Identity() const;
        QString DesktopEntry() const;
        QStringList SupportedUriSchemes() const;
        QStringList SupportedMimeTypes() const;
        /* @} */

        /**
         * @addtogroup org.mpris.MediaPlayer2.Player
         * @{
         */
        void Next();
        void Previous();
        void Pause();
        void PlayPause();
        void Stop();
        void Play();
        void Seek( qlonglong offset );
        void SetPosition( const QDBusObjectPath &trackId, qlonglong offset );
        void OpenUri( const QString &uri );

        // NB: Amarok extensions, not part of the mpris spec
        void StopAfterCurrent();
        void AdjustVolume( double increaseBy );
        void SetMuted( bool );
        void ShowOSD() const;
        void LoadThemeFile( const QString &path ) const;

        // Properties
        void SetLoopStatus( const QString & );

        int Rate() const
        {
            return 1.0;
        }

        void SetRate( double ) {}

        void SetShuffle( bool );

        double Volume() const;
        void SetVolume( double );

        qlonglong Position() const;

        double MinimumRate() const
        {
            return 1.0;
        }

        double MaximumRate() const
        {
            return 1.0;
        }

        bool CanControl() const
        {
            return true;
        }

    Q_SIGNALS:
        void Seeked( qlonglong );
        /* @} */


    private:
        QList<QByteArray> m_changedProperties;

        void setPropertyInternal( const char *name, const QVariant &value );

    private Q_SLOTS:
        void trackPositionChanged( qint64 position, bool userSeek );
        void seekableChanged( bool );
        void volumeChanged( int percent );

        void updateTrackProgressionProperties();
        void updatePlaybackStatusProperty();
        void updatePlaylistProperties();
        void updateTrackProperties();
        void emitPropertiesChanged();
    };
}

#endif

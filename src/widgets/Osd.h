/****************************************************************************************
 * Copyright (c) 2003 Christian Muehlhaeuser <chris@chris.de>                           *
 * Copyright (c) 2008,2009 Mark Kretschmann <kretschmann@kde.org>                       *
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

#ifndef AMAROK_OSD_H
#define AMAROK_OSD_H

#include "core/meta/Meta.h"

#include <QImage>
#include <QPixmap>
#include <QString>
#include <QWidget> //baseclass

#define OSD_WINDOW_OPACITY 0.74


class OSDWidget : public QWidget
{
    Q_OBJECT

    public:
        enum Alignment { Left, Middle, Center, Right };

        /** resets the colours to defaults */
        void unsetColors();

        /** shadow size in every direction */
        static const int SHADOW_SIZE = 5;

    public slots:
        /** calls setText() then show(), after setting image if needed */
        void show( const QString &text, const QImage &newImage = QImage() );
        void ratingChanged( const short rating );
        void ratingChanged( const QString& path, int rating );
        void volumeChanged( int volume );

        /** reimplemented, shows the OSD */
        virtual void show();
        virtual void setVisible( bool visible );

        /**
         * For the sake of simplicity, when these settings are
         * changed they do not take effect until the next time
         * the OSD is shown!
         *
         * To force an update call show();
         */
        void setDuration( int ms ) { m_duration = ms; }
        void setTextColor( const QColor &color );

        inline int offset() const { return m_y; }
        void setOffset( int y ) { m_y = y; }

        inline int alignment() const { return m_alignment; }
        void setAlignment( Alignment alignment ) { m_alignment = alignment; }

        inline int screen() const { return m_screen; }
        void setScreen( int screen );

        void setPaused( bool paused ) { m_paused = paused; }
        void setImage( const QImage &image ) { m_cover = image; }
        void setText( const QString &text ) { m_text = text; }
        void setRating( const short rating ) { m_rating = rating; }
        void setTranslucent( bool enabled ) { setWindowOpacity( enabled ? OSD_WINDOW_OPACITY : 1.0 ); }
        void setFontScale( int scale );
        void setHideWhenFullscreenWindowIsActive( bool hide );

    protected:
        explicit OSDWidget( QWidget *parent, const char *name = "osd" );
        virtual ~OSDWidget();

        // work-around to get default point size on this platform, Qt API does not offer this directly
        inline qreal defaultPointSize() const { return QFont(font().family()).pointSizeF(); }

        /** determine new size and position */
        QRect determineMetrics( const int marginMetric );

        /**
         * @short Checks if the OSD is temporary disabled.
         * This is usually the case if the OSD should not be shown
         * if a fullscreen application is active (@see m_hideWhenFullscreenWindowIsActive)
         * (where the OSD could steal focus).
         */
        bool isTemporaryDisabled();

        // Reimplemented from QWidget
        virtual void paintEvent( QPaintEvent* );
        virtual void mousePressEvent( QMouseEvent* );
        virtual void resizeEvent( QResizeEvent *e );
        virtual bool event( QEvent* );

        /** distance from screen edge */
        static const int MARGIN = 15;

    private:
        uint        m_m;
        QSize       m_size;
        int         m_duration;
        QTimer     *m_timer;
        Alignment   m_alignment;
        int         m_screen;
        uint        m_y;
        short       m_rating;
        int         m_volume;
        bool        m_showVolume;
        QString     m_text;
        QImage      m_cover;
        QPixmap     m_scaledCover;
        bool        m_paused;
        bool        m_hideWhenFullscreenWindowIsActive;
};


class OSDPreviewWidget : public OSDWidget
{
    Q_OBJECT

public:
    OSDPreviewWidget( QWidget *parent );

public slots:
    void setTextColor( const QColor &color ) { OSDWidget::setTextColor( color ); doUpdate(); }
    //void setFont( const QFont &font ) { OSDWidget::setFont( font ); doUpdate(); }
    void setScreen( int screen ) { OSDWidget::setScreen( screen ); doUpdate(); }
    void setFontScale( int scale ) { OSDWidget::setFontScale( scale ); doUpdate(); }
    void setTranslucent( bool enabled ) { OSDWidget::setTranslucent( enabled );; doUpdate(); }

    void setUseCustomColors( const bool use, const QColor &fg );

private:
    inline void doUpdate() { if( !isHidden() ) show(); }

signals:
    void positionChanged();

protected:
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void mouseMoveEvent( QMouseEvent * );

private:
    bool   m_dragging;
    QPoint m_dragOffset;
};



namespace Amarok
{
    class OSD : public OSDWidget
    {
        Q_OBJECT

    public:
        static OSD* instance();
        static void destroy();

        void applySettings();
        virtual void show( Meta::TrackPtr track );

        // Don't hide baseclass methods - prevent compiler warnings
        virtual void show() { OSDWidget::show(); }

    protected slots:
        void muteStateChanged( bool mute );
        void trackPlaying( Meta::TrackPtr track );
        void stopped();
        void paused();
        void metadataChanged();

    public slots:
        /**
         * When user pushs global shortcut or uses script to toggle
         * even if it is disabled()
         */
        void forceToggleOSD();

    private:
        OSD();
        ~OSD();

        static OSD* s_instance;

        Meta::TrackPtr m_currentTrack;
    };
}

#endif /*AMAROK_OSD_H*/

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  osd.h   -  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Fre Sep 26 2003
  copyright: (C) 2003 by Christian Muehlhaeuser
  email:     chris@chris.de
*/

#ifndef AMAROK_OSD_H
#define AMAROK_OSD_H

#include "meta/Meta.h"

#include <QImage>
#include <QPixmap>
#include <QWidget> //baseclass


#define OSD_WINDOW_OPACITY 0.8

namespace Plasma
{
    class PanelSvg;
}

class OSDWidget : public QWidget
{
    Q_OBJECT

    public:
        enum Alignment { Left, Middle, Center, Right };

        explicit OSDWidget( QWidget *parent, const char *name = "osd" );

        /** resets the colours to defaults */
        void unsetColors();

      public slots:
        /** calls setText() then show(), after setting image if needed */
        void show( const QString &text, QImage newImage = QImage::QImage() );
        void ratingChanged( const short rating );
        void ratingChanged( const QString& path, int rating );
        void volChanged( unsigned char volume );

        /** reimplemented, shows the OSD */
        virtual void show();

        /**
         * For the sake of simplicity, when these settings are
         * changed they do not take effect until the next time
         * the OSD is shown!
         *
         * To force an update call show();
         */
        void setDuration( int ms ) { m_duration = ms; }
        void setTextColor( const QColor &color )
        {
            QPalette palette = this->palette();
            palette.setColor( QPalette::Active, QPalette::WindowText, color );
            setPalette(palette);
        }
        void setOffset( int y ) { m_y = y; }
        void setAlignment( Alignment alignment ) { m_alignment = alignment; }
        void setImage( const QImage &image ) { m_cover = image; }
        void setScreen( int screen );
        void setText( const QString &text ) { m_text = text; }
        void setDrawShadow( const bool b ) { m_drawShadow = b; }
        void setRating( const short rating ) { if ( isEnabled() ) m_rating = rating; }
        void setTranslucent( bool enabled ) { setWindowOpacity( enabled ? OSD_WINDOW_OPACITY : 1.0 ); }

    protected:
        virtual ~OSDWidget();

        /** determine new size and position */
        QRect determineMetrics( const int marginMetric );

        /** reimplemented */
        virtual void paintEvent( QPaintEvent* );
        virtual void mousePressEvent( QMouseEvent* );
        void resizeEvent( QResizeEvent *e );
        virtual bool event( QEvent* );

        /** distance from screen edge */
        static const int MARGIN = 15;

        uint        m_m;
        QSize       m_size;
        int         m_duration;
        QTimer     *m_timer;
        Alignment   m_alignment;
        int         m_screen;
        uint        m_y;
        bool        m_drawShadow;
        short       m_rating;
        unsigned char m_newvolume;
        bool        m_volume;
        QString     m_text;
        QImage      m_cover;
        QPixmap     m_scaledCover;
};



class OSDPreviewWidget : public OSDWidget
{
    Q_OBJECT

public:
    OSDPreviewWidget( QWidget *parent );

    int screen()    { return m_screen; }
    int alignment() { return m_alignment; }
    int y()         { return m_y; }

public slots:
    void setTextColor( const QColor &color ) { OSDWidget::setTextColor( color ); doUpdate(); }
    void setDrawShadow( bool b ) { OSDWidget::setDrawShadow( b ); doUpdate(); }
    void setFont( const QFont &font ) { OSDWidget::setFont( font ); doUpdate(); }
    void setScreen( int screen ) { OSDWidget::setScreen( screen ); doUpdate(); }
    void setUseCustomColors( const bool use, const QColor &fg )
    {
        if( use ) OSDWidget::setTextColor( fg );
        else      unsetColors();
        doUpdate();
    }
    void setTranslucent( bool enabled ) { setWindowOpacity( enabled ? OSD_WINDOW_OPACITY : 1.0 ); doUpdate(); }

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
    class OSD : public OSDWidget, public Meta::Observer
    {
        Q_OBJECT

    public:
        static OSD *instance()
        {
            static OSD *s_instance = new OSD;
            return s_instance;
        }

        void applySettings();
        virtual void show( Meta::TrackPtr track );

        //Reimplemented from Meta::Observer
        using Meta::Observer::metadataChanged;
        virtual void metadataChanged( Meta::Track *track );
        virtual void metadataChanged( Meta::Album *album );
        virtual void metadataChanged( Meta::Artist *artist );

        // Don't hide baseclass methods - prevent compiler warnings
        virtual void show() { OSDWidget::show(); }

    public slots:
        /**
         * When user pushs global shortcut or uses script to toggle
         * even if it is disabled()
         */
        void forceToggleOSD();

    private:
        OSD();

        Meta::TrackPtr m_track;
    };
}

#endif /*AMAROK_OSD_H*/

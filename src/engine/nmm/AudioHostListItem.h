#ifndef AUDIOHOSTLISTITEM_H
#define AUDIOHOSTLISTITEM_H

#include <qcolor.h>
#include <qlabel.h>
#include <qstring.h>
#include <qwidget.h>

class ServerregistryPing;

class AudioHostListItem : public QWidget {
    Q_OBJECT
    
    public:
        AudioHostListItem( bool, QString, QWidget * );
        ~AudioHostListItem();

        void setHighlighted( bool = true );
        QString hostname() const;

    protected:
        void mousePressEvent ( QMouseEvent * );

    signals:
        void pressed( AudioHostListItem* );

    private slots:
        void registryAvailable( bool );

    private:
        /**
         * Calculates background color.
         */
        QColor calcBackgroundColor( QString, QColor );
        
        QLabel *statusButton;
        QLabel *hostLabel;

        ServerregistryPing *registry;
};

#endif

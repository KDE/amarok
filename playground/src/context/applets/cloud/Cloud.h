/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhn@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef CLOUD_APPLET_H
#define CLOUD_APPLET_H

#include <context/Applet.h>
#include <context/DataEngine.h>
#include <context/Svg.h>

#include <KDialog>

#include <QGraphicsProxyWidget>
#include <QTimeLine>
#include <qwebview.h>

class QCheckBox;
class QGraphicsPixmapItem;
class QHBoxLayout;
class QLabel;
class QSpinBox;


/**
A simple text item to go in a coud box

    @author Nikolaj Hald Nielsen <nhn@kde.org>
 */
class CloudTextItem : public QGraphicsTextItem
{

Q_OBJECT
public:
    explicit CloudTextItem( const QString &text, QGraphicsItem *parent = 0, QGraphicsScene *scene = 0 );

protected:
    virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
    virtual void hoverLeaveEvent ( QGraphicsSceneHoverEvent * event );
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );

public slots:
    void colorFadeSlot( int step );

private:
    QTimeLine * m_timeLine;


signals:
    void clicked( const QString &text );
};


class Cloud : public Context::Applet
{
    Q_OBJECT
public:
    Cloud( QObject* parent, const QVariantList& args );
    ~Cloud();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    bool hasHeightForWidth() const;
    qreal heightForWidth( qreal width ) const;

    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint) const;

public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );

private slots:
    void cloudItemActivated( const QString &text );

private:

    void addText( const QString &text, int weight );

    void cropAndNormalize( int minCount, int maxCount );

    void drawCloud();

     /**
     * Adjusts the current line in the cloud so all ellements are aligned vertically
     * and the whole line is centered in the cloud box
     */
    void adjustCurrentLinePos();

    qreal m_runningX;
    qreal m_runningY;
    qreal m_currentLineMaxHeight;

    int m_maxFontSize;
    int m_minFontSize;

    float m_maxHeightInFirstLine;

    QList<CloudTextItem *> m_currentLineItems;
    QList<CloudTextItem *> m_textItems;

    KDialog* m_config;
    QHBoxLayout* m_configLayout;
    QSpinBox* m_spinWidth;
    int m_width;

    qreal m_aspectRatio;

    Context::Svg* m_theme;
    QSizeF m_size;

    QGraphicsSimpleTextItem* m_cloudName;

    QList<QVariant> m_strings;
    QList<QVariant> m_weights;
    QVariantMap m_actions;

    bool m_initialized;
};

K_EXPORT_AMAROK_APPLET( cloud, Cloud )

#endif

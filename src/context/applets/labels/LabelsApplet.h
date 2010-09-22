/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2009 simon.esneault <simon.esneault@gmail.com>                         *
 * Copyright (c) 2010 Daniel Faust <hessijames@gmail.com>                               *
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

#ifndef LABELS_APPLET_H
#define LABELS_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "core/engine/EngineObserver.h"

#include <ui_labelsSettings.h>

#include <QPointer>

class TextScrollingWidget;
class LabelGraphicsItem;
class KComboBox;
class QGraphicsProxyWidget;


class LabelsApplet : public Context::Applet, public Engine::EngineObserver
{
    Q_OBJECT

public:
    LabelsApplet( QObject *parent, const QVariantList &args );
    ~LabelsApplet();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );
    
    // inherited from EngineObserver
    virtual void engineNewTrackPlaying();
    virtual void enginePlaybackEnded( qint64 finalPosition, qint64 trackLength, PlaybackEndedReason reason );

public slots:
    virtual void init();
    void dataUpdated( const QString &name, const Plasma::DataEngine::Data &data );
    void connectSource( const QString &source );
    void toggleLabel( const QString &label );
    void listLabel( const QString &label );
    void blacklistLabel( const QString &label );
    void addLabelPressed();
    void saveSettings();
    
protected:
    void createConfigurationInterface(KConfigDialog *parent);

private:
    void updateLabels();
    void startDataQuery();

    QMap < QString, QVariant >    m_labelInfos;
    QList < LabelGraphicsItem * > m_labelItems;
    QStringList             m_currentLabels;
    QStringList             m_allLabels;
    
    QString                 m_titleText;
    TextScrollingWidget     *m_titleLabel;
    KComboBox               *m_addLabel;

    QPointer<QGraphicsProxyWidget> m_addLabelProxy;

    int                     m_numLabels;
    int                     m_minCount;
    int                     m_personalCount;
    bool                    m_autoAdd;
    int                     m_minAutoAddCount;
    QStringList             m_blacklist;
    bool                    m_stoppedstate;

    QPointer<Plasma::IconWidget> m_reloadIcon;
    QPointer<Plasma::IconWidget> m_settingsIcon;

    Ui::labelsSettings      ui_Settings;

private slots:
    void resultReady( const QString &collectionId, const Meta::LabelList &labels );
    void reload();

};

K_EXPORT_AMAROK_APPLET( labels, LabelsApplet )

#endif

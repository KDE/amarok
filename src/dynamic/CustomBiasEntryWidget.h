/****************************************************************************************
 * Copyright (c) 2009, 2010 Leo Franchi <lfranchi@kde.org>                              *
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

#ifndef DYNAMIC_CUSTOM_BIAS_ENTRY_WIDGET_H
#define DYNAMIC_CUSTOM_BIAS_ENTRY_WIDGET_H

#include "amarok_export.h"
#include "DynamicBiasWidgets.h"

namespace Dynamic
{

class CustomBias;

// this should not be subclassed by implementing biases. this will call the widget() function
// of the CustomBiasEntry set on the CustomBias.
class AMAROK_EXPORT CustomBiasEntryWidget : public PlaylistBrowserNS::BiasWidget
{
    Q_OBJECT
    public:
        explicit CustomBiasEntryWidget( CustomBias* bias, QWidget* parent = 0 );

    signals:
        void weightChangedInt( int );

    public slots:
        void refreshBiasFactories();

    private slots:
        void selectionChanged( int index );
        void weightChanged( int amount );

    private:
        void setCurrentLoadedBiasWidget();

        CustomBias* m_cbias;
        QWidget* m_currentConfig;

        QGridLayout* m_layout;
        Amarok::Slider* m_weightSelection;
        QLabel*         m_weightLabel;
        QLabel*         m_withLabel;
        KComboBox*      m_fieldSelection;
};

}

#endif

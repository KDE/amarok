/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2005
 *                    NMM work group,
 *                    Computer Graphics Lab,
 *                    Saarland University, Germany
 *                    http://www.networkmultimedia.org
 *
 * Maintainer:        Robert Gogolok <gogo@graphics.cs.uni-sb.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307
 * USA
 */

#ifndef NMMCONFIGDIALOGBASE_H
#define NMMCONFIGDIALOGBASE_H

#include <qpixmap.h>
#include <qwidget.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QLabel;
class KComboBox;
class QButtonGroup;
class QRadioButton;
class QListBox;
class QListBoxItem;
class QPushButton;

class NmmConfigDialogBase : public QWidget
{
    Q_OBJECT

public:
    NmmConfigDialogBase( QWidget* parent = 0);
    ~NmmConfigDialogBase();

    KComboBox* audioPlaybackNode;

    QButtonGroup* audioGroup;
    QButtonGroup* videoGroup;

    QRadioButton* audioLocalhostButton;
    QRadioButton* videoLocalhostButton;

    QRadioButton* audioHostListButton;
    QRadioButton* videoHostListButton;

    QListBox* audioListBox;
    QListBox* videoListBox;

    QPushButton* addAudioLocationButton;
    QPushButton* removeAudioLocationButton;
    QPushButton* addVideoLocationButton;
    QPushButton* removeVideoLocationButton;
    
    QRadioButton* audioEnvironmentButton;
    QRadioButton* videoEnvironmentButton;

};

#endif // NMMCONFIGDIALOGBASE_H

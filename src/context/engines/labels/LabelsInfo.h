/****************************************************************************************
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

#ifndef AMAROK_LABELS_INFO
#define AMAROK_LABELS_INFO

#include "context/DataEngine.h"
#include "core/support/Debug.h"

//!  Struct LabelsInfo, contains all the info for a label
class LabelsInfo {

public:

    LabelsInfo()
    {}

    ~LabelsInfo()
    {}

    // stores the label and it's importance (value between 0 and 100)
    QString name;
    int count;
};

#endif

/****************************************************************************************
* Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
*                                                                                      *
* This program is free software; you can redistribute it and/or modify it under        *
* the terms of the GNU General Public License as published by the Free Software        *
* Foundation; either version 2 of the License, or (at your option) any later           *
* version.                                                                             *
*                                                                                      *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
* PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
*                                                                                      *
* You should have received a copy of the GNU General Public License along with         *
* this program.  If not, see <http://www.gnu.org/licenses/>.                           *
****************************************************************************************/

#ifndef AMAROK_VIDEOCLIP_INFO
#define AMAROK_VIDEOCLIP_INFO

#include "context/DataEngine.h"

//!  Struct VideoInfo, contain all the info vor a video
struct VideoInfo {
    QString url;        // Url for the browser (http://www.youtube.com/watch?v=153d9tc3Oao )
    QString title;      // Name of the video
    QString coverurl;   // url of the cover
    QString duration;   // formatted as a QString(mm:ss)
    QString desc;       // full description
    QPixmap * cover;    // Image data
    QString views;      // number of view of the video
    float rating;       // rating should be beetween 0 to 5
    QString videolink;  // direct video link to the downloadable file
    QString source;     // "youtube" or "dailymotion" or "vimeo" or whatever
    int relevancy;      // used to filter and order the files
    int length;         // length in seconds
    QString artist;     // The artist just to show it in the artist name
    bool isHQ;          // if HQ is available, turn to true;
};

#endif
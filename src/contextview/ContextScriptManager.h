/***************************************************************************
 *   Copyright (C) 2007 by Leo Franchi <lfranchi@gmail.com>                *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef CONTEXT_SCRIPT_MANAGER_H
#define CONTEXT_SCRIPT_MANAGER_H

#include "GenericInfoBox.h"

#include <QString>
#include <QMap>

namespace Context
{


typedef QMap< int, GenericInfoBox* > BoxMap;

class ContextScriptManager : public QObject
{
    Q_OBJECT

public:
        
    ContextScriptManager() { m_boxes = new BoxMap(); }
    
    static ContextScriptManager* instance() {  return s_instance ? s_instance : new ContextScriptManager(); }
    
    ~ContextScriptManager() { delete m_boxes; }
public slots:
    int addContextBox( const QString& title, const QString& contents, const QString& stylesheet); // returns box ID, so scripts can refer to boxes later
    void changeBoxTitle( const int boxId, const QString& title);
    void changeBoxContents( const int boxId, const QString& contents );
    void changeBoxStylesheet( const int boxId, const QString& stylesheet);
    
    void removeContextBox( int boxId );
    
private:
    BoxMap* m_boxes;
    
    static ContextScriptManager* s_instance;
};

} // namespace Context  

#endif //CONTEXT_SCRIPT_MANAGER_H

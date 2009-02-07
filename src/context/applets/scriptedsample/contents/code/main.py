#*****************************************************************************
#* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>            *
#*****************************************************************************/

#***************************************************************************
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License as published by  *
#*   the Free Software Foundation; either version 2 of the License, or     *
#*   (at your option) any later version.                                   *
#*                                                                         *
#***************************************************************************/

#
# This is the sample scripted context applet. 
#
#


from PyQt4.QtGui import *
from PyKDE4.plasma import Plasma
from PyKDE4 import plasmascript
 
class HelloPython(plasmascript.Applet):
    def __init__(self,parent,args=None):
        plasmascript.Applet.__init__(self,parent)
 
    def init(self):
        self.setHasConfigurationInterface(False)
        self.resize(125, 125)
        self.setAspectRatioMode(Plasma.Square)
 
    def paintInterface(self, painter, option, rect):
        painter.save()
        painter.setPen(Qt.white)
        painter.drawText(rect, Qt.AlignVCenter | Qt.AlignHCenter, "Hello Python!")
        painter.restore()
 
def CreateApplet(parent):
    return HelloPython(parent)
############################################################################
# Zeroconf support - publishing configured streams
# (c) 2005 Jakub Stachowski <qbast@go2.pl>
#
# Depends on: Python 2.2, pyzeroconf 0.12+metaservice patch
############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
############################################################################
import Zeroconf
import socket
from string import split

publisher = None      # SIGTERM handler must be able to reach this

class Publisher:
    
    active = False
    zeroconf = None
    localip = None
    localhostname = None

    def services(self):      # override this to provide list of services to register
        return []
    
    def run(self):	
	
        self.localhostname = split(socket.gethostname(),'.')[0]+'.local.'
	try:
	        self.localip = socket.gethostbyname(self.localhostname)
	        self.zeroconf = Zeroconf.Zeroconf(self.localip)
	except:
		return
	self.active = True

        toRegister = self.services()
        for i in toRegister:
            service = Zeroconf.ServiceInfo(
	    i["type"]+".local.",
            i["name"]+"."+i["type"]+".local.",
	    socket.inet_aton(self.localip),
	    i["port"],
	    0,
	    0,
	    i["properties"],
	    self.localhostname)
	    self.zeroconf.registerService(service)

    def shutdown(self):
        if self.active: self.zeroconf.close()



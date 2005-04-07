#!/usr/bin/python

import socket
import sys
from sre import *

REQ = """
GET /amarok HTTP/1.0 
Host: 192.168.0.100 
User-Agent: TESTER
Accept: */* 
Icy-MetaData:1 
Connection: close

""".lstrip()
buf_size = 4096

addr = sys.argv[1]
host, port, mount = findall(r'http://(.*):(\d{4})/(.*)', addr)[0]
port = int(port)
print 'host = %s \tport = %d\tmount=%s' % (host, port, mount)
s = socket.socket()
s.connect( (host, port) )
s.send( REQ )

buf = s.recv( 1024 ).lower()
meta_interval = int(findall( r'icy-metaint:\s*(\d*)\r\n', buf )[0])
print 'icy-metaint = %d'  % meta_interval

buf = s.recv( buf_size )
byte_counter = len(buf)
while True:
	if byte_counter == meta_interval:
		meta_len = 16 * ord(s.recv(1))
		meta = s.recv(meta_len)
		#print 'meta_len = %d' % meta_len
		if meta_len > 0: print '[%d bytes] %s' % (meta_len, meta)
		byte_counter = 0
	else:
		if byte_counter + buf_size > meta_interval:
			buf = s.recv( meta_interval - byte_counter )
			byte_counter += len(buf)
		else: 
			buf = s.recv( buf_size )
			byte_counter += len(buf)


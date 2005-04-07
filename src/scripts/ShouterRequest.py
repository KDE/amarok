#!/usr/bin/env python

import os
import SocketServer
import binfuncs
from sre import search,findall
from StreamConfig import *
import Globals
import time
import timing
import urllib
import mimetypes
from debug import *
from StreamController import *
from Encoder import *

META = "%cStreamTitle='%s';StreamUrl='%s';%s"
ICYRESP = 'ICY 200 OK\r\nicy-notice1:%s\r\nicy-notice2:%s\r\nicy-name:%s\r\nicy-br:%d\r\nicy-genre:%s\r\nicy-url:%s\r\nicy-metaint:%d\r\n\r\n'
PADDING = '\x00'*16
SILENCE_F = '../scripts/Shouter/silence/silence-%s.mp3'

class ShouterRequest(SocketServer.StreamRequestHandler):
	""" A per-thread streaming object.
	
	Chiefly responsible for handling a specific request, acquiring the stream data,
	and injecting meta-data if needed
	"""

	playlist = []
	file = None
	req_header = None
	icy = False
	cfg = None
	byte_counter = 0
	meta_is_dirty = True
	pl_pos = 0

	# blind is a sentinel boolean so that the loop in the stream method is interruptible
	blind = True

	def __init__(self, request, client_address, stream_controller):
		debug('ShouterRequest init')
		self.server = stream_controller
		self.client_address = client_address
		self.request = request
		self.cfg = stream_controller.cfg
		self.req_header = str(request.recv( 4092 ))

	def run(self):
		debug( 'streamrequest.run' )
		if self.req_header.startswith( 'PROPFIND' ):
			self._do_PROPFIND()

		if self.req_header.startswith('HEAD') :
			self._do_HEAD()

		if self.req_header.startswith( 'GET' ):
			self._do_GET()
		
	def _send_status( self, status, headers=dict()):
		response = 'HTTP/1.1 %d OK\r\n' % status
		for h in headers.keys():
			response += str(h) + ': ' + str(headers[h]) + '\r\n'
		response += '\r\n'
		debug( 'send_status sending response:\n%s' % response )
		self.request.send( response )
		
	def _check_request( self ):
		debug ('_check_request')
		req_type, mount = findall(r'(\w*) (/\S*) ', self.req_header)[0]
		if mount != self.cfg.mount:
			if self.cfg.enable_dl and req_type == 'GET':
				if len(self.server.playlist)>0: f = self.server.playlist[-1]
				else: f=''
				url = urllib.pathname2url(f)
				if mount == self.cfg.dl_mount:
					headers = {'Location':url}
					self._send_status(300, headers )
				elif mount == url:
					headers = {
						'Content-length':os.stat(f)[6],
						'Content-type':mimetypes.guess_type(url)[0]
						}
					self._send_status(200, headers)
					self.send_file(f)
			else:
				self.server.log( 'Request for unmapped mount %s' % mount )
				self._send_status(404)
			self.server.sockets.pop(self.request)
			return False

		req_header_l = ''
		for l in self.req_header.splitlines():
			req_header_l += l + ' '

		debug( req_header_l )
		if search( r'icy-metadata:\s*1', self.req_header.lower() ):
			self.icy = True

		self.server.log( '-- %s -- : %s' % (self.request.getpeername()[0], req_header_l ))
		return True
		
	def send_file(self, file):
		debug( 'send file %s' % file )
		f = open(file, 'r')
		buf = f.read( self.cfg.buf_size )
		while len(buf) != 0 :
			timing.start()
			self.request.send( buf )
			timing.finish()
			self.byte_counter += len(buf)
			buf = f.read( self.cfg.buf_size )
			try:
				t_s = len(buf)/1024.0 / self.cfg.dl_throttle - (timing.micro() * 1e-6)
				if t_s: time.sleep( t_s )
			except:
				pass
		
	def _do_HEAD(self):
		debug( '_do_HEAD' )
		if self._check_request(): 
			if self.icy :
				resp = ICYRESP % (self.cfg.desc1, self.cfg.desc2, self.cfg.name, self._get_bitrate(self.server.playlist[-1]), self.cfg.genre, self.cfg.url, self.cfg.icy_interval )
				self.request.send(resp)
			else :
				# amarok doesn't respond well to mpegurl mimetype streams
				if search( r'user-agent:\s?amarok', self.req_header.lower()):
					resp ='HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n' 
				resp ='HTTP/1.0 200 OK\r\nContent-Type: audio/x-mpegurl\r\n\r\n' 
				self.request.send( resp )

	def _do_PROPFIND(self):
		debug( '_do_PROPFIND' )
		# My enthusiasm for webdav wanes. We'll assume that such a bold
		# request could only have come from a kde app. For now, they get a canned response. 
		self._check_request()
		dav_resp_header = """
		HTTP/1.1 207 Multi-Status
		Content-Type: text/xml; charset="utf-8"
		Content-Length: %s

		""".lstrip()

		dav_resp_body = """
		<?xml version="1.0" encoding="utf-8" ?>
		<D:multistatus xmlns:D="DAV:">
			<D:response>
			</D:response>
		</D:multistatus>
		""".lstrip()
		self.request.send( (dav_resp_header % len(dav_resp_body) + dav_resp_body))

	def _do_GET(self):
		debug( '_do_GET' )
		if self._check_request() : 
			if self.icy:
				resp = ICYRESP % (self.cfg.desc1, self.cfg.desc2, self.cfg.name, self._get_bitrate(self.server.playlist[-1]), self.cfg.genre, self.cfg.url, self.cfg.icy_interval )
				self.request.send(resp)
			else:
				#resp ='HTTP/1.0 200 OK\r\nContent-Type: audio/x-mpegurl\r\n\r\n' 
				resp ='HTTP/1.0 200 OK\r\n\r\n' 
				self.request.send( resp )

			# start this stream with the last song in the list
			self.pl_pos = len(self.server.playlist)-1
			if self.pl_pos==-1: self.pl_pos=0
			has_played_once = False
			while True:
				pl = self.server.playlist
				if len(pl)==0 or self.pl_pos>=len(pl):
					debug('Sending silence to %s' % str(self.request.getpeername()[0]))
					self.stream(SILENCE_F % 160, 0, 160)
				else:
					self.file = pl[self.pl_pos]
					self.meta_is_dirty = True
					if has_played_once is False:
						pos = self._get_play_cursor()
						self.stream(self.file, pos)
					else: 
						self.stream(self.file)
					has_played_once = True
					self.pl_pos += 1

	def _get_play_cursor(self):
		""" Find byte position via times returned by dcop """		

		debug( '_get_play_cursor %s ' % str(self) )
		total = int(Globals.PlayerDcop( 'trackTotalTime' ).result().rstrip())
		current = int(Globals.PlayerDcop( 'trackCurrentTime' ).result().rstrip()) + self.cfg.pre_seek
		if total < current : total = current
		frac = float(current)/total * self.cfg.punc_factor/100.0
		size = os.stat(self.server.playlist[-1])[6]
		return long(frac*size)

	def get_meta( self ):
		""" Transform ID3 info into something appropriate for streams """

		if self.meta_is_dirty:
			debug( 'get_meta meta_is_dirty = True' )
			title = "%s - %s" % (Globals.PlayerDcop('artist').result().rstrip(), Globals.PlayerDcop('title').result().rstrip())
			addr = self.request.getsockname()
			download_url = ''
			if self.cfg.enable_dl:
				download_url = 'http://%s:%d%s' % (addr[0], addr[1], self.cfg.dl_mount)

			# The literal 28 is the number of static characters in the META string (see top)
			length = len(title) + len(download_url) + 28
			padding = 16 - length % 16 
			meta = META % ( (length + padding)/16, title, download_url, PADDING[:padding] )
			#debug('sending meta string: %s' % meta)
			self.meta_is_dirty = False
			return meta
		else:
			return '\x00'
	
	def _get_bitrate( self, file ):
		temp = Globals.PlayerDcop( "bitrate" ).result().rstrip()
		bitrate = 160
		try:
			bitrate = int(findall( r'(\d*)\skbps', temp )[0])
		except:
			bitrate = 160
		return bitrate

	def stream( self, file, pos=0, bitrate=0 ):
		debug('ShouterRequest.stream. This should not happen')

class ReencodedRequest(ShouterRequest):
	""" Reencoding stream request handler """

	def stream(self, file, pos=0, bitrate=0):
		debug( 'ReencodedRequest.stream %s' % file )

		buf = 0
		bitrate = self.cfg.stream_br
		size = os.stat(file)[6]
		sleep_factor = 8.0/(bitrate * 1024.0)
		
		self.blind = True
		f = None
		try:
			f = self.server.coded_files[file]
		except KeyError:
			f = Encoder(file)

		while self.blind and pos < size :
			bytes_till_meta = self.cfg.icy_interval - self.byte_counter
			if bytes_till_meta == 0:
				# send meta and reset counter
				if self.icy:
					meta = self.get_meta()
					self.request.send(meta) 
				self.byte_counter = 0
			if bytes_till_meta < self.cfg.buf_size :
				# send whats left. Will update meta on next iteration
				buf = f.read_from(bytes_till_meta, pos)
				self.request.send(buf)
				self.byte_counter += len(buf)
				pos += len(buf)
			else :
				# Send a normal buffer
				buf = f.read_from(self.cfg.buf_size, pos)
				self.request.send(buf)
				self.byte_counter += len(buf)
				pos += len(buf)
			sleep_int = len(buf) * sleep_factor
			time.sleep(sleep_int)

class StreamRequest(ShouterRequest):
	""" Non-reencoding stream request handler """

	def _get_data_start( self, file ):
		""" Find where ID3 tags end and file data begins """

		f = open(file, 'r')
		f.seek(6)
		length = f.read(4)
		start = binfuncs.bin2dec( binfuncs.bytes2bin( length, 7)) + 10
		f.close()
		return start

	def stream( self, file, pos=0, bitrate=0 ):
		debug('stream byte_counter=%d pos=%d file=%s' % (self.byte_counter, pos, file))
		buf = 0
		if bitrate==0: bitrate = self._get_bitrate(file)
		size = os.stat(file)[6]
		sleep_factor = 8.0/(bitrate * 1024.0)
		
		f = open( file, 'r' )

		# this isn't quite right, but is of relatively little consequence
		# as the headers themselves are very small
		f.seek( self._get_data_start(file) + pos  )
		self.blind = True
		while self.blind and f.tell() < size :
			bytes_till_meta = self.cfg.icy_interval - self.byte_counter
			if bytes_till_meta == 0:
				# send meta and reset counter
				if self.icy:
					meta = self.get_meta()
					self.request.send(meta) 
				self.byte_counter = 0
			if bytes_till_meta < self.cfg.buf_size :
				# send whats left. Will update meta on next iteration
				buf = f.read(bytes_till_meta)
				self.request.send(buf)
				self.byte_counter += len(buf)
			else :
				# Send a normal buffer
				buf = f.read(self.cfg.buf_size)
				self.request.send(buf)
				self.byte_counter += len(buf)
			sleep_int = len(buf) * sleep_factor
			time.sleep(sleep_int)


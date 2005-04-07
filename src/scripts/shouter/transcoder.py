#!/usr/bin/python

# method:
# get start of file
# write real data as smaller chunked files
# decode files to wav
# encode wav to mp3
# delete wav
# serve mp3
# repeat for next chunk

from binfuncs import *
import sys
import os
import tempfile
import time

file1 = None
CHUNK_SIZE = 524288
chunks = 0
try:
	file1 = sys.argv[1]
except:
	file1 = 'file1.mp3'
	
f_orig = open(file1, 'r')
f_orig.seek(6)
length = f_orig.read(4)
start = bin2dec(bytes2bin(length,7)) + 10
f_orig.seek(start)

while f_orig.tell() < os.stat(f_orig.name)[6]:
	f_chunk = tempfile.NamedTemporaryFile( suffix='.chunk', prefix='shouter-', dir='/tmp' )
	f_pcm = tempfile.NamedTemporaryFile( suffix='.chunk.pcm', prefix='shouter-', dir='/tmp' )
	f_ogg = tempfile.NamedTemporaryFile( suffix='.chunk.ogg', prefix='shouter-', dir='/tmp' )

	f_orig.seek(chunks * CHUNK_SIZE)
	f_chunk.write(f_orig.read(CHUNK_SIZE))
	chunks += 1

	os.popen( 'mplayer -ao pcm %s -aofile %s' % (f_chunk.name, f_pcm.name) )
	f_chunk.close()

	os.popen( 'oggenc -r %s -o %s' % (f_pcm.name, f_ogg.name) )
	f_pcm.close()
	time.sleep(10)
	f_ogg.close()


class Transcoder:
	chunk_size = 524288
	chunks = 0

class DataProvider:
	source, format = '', ''
	transcoder = None

	def __init__(self, source, format):
		self.source = source
		self.format = format

	def seek(self, pos):
		pass

	def read(self, length):
		pass

import os
from binfuncs import *
from debug import *
import tempfile

GENERICDEC = 'mplayer -really-quiet -ao pcm %s -aofile %s'

OGGDEC = 'oggdec -Q %s -o %s'
OGGENC = 'oggenc -Q -b %d -r %s -o %s'

MP3DEC = 'lame --quiet decode --mp3input %s %s'
MP3ENC = 'lame --quiet -b %d %s %s'


class GenericHelper:
    def chunk(self, chunkname, mastername, pos, chunk_size):
        """This really should be overridden, this implementation is not to be trusted"""

        debug('WARNING: Chunking with GenericHelper. Don\'t expect this to work on data with headers')
        pos, chunk_size = int(pos), int(chunk_size)
        fmaster_d = file(mastername, 'r')
        fchunk_d = file(chunkname, 'w')
        fmaster_d.seek(pos)
        fchunk_d.write(fmaster_d.read(chunk_size))
        fchunk_d.close()
        fmaster_d.close()

    def decode(self, inname, wavname):
        debug('generic decode')
        os.popen(GENERICDEC % (inname, wavname))

    def get_data_start(self, fname):
        return 0

class oggHelper(GenericHelper):
    def chunk(self, chunkname, mastername, pos, chunk_size):
        # do it the bad and extremely inefficient way by
        # making two cuts per chunk, fore and aft
        # syntax: vcut infile.ogg file1.ogg file2.ogg cutpoint
        # cutpoint = sample-rate (ie 441000) * seconds
        #         = pos * 8 * 44100 / (bitrate * 1024)
        #         = pos/bitrate * 344.53125
        # FIXME: Use actual bitrate and sample rate, though this doesn't
        # really make a big difference.
        debug( 'ogg chunk pos=%d chunk_size=%d' % (pos, chunk_size))
        br = 160.0
        cut1 = pos/br * 344.53125 
        tf = tempfile.NamedTemporaryFile(suffix='.temp', prefix='shouter-', dir='/tmp')
        if pos:    
            cmd = 'vcut "%s" "%s" %s %d' % (mastername, tf.name, chunkname, cut1)
            os.popen(cmd)

        cut2 = (chunk_size)/br * 344.53125
        cmd = 'vcut "%s" "%s" %s %d' % (mastername, chunkname, tf.name, cut2)
        os.popen(cmd)
        tf.close()
        debug('Finished cutting ogg at %d and %d' % (cut1, cut2))

    def decode(self, oggname, wavname):
        debug('ogg decode: %s -> %s' % (oggname, wavname))
        os.popen(OGGDEC % (oggname, wavname))

    def encode(self, wavname, oggname, bitrate):
        debug('ogg encode: %s -> %s' % (wavname, oggname))
        os.popen(OGGENC % (bitrate, wavname, oggname))
    
class mp3Helper(GenericHelper):
    def decode(self, mp3name, wavname):
        debug(MP3DEC % (mp3name, wavname))
        os.popen(MP3DEC % (mp3name, wavname))

    def encode(self, wavname, mp3name, bitrate):
        # encode pcm->mp3
        temp = tempfile.NamedTemporaryFile(prefix='shouter-', suffix='.temp', dir='/tmp')
        debug('mp3 encode: %s -> %s' % (wavname, mp3name))
        os.popen(MP3ENC % (bitrate, wavname, temp.name))
        
        # Strip mp3 headers
        f = file(mp3name, 'w')
        temp.seek(self._get_data_start(temp.name))
        #temp.seek(0)
        buf = temp.read(4096)
        while len(buf) != 0:
            f.write(buf)
            buf = temp.read(4096)
        temp.close()
        f.close()

    def _get_data_start(self, fname):
        f = open(fname, 'r')
        f.seek(6)
        length = f.read(4)
        start = bin2dec(bytes2bin(length, 7)) + 10
        f.close()
        return start


############################################################################
# Binary file functions. On loan from eye3D <http://eye3d.nicfit.net>
# (c) 2005 James Bellenger <jamesb@squaretrade.com>
#
# Depends on: Python 2.2 
############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
############################################################################

def get_mp3_start(fname):
    """ Find where ID3 tags end and file data begins """

    f = open(fname, 'r')
    id3 = f.read(3)
    if not id3=="ID3": return 0
    f.seek(6)
    l = f.read(4)
    start = bin2dec(bytes2bin(l, 7)) + 10
    f.close()
    return start

def bytes2bin(bytes, size = 8):
    retVal = []
    for b in bytes:
        bits = []
        b = ord(b)
        while b > 0:
            bits.append( b & 1 )
            b >>= 1

        if len(bits) < size:
            bits.extend([0] * (size - len(bits)))
        elif len(bits) > size:
            bits = bits[:size]

        # Big endian
        bits.reverse()
        retVal.extend(bits)

    if len(retVal) == 0:
        retVal = [0]
    return retVal;

def bin2dec(x):
    bits = []
    bits.extend(x)
    bits.reverse()

    multi = 1
    value = long(0)
    for b in bits:
        value += b * multi
        multi *= 2
    return value

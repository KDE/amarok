#!/usr/bin/env python

# Shamelessly ganked from eye3D

def bytes2bin( bytes, size = 8 ):
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

def bin2dec( x ):
    bits = []
    bits.extend(x)
    bits.reverse()

    multi = 1
    value = long(0)
    for b in bits:
        value += b * multi
        multi *= 2
    return value



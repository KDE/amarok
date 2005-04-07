debug_prefix = "[Shouter]"
debug_h = open('shouter.debug', 'a')
def debug( message ):
	debug_h.write( '%s %s\n' % (debug_prefix, str(message)))
	debug_h.flush()

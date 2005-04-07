class StreamConfig:
	""" Container for stream configuration. Values are defaults and should be
	reset when the configuration is loaded from disk """

	mount = '/amarok'
	genre = 'Mixed'
	name = 'amaroK shouter'
	url = 'http://amarok.kde.org'
	desc1, desc2 = '', ''
	port = 8000
	buf_size = 4096
	icy_interval = 16384
	max_clients = 4
	punc_factor = 100
	pre_seek = 2
	force_update = True
	enable_dl = True
	dl_mount = '/current'
	dl_throttle = 20
	reencoding = 1
	stream_format = 'mp3'
	stream_br = 192
	chunk_size = 524288

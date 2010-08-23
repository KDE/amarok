import subprocess
import time
import unittest

import dbus

APPNAME = "amarok"
MPRIS_OBJECT_PATH = "/org/mpris/MediaPlayer2"
MPRIS_PREFIX = "org.mpris.MediaPlayer2"

ROOT_IFACE = MPRIS_PREFIX
PLAYER_IFACE = MPRIS_PREFIX + ".Player"

def get_mpris_object():
    bus = dbus.SessionBus()
    return bus.get_object(MPRIS_PREFIX + "." + APPNAME, MPRIS_OBJECT_PATH)

def start_player():
    devnull = file("/dev/null", "a")
    player = subprocess.Popen(["amarok", "--nofork"], stderr=devnull, stdout=devnull)
    time.sleep(10)
    return player

def stop_player(player):
    player.terminate()
    player.wait()

class TestMediaPlayer2(unittest.TestCase):
    def check_property(self, props, key, expected_value):
        value = props[key]
        self.assertEquals(value, expected_value)
        del props[key]

    def check_has_property(self, props, key):
        self.assert_(key in props)
        del props[key]

    def test_properties(self):
        player = start_player()
        try:
            mpris = get_mpris_object()
            props = mpris.GetAll(ROOT_IFACE)
            self.check_property(props, "CanQuit", True)
            self.check_property(props, "CanRaise", True)
            self.check_property(props, "HasTrackList", False)
            self.check_property(props, "Identity", "Amarok")
            self.check_property(props, "DesktopEntry", "amarok")
            self.check_has_property(props, "SupportedUriSchemes")
            self.check_has_property(props, "SupportedMimeTypes")
            self.assertEquals(len(props), 0)
        finally:
            stop_player(player)

    def test_quit(self):
        player = start_player()
        mpris = get_mpris_object()
        mpris.Quit()
        start = time.time()
        while time.time() < start + 60 * 5 and player.poll() is None:
            time.sleep(.5)
        if player.poll() is None:
            stop_player(player)
            self.fail("Player not stopped")

unittest.main()

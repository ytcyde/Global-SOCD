import obspython as obs
from pynput import keyboard
import threading

class KeyHandler:
    def __init__(self):
        self.a_pressed = False
        self.d_pressed = False
        self.listener = None

    def on_press(self, key):
        try:
            if key.char == 'a':
                self.a_pressed = True
                if self.d_pressed:
                    keyboard.Controller().release('d')
            elif key.char == 'd':
                self.d_pressed = True
                if self.a_pressed:
                    keyboard.Controller().release('a')
        except AttributeError:
            pass

    def on_release(self, key):
        try:
            if key.char == 'a':
                self.a_pressed = False
                if self.d_pressed:
                    keyboard.Controller().press('d')
            elif key.char == 'd':
                self.d_pressed = False
                if self.a_pressed:
                    keyboard.Controller().press('a')
        except AttributeError:
            pass

    def start_listener(self):
        if self.listener is None:
            self.listener = keyboard.Listener(on_press=self.on_press, on_release=self.on_release)
            self.listener.start()

    def stop_listener(self):
        if self.listener:
            self.listener.stop()
            self.listener = None

key_handler = KeyHandler()

def on_event(event):
    if event == obs.OBS_FRONTEND_EVENT_RECORDING_STARTED:
        key_handler.start_listener()
    elif event == obs.OBS_FRONTEND_EVENT_RECORDING_STOPPED:
        key_handler.stop_listener()

def script_load(settings):
    obs.obs_frontend_add_event_callback(on_event)

def script_unload():
    obs.obs_frontend_remove_event_callback(on_event)
    key_handler.stop_listener()
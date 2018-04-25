# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""
Automate greetings, lights, unlock and music according to gestures.
"""

from sys import argv, stderr, exit
from os import getenv, system
from time import sleep, time

import numpy as np
import keras

from camera import Camera

from pygame import mixer
from gpiozero import Energenie



# Smooth out spikes in predictions but increase apparent latency.
SMOOTH_FACTOR = 0.7

SHOW_UI = getenv("DISPLAY")
NONE, DOOR, LIGHT, MUSIC, STOP = 0, 1, 2, 3, 4

if SHOW_UI:
    import pygame


class Room:
    """
        Convert gesture actions detected into responses, such as playing music and turning lights
        on. This class is hard-coded for the demo video. It will not work without modification as
        you probably don't have a machine called 'zero' with lock and unlock scripts, or the file
        audio/epic.mp3, which was used under licence.

        To explore your own neural network's behaviour, see run.py instead. Otherwise use this as
        a template to build your own gesture-driven automation.
    """
    def __init__(self):
        self.state = 'empty'
        self.ready = True
        system('ssh zero ./lock &')
        mixer.init()
        self.light = Energenie(1)

    def update(self, action):
        if action == NONE:
            self.ready = True

        if not self.ready:
            return

        if action != NONE:
            self.ready = False

        if action == DOOR:
            if self.state == 'empty':
                self.enter()
                self.state = 'occupied'
            elif self.state == 'occupied':
                self.leave()
                self.state = 'empty'
        elif action == LIGHT:
            self.toggle_light()
        elif action == MUSIC:
            self.play_music()
        elif action == STOP:
            self.dim_music()

    def enter(self):
        stderr.write('\nEnter\n')
        self.light.on()
        mixer.music.load('audio/good-evening.wav')
        mixer.music.set_volume(1.0)
        mixer.music.play()
        system('ssh zero ./unlock &')

    def toggle_light(self):
        if self.light.value:
            stderr.write('\nLight off\n')
            self.light.off()
        else:
            stderr.write('\nLight on\n')
            self.light.on()

    def play_music(self):
        stderr.write('\nMusic play\n')
        mixer.music.load('audio/epic.mp3')
        mixer.music.set_volume(0.5)
        mixer.music.play()

    def dim_music(self):
        stderr.write('\nDim music\n')
        mixer.music.set_volume(0.1)

    def leave(self):
        stderr.write('\nLeave\n')
        mixer.music.fadeout(1)
        mixer.music.load('audio/good-night.wav')
        mixer.music.set_volume(1.0)
        mixer.music.play()
        self.light.off()
        system('ssh zero ./lock &')
        sleep(5)
        mixer.music.load('audio/alarm-active.wav')
        mixer.music.play()


def main():
    if len(argv) != 2 or argv[1] == '--help':
        print("""Usage: story.py MODEL
Walk through a typical evening in the life of a developer.""")
        exit(1)

    model_file = argv[1]

    # Here we load our trained model
    print('Loading model')
    model = keras.models.load_model(model_file)

    # Initialize the camera and sound systems
    camera = Camera(training_mode=False)

    # Create a preview window so we can see if we are in frame or not
    if SHOW_UI:
        pygame.display.init()
        pygame.display.set_caption('Loading')
        screen = pygame.display.set_mode((512, 512))

    # Smooth the predictions to avoid interruptions to audio
    smoothed = np.array([1.0, 0.0, 0.0, 0.0, 0.0])

    # Initialize the room state machine to respond to detected gesture
    room = Room()

    print('Now running!')
    started = time()
    frames = 0
    while True:
        raw_frame = camera.next_frame()
        frames += 1

        # Use MobileNet to get the features for this frame
        x = np.array(raw_frame) / 255.0

        # With these features we can predict a 'normal' / 'yeah' class (0 or 1)
        # Keras expects an array of inputs and produces an array of outputs
        classes = model.predict(np.array([x]))[0]

        # smooth the outputs - this adds latency but reduces interruptions
        smoothed = smoothed * SMOOTH_FACTOR + classes * (1.0 - SMOOTH_FACTOR)
        selected = np.argmax(smoothed) # The selected class is the one with highest probability

        elapsed = time() - started
        fps = frames/elapsed
        summary = 'Ready: %5s, state %10s - class %d [%s] @ %.0f FPS' % (str(room.ready), room.state, selected, ' '.join('%02.0f%%' % (99 * p) for p in smoothed), fps)
        stderr.write('\r' + summary)

        # Perform actions based on the detected gesture
        room.update(selected)

        # Show the image in a preview window so you can tell if you are in frame
        if SHOW_UI:
            pygame.display.set_caption(summary)
            surface = pygame.surfarray.make_surface(raw_frame)
            screen.blit(pygame.transform.smoothscale(surface, (512, 512)), (0, 0))
            pygame.display.flip()

            for evt in pygame.event.get():
                if evt.type == pygame.QUIT:
                    pygame.quit()
                    break


if __name__ == '__main__':
    main()

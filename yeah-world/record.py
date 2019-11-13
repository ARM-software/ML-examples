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
Record frames from the camera and save them to a file. There are many
ways to do this; the method here (pickle raw frames) is a compromise
between requiring few dependencies, being easy to understand, allowing
per-frame data augmentation and performance on a Pi Zero.
"""
from __future__ import print_function
from time import time, sleep
from sys import argv, exit, stdout
from pickle import load, dump
from os.path import exists
from os import getenv
import pygame
from camera import Camera



SHOW_UI = getenv("DISPLAY")

if SHOW_UI:
    pygame.init()

def main():
    if len(argv) != 3:
        print("""Usage: %s FILENAME SECONDS
Record frames from the camera for SECONDS seconds and save them in FILENAME""" % argv[0])
        exit(1)
    name = argv[1]
    seconds = int(argv[2])

    # Initialize the camera and record video frames for a few seconds
    # We select a random exposure for each frame to generate a wide range
    # of lighting samples for training
    camera = Camera(training_mode=True)
    record(camera, name, seconds)


def status(text):
    """ Show a status update to the command-line and optionally the UI if enabled """
    if SHOW_UI:
        pygame.display.set_caption(text)
    stdout.write('\r%s' % text)
    stdout.flush()


def record(camera, filename, seconds):
    """ Record from the camera """

    # Create window so people can see themselves in the camera while we are recording
    if SHOW_UI:
        pygame.display.init()
        pygame.display.set_caption('Loading...')
        screen = pygame.display.set_mode((512, 512))

    delay = 3 # Give people a 3 second warning to get ready
    started = time()
    while time() - started < delay:
        status("Recording in %.0f..." % max(0, delay - (time() - started)))
        sleep(0.1)

    frames = []
    started = time()
    while time() - started < seconds:
        frame = camera.next_frame()
        frames.append(frame)

        # Update our progress
        status("Recording [ %d frames, %3.0fs left ]" %
               (len(frames), max(0, seconds - (time() - started))))

        # Show the image in a preview window so you can tell if you are in frame
        if SHOW_UI:
            surface = pygame.surfarray.make_surface(frame)
            screen.blit(pygame.transform.scale(surface, (512, 512)), (0, 0))
            pygame.display.flip()
            for evt in pygame.event.get():
                if evt.type == pygame.QUIT:
                    pygame.quit()
                    exit(1)

    print('')

    # Save the frames to a file, appending if one already exists
    if exists(filename):
        print("%s already exists, merging datasets" % filename)
        existing = load(open(filename, 'rb'))
        frames += existing

    stdout.write('Writing %d frames to %s... ' % (len(frames), filename))
    stdout.flush()
    dump(frames, open(filename, 'wb'), protocol=2)
    print('done.')


if __name__ == '__main__':
    main()

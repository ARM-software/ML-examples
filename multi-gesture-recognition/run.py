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
Run a trained model on frames from the camera display predictions on stderr.
"""

from sys import argv, stderr, exit
from os import getenv

import numpy as np
import keras

from camera import Camera

# Smooth out spikes in predictions but increase apparent latency. Decrease on a Pi Zero.
SMOOTH_FACTOR = 0.9

SHOW_UI = getenv("DISPLAY")

if SHOW_UI:
    import pygame


def main():
    if len(argv) != 2 or argv[1] == '--help':
        print("""Usage: run.py MODEL
Use MODEL to classify camera frames and display predictions on stderr.""")
        exit(1)

    model_file = argv[1]

    # Create a preview window so we can see if we are in frame or not
    if SHOW_UI:
        pygame.display.init()
        pygame.display.set_caption('Loading')
        screen = pygame.display.set_mode((512, 512))

    # Here we load our trained model
    print('Loading model')
    model = keras.models.load_model(model_file)

    # Initialize the camera and sound systems
    camera = Camera(training_mode=False)

    # Prepare variables to smooth the predictions over multiple frames
    smoothed = np.ones(model.output_shape[1:])
    smoothed /= len(smoothed)

    print('Now running!')
    while True:
        raw_frame = camera.next_frame()
        x = np.array(raw_frame) / 255.0 # preprocess values to be in the range 0-1

        # With these features we can predict a 'normal' / 'yeah' class (0 or 1)
        # Keras expects an array of inputs and produces an array of outputs
        classes = model.predict(np.array([x]))[0]

        # Smooth the outputs - this adds latency but reduces oscillations between predictions
        smoothed = classes #smoothed * SMOOTH_FACTOR + classes * (1.0 - SMOOTH_FACTOR)
        selected = np.argmax(smoothed) # The selected class is the one with highest probability

        # Show the class probabilities and selected class
        summary = 'Class %d [%s]' % (selected, ' '.join('%02.0f%%' % (99 * p) for p in smoothed))
        stderr.write('\r' + summary)

        # Perform actions for selected class here:
        # e.g. use os.system to execute a command, or pygame.mixer to play sounds and music

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

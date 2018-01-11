from sys import argv, stderr, exit
from os import getenv
from random import choice
from glob import glob
from time import time, sleep
from subprocess import Popen

import numpy as np
from tensorflow import keras

from camera import Camera
from pinet import PiNet
from randomsound import RandomSound

# Smooth out spikes in predictions but increase apparent latency. Decrease on a Pi Zero.
SMOOTH_FACTOR = 0.8

show_ui = getenv("DISPLAY") 

if show_ui:
    import pygame


def main():
    if len(argv) != 2 or argv[1] == '--help':
        print('Usage: run.py MODEL\nUse MODEL to classify camera frames and play sounds when class 0 is recognised.')
        exit(1)

    model_file = argv[1]

    # We use the same MobileNet as during recording to turn images into features
    print('Loading feature extractor')
    extractor = PiNet()

    # Here we load our trained classifier that turns features into categories
    print('Loading classifier')
    classifier = keras.models.load_model(model_file)

    # Initialize the camera and sound systems
    camera = Camera(training_mode=False)
    random_sound = RandomSound()


    # Create a preview window so we can see if we are in frame or not
    if show_ui:
        pygame.display.init()
        pygame.display.set_caption('Loading')
        screen = pygame.display.set_mode((512, 512))

    # Smooth the predictions to avoid interruptions to audio
    smoothed = np.ones(classifier.output_shape[1:])
    smoothed /= len(smoothed)

    print('Now running!')
    while True:
        raw_frame = camera.next_frame()

        # Use MobileNet to get the features for this frame
        z = extractor.features(raw_frame)

        # With these features we can predict a 'normal' / 'yeah' class (0 or 1)
        classes = classifier.predict(np.array([z]))[0] # Keras expects an array of inputs and produces an array of outputs

        # smooth the outputs - this adds latency but reduces interruptions
        smoothed = smoothed * SMOOTH_FACTOR + classes * (1.0 - SMOOTH_FACTOR)
        selected = np.argmax(smoothed) # The selected class is the one with highest probability

        # Show the class probabilities and selected class
	summary = 'Class %d [%s]' % (selected, ' '.join('%02.0f%%' % (99 * p) for p in smoothed))
        stderr.write('\r' + summary)

        # Perform an action for the selected class. In this case, play a random sound from the sounds/ dir
        if selected == 0:
            random_sound.play_from('sounds/')
        else:
            random_sound.stop()

        # Show the image in a preview window so you can tell if you are in frame
        if show_ui:
	    pygame.display.set_caption(summary)
            surface = pygame.surfarray.make_surface(raw_frame)
            screen.blit(pygame.transform.smoothscale(surface, (512, 512)), (0,0))
            pygame.display.flip()

            for evt in pygame.event.get():
	            if evt.type == pygame.QUIT:
		        pygame.quit()
		        break


if __name__ == '__main__':
    main()


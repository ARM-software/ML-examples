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
""" A rough-and-ready program to interactively classify images into up to 5 categories.

Keyboard controls:
* 0-4: classify the current image as '0' through '4'
* 9: classify the next 10 images as '0' (useful if 0 is the 'normal' class with many frames)
* Escape: go back one image
* Space: toggle viewer brightness to make human classification easier (does not modify file)
* S: save changes by moving images to their new directories and quit
* Close the window without pressing S to exit without moving files.
"""
from sys import argv, exit
from os import system
from time import time
from glob import glob
import pygame

def main():
    if len(argv) != 2 or argv[1] == '--help':
        print('Usage: %s DIRECTORY\nClassify images in DIRECTORY and move into subdirs' % argv[0])
        exit(1)

    directory = argv[1]
    images = glob('%s/*.png' % directory)
    images.sort()

    pygame.display.init()
    screen = pygame.display.set_mode((512, 512))

    started = time()
    i = 0
    kind = {}
    lighten = False

    while True:
        i = min(i, len(images)-1)

        image = images[i]
        duration = time() - started
        mins_remaining = (len(images)-(i+1)) * (duration / (i+1)) / 60.0
        caption = "%4d / %4d - %.0f%%, %.0f mins remaining" % (i+1, len(images),
                                                               100*(i+1)/len(images),
                                                               mins_remaining)
        pygame.display.set_caption(caption)
        try:
            surface = pygame.image.load(image)

            if lighten:
                light = pygame.Surface((surface.get_width(), surface.get_height()),
                                       flags=pygame.SRCALPHA)
                light.fill((128, 128, 128, 0))
                surface.blit(light, (0, 0), special_flags=pygame.BLEND_RGBA_ADD)

            screen.blit(pygame.transform.scale(surface, (512, 512)), (0, 0))
            pygame.display.flip()
        except:
            pygame.display.set_caption("%4d / %4d - error reading %s" % (i+1, len(images), image))

        while True:
            evt = pygame.event.wait()
            if evt.type == pygame.QUIT:
                print('Exiting without moving images (use S to save and exit)')
                pygame.quit()
                exit(0)
            elif evt.type == pygame.KEYDOWN:
                if evt.key == pygame.K_9: # Not class 9, but rather 10 frames of class 1
                    for _ in range(10):
                        kind[image] = 0
                        i += 1
                        i = min(i, len(images)-1)
                        image = images[i]
                    break
                if evt.key == pygame.K_0:
                    kind[image] = 0
                    i += 1
                    break
                if evt.key == pygame.K_1:
                    kind[image] = 1
                    i += 1
                    break
                if evt.key == pygame.K_2:
                    kind[image] = 2
                    i += 1
                    break
                if evt.key == pygame.K_3:
                    kind[image] = 3
                    i += 1
                    break
                if evt.key == pygame.K_4:
                    kind[image] = 4
                    i += 1
                    break
                if evt.key == pygame.K_ESCAPE:
                    i -= 1
                    break
                if evt.key == pygame.K_SPACE:
                    lighten = not lighten
                    break
                if evt.key == pygame.K_s:
                    print('Moving %d images to subdirectories' % (len(kind)))
                    for k in kind.values():
                        system('mkdir -p %s/%s' % (directory, k))
                    for image, k in kind.items():
                        system('mv %s %s/%s/' % (image, directory, k))
                    print('Classified %d images in %.0f minutes' % (len(kind), duration/60))
                    pygame.quit()
                    exit(0)

if __name__ == '__main__':
    main()

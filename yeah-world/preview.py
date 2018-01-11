#!/usr/bin/python

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
Show a camera preview using the same resolution and FPS settings
as will be used for training, but without randomly adjusting
the exposure (training_mode=False). Enable this to see what it does.
"""

from sys import stderr
from time import time

import pygame
from camera import Camera


def main():
    camera = Camera(training_mode=False)
    pygame.display.init()
    pygame.display.set_caption("PiCamera preview")
    screen = pygame.display.set_mode((512, 512))

    while True:
        total = time()
        frame = camera.next_frame()
        surface = pygame.surfarray.make_surface(frame)
        screen.blit(pygame.transform.scale(surface, (512, 512)), (0, 0))
        pygame.display.flip()

        for evt in pygame.event.get():
            if evt.type == pygame.QUIT:
                pygame.quit()
                break

        total = time() - total
        stderr.write("\r%4.0f ms per frame, %2.1f FPS  " % (total*1000, 1.0/total))

if __name__ == '__main__':
    main()

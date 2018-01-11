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

from os.path import join
from glob import glob
from random import choice
from pygame import mixer
from time import sleep



class RandomSound:
    def __init__(self):
        mixer.init()
        self.playing = None

    def play_from(self, path):
        if mixer.music.get_busy() and self.playing == path: return

        filename = choice(glob(join(path, '*.wav')))
        mixer.music.load(filename)
        mixer.music.play()
        self.playing = path

    def wait(self):
        while mixer.music.get_busy():
            sleep(0.1)
    
    def stop(self):
        mixer.music.stop()



if __name__ == '__main__':
    from sys import argv
    rs = RandomSound()
    rs.play_from(argv[1])
    rs.wait()


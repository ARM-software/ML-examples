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
A simple wrapper around PiCamera that can randomly vary the exposure and
white balance during image capture as a form of data augmentation.
"""
from builtins import next
from builtins import object
from time import sleep
from random import choice, uniform

from picamera.array import PiRGBArray
from picamera import PiCamera



class Camera(object):
    """ A simple PiCamera wrapper. Setting training_mode will randomly vary
        exposure and white balance between captured images. The capture stream
        is truncated before capture to reduce recognition latency at the expense
        of frame rate. """
    def __init__(self, training_mode):
        self.training_mode = training_mode
        self.camera = PiCamera()
        self.camera.resolution = (128, 128)
        self.camera.framerate = 30
        self.capture = PiRGBArray(self.camera, size=self.camera.resolution)
        self.stream = self.camera.capture_continuous(self.capture,
                                                     format='rgb',
                                                     use_video_port=True)
        if training_mode:
            # We allow picamera's auto exposure settings to settle for 5 seconds then we
            # lock them so they don't change in response to actions as this can cause problems.
            # If waving your hands causes the camera to shift its exposure settings during training
            # as then the network will likely learn to detect the exposure setting not your hands!
            sleep(5)
            self.camera.shutter_speed = self.camera.exposure_speed
            self.camera.exposure_mode = 'off'
            self.base_awb = self.camera.awb_gains
            self.camera.awb_mode = 'off'


    def next_frame(self):
        """ Capture a frame from the camera. By truncating the frame buffer we
        exchange FPS for lower latency. When responding to gestures latency is
        more important to the user experience. """
        self.capture.truncate(0)
        if self.training_mode:
            self.camera.iso = choice([100, 200, 320, 400, 500, 640, 800])
            awb_r = max(0., uniform(-.5, .5) + self.base_awb[0])
            awb_b = max(0., uniform(-.5, .5) + self.base_awb[1])
            self.camera.awb_gains = (awb_r, awb_b)

        frame = next(self.stream).array
        return frame

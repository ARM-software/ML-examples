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
Record frames from the camera and save them to as PNG files.
"""
from time import time, sleep
from sys import argv, exit, stdout
from os import mkdir, path
import errno
from PIL import Image
from camera import Camera



def main():
    if len(argv) != 3:
        print("""Usage: %s DIRECTORY SECONDS
Record frames from the camera for SECONDS seconds and save them in DIRECTORY""" % argv[0])
        exit(1)
    name = argv[1]
    seconds = int(argv[2])

    # Initialize the camera and record video frames for a few seconds
    # We select a random exposure for each frame to generate a wide range
    # of lighting samples for training
    camera = Camera(training_mode=True)
    record(camera, name, seconds)


def status(text):
    """Helper function to show status updates"""
    stdout.write('\r%s' % text)
    stdout.flush()


def record(camera, dirname, seconds):
    """ Record from the camera """

    delay = 3 # Give people a 3 second warning to get ready
    started = time()
    while time() - started < delay:
        status("Recording in %.0f..." % max(0, delay - (time() - started)))
        sleep(0.1)

    try:
        mkdir(dirname)
    except OSError as err:
        if err.errno != errno.EEXIST:
            raise err

    num_frames = 0

    started = time()
    while time() - started < seconds or seconds == -1:
        frame = camera.next_frame()
        image = Image.fromarray(frame)
        filename = (path.join(dirname, '%05d.png') % num_frames)
        image.save(filename, "PNG")
        num_frames += 1

        # Update our progress
        if seconds != -1:
            status("Recording [ %d frames, %3.0fs left ]" % (num_frames,
                                                             max(0, seconds - (time() - started))))
        else:
            status("Recording [ %d frames, %3.0fs so far ]" % (num_frames,
                                                               max(0, (time() - started))))

    print('')

    # Save the frames to a file, appending if one already exists
    print('Wrote %d frames to %s\n' % (num_frames, dirname))


if __name__ == '__main__':
    main()

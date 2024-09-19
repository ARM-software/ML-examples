# Copyright (c) 2023-2024 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Python VSI Video Server module

try:
    import argparse
    import ipaddress
    import logging
    import os
    from multiprocessing.connection import Listener

    import cv2
    import numpy as np
except ImportError as err:
    print(f"VSI:Video:Server:ImportError: {err}")
except Exception as e:
    print(f"VSI:Video:Server:Exception: {type(e).__name__}")


## Set verbosity level
verbosity = logging.ERROR

# [debugging] Verbosity settings
level = { 10: "DEBUG",  20: "INFO",  30: "WARNING",  40: "ERROR" }
logging.basicConfig(format='VSI Server: [%(levelname)s]\t%(message)s', level = verbosity)
logging.info("Verbosity level is set to " + level[verbosity])

# Default Server configuration
default_address       = ('127.0.0.1', 6000)
default_authkey       = 'vsi_video'

# Supported file extensions
video_file_extensions = ('wmv', 'avi', 'mp4')
image_file_extensions = ('bmp', 'png', 'jpg')
video_fourcc          = {'wmv' : 'WMV1', 'avi' : 'MJPG', 'mp4' : 'mp4v'}

# Mode Input/Output
MODE_IO_Msk           = 1<<0
MODE_Input            = 0<<0
MODE_Output           = 1<<0

class VideoServer:
    def __init__(self, address, authkey):
        # Server commands
        self.SET_FILENAME     = 1
        self.STREAM_CONFIGURE = 2
        self.STREAM_ENABLE    = 3
        self.STREAM_DISABLE   = 4
        self.FRAME_READ       = 5
        self.FRAME_WRITE      = 6
        self.CLOSE_SERVER     = 7
        # Color space
        self.GRAYSCALE8       = 1
        self.RGB888           = 2
        self.BGR565           = 3
        self.YUV420           = 4
        self.NV12             = 5
        self.NV21             = 6
        # Variables
        self.listener         = Listener(address, authkey=authkey.encode('utf-8'))
        self.filename         = ""
        self.mode             = None
        self.active           = False
        self.video            = True
        self.stream           = None
        self.frame_ratio      = 0
        self.frame_drop       = 0
        self.frame_index      = 0
        self.eos              = False
        # Stream configuration
        self.resolution       = (None, None)
        self.color_format     = None
        self.frame_rate       = None

    # Set filename
    def _setFilename(self, base_dir, filename, mode):
        filename_valid = False

        if self.active:
            return filename_valid

        self.filename    = ""
        self.frame_index = 0

        file_extension = str(filename).split('.')[-1].lower()

        if file_extension in video_file_extensions:
            self.video = True
        else:
            self.video = False

        file_path = os.path.join(base_dir, filename)
        logging.debug(f"File path: {file_path}")

        if (mode & MODE_IO_Msk) == MODE_Input:
            self.mode = MODE_Input
            if os.path.isfile(file_path):
                if file_extension in (video_file_extensions + image_file_extensions):
                    self.filename  = file_path
                    filename_valid = True
        else:
            self.mode = MODE_Output
            if file_extension in (video_file_extensions + image_file_extensions):
                if os.path.isfile(file_path):
                    os.remove(file_path)
                self.filename  = file_path
                filename_valid = True

        return filename_valid

    # Configure video stream
    def _configureStream(self, frame_width, frame_height, color_format, frame_rate):
        if (frame_width == 0 or frame_height == 0 or frame_rate == 0):
            return False

        self.resolution   = (frame_width, frame_height)
        self.color_format = color_format
        self.frame_rate   = frame_rate

        return True

    # Enable video stream
    def _enableStream(self, mode):
        if self.active:
            return

        self.eos = False
        self.frame_ratio = 0
        self.frame_drop  = 0

        if self.stream is not None:
            self.stream.release()
            self.stream = None

        if self.filename == "":
            self.video = True
            if (mode & MODE_IO_Msk) == MODE_Input:
                # Device mode: camera
                self.mode = MODE_Input
            else:
                # Device mode: display
                self.mode = MODE_Output

        if self.video:
            if self.mode == MODE_Input:
                if self.filename == "":
                    self.stream = cv2.VideoCapture(0)
                    if not self.stream.isOpened():
                        logging.error("Failed to open Camera interface")
                        return
                else:
                    self.stream = cv2.VideoCapture(self.filename)
                    self.stream.set(cv2.CAP_PROP_POS_FRAMES, self.frame_index)
                    video_fps = self.stream.get(cv2.CAP_PROP_FPS)
                    if video_fps > self.frame_rate:
                        self.frame_ratio = video_fps / self.frame_rate
                        logging.debug(f"Frame ratio: {self.frame_ratio}")
            else:
                if self.filename != "":
                    extension = str(self.filename).split('.')[-1].lower()
                    fourcc = cv2.VideoWriter_fourcc(*f'{video_fourcc[extension]}')

                    if os.path.isfile(self.filename) and (self.frame_index != 0):
                        tmp_filename = f'{self.filename.rstrip(f".{extension}")}_tmp.{extension}'
                        os.rename(self.filename, tmp_filename)
                        cap    = cv2.VideoCapture(tmp_filename)
                        width  = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
                        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
                        self.resolution = (width, height)
                        self.frame_rate = cap.get(cv2.CAP_PROP_FPS)
                        self.stream = cv2.VideoWriter(self.filename, fourcc, self.frame_rate, self.resolution)

                        while cap.isOpened():
                            ret, frame = cap.read()
                            if not ret:
                                cap.release()
                                os.remove(tmp_filename)
                                break
                            self.stream.write(frame)
                            del frame

                    else:
                        self.stream = cv2.VideoWriter(self.filename, fourcc, self.frame_rate, self.resolution)

        self.active = True
        logging.info("Stream enabled")

    # Disable Video Server
    def _disableStream(self):
        self.active = False
        if self.stream is not None:
            if self.mode == MODE_Input:
                self.frame_index = self.stream.get(cv2.CAP_PROP_POS_FRAMES)
            self.stream.release()
            self.stream = None
        logging.info("Stream disabled")

    # Resize frame to requested resolution in pixels
    def __resizeFrame(self, frame, resolution):
        frame_h = frame.shape[0]
        frame_w = frame.shape[1]

        # Calculate requested aspect ratio (width/height):
        crop_aspect_ratio  = resolution[0] / resolution[1]

        if crop_aspect_ratio != (frame_w / frame_h):
            # Crop into image with resize aspect ratio
            crop_w = int(frame_h * crop_aspect_ratio)
            crop_h = int(frame_w / crop_aspect_ratio)

            if   crop_w > frame_w:
                # Crop top and bottom part of the image
                top    = (frame_h - crop_h) // 2
                bottom = top + crop_h
                frame  = frame[top : bottom, 0 : frame_w]
            elif crop_h > frame_h:
                # Crop left and right side of the image``
                left   = (frame_w - crop_w) // 2
                right  = left + crop_w
                frame  = frame[0 : frame_h, left : right]
            else:
                # Crop to the center of the image
                left   = (frame_w - crop_w) // 2
                right  = left + crop_w
                top    = (frame_h - crop_h) // 2
                bottom = top + crop_h
                frame  = frame[top : bottom, left : right]
            logging.debug(f"Frame cropped from ({frame_w}, {frame_h}) to ({frame.shape[1]}, {frame.shape[0]})")
        
        logging.debug(f"Resize frame from ({frame.shape[1]}, {frame.shape[0]}) to ({resolution[0]}, {resolution[1]})")
        try:
            frame = cv2.resize(frame, resolution)
        except Exception as e:
            logging.error(f"Error in resizeFrame(): {e}")

        return frame

    # Change color space of a frame from BGR to selected profile
    def __changeColorSpace(self, frame, color_space):
        color_format = None

        # Default OpenCV color profile: BGR
        if self.mode == MODE_Input:
            if   color_space == self.GRAYSCALE8:
                color_format = cv2.COLOR_BGR2GRAY
            elif color_space == self.RGB888:
                color_format = cv2.COLOR_BGR2RGB
            elif color_space == self.BGR565:
                color_format = cv2.COLOR_BGR2BGR565
            elif color_space == self.YUV420:
                color_format = cv2.COLOR_BGR2YUV_I420
            elif color_space == self.NV12:
                frame = self.__changeColorSpace(frame, self.YUV420)
                color_format = cv2.COLOR_YUV2RGB_NV12
            elif color_space == self.NV21:
                frame = self.__changeColorSpace(frame, self.YUV420)
                color_format = cv2.COLOR_YUV2RGB_NV21

        else:
            if   color_space == self.GRAYSCALE8:
                color_format = cv2.COLOR_GRAY2BGR
            elif color_space == self.RGB888:
                color_format = cv2.COLOR_RGB2BGR
            elif color_space == self.BGR565:
                color_format = cv2.COLOR_BGR5652BGR
            elif color_space == self.YUV420:
                color_format = cv2.COLOR_YUV2BGR_I420
            elif color_space == self.NV12:
                color_format = cv2.COLOR_YUV2BGR_I420
            elif color_space == self.NV21:
                color_format = cv2.COLOR_YUV2BGR_I420

        if color_format != None:
            logging.debug(f"Change color space to {color_format}")
            try:
                frame = cv2.cvtColor(frame, color_format)
            except Exception as e:
                logging.error(f"Error in changeColorSpace(): {e}")

        return frame

    # Read frame from source
    def _readFrame(self):
        frame = bytearray()

        if not self.active:
            return frame

        if self.eos:
            return frame

        if self.video:
            if self.frame_ratio > 1:
                _, tmp_frame = self.stream.read()
                self.frame_drop += (self.frame_ratio - 1)
                if self.frame_drop > 1:
                    logging.debug(f"Frames to drop: {self.frame_drop}")
                    drop = int(self.frame_drop // 1)
                    for i in range(drop):
                        _, _ = self.stream.read()
                    logging.debug(f"Frames dropped: {drop}")
                    self.frame_drop -= drop
                    logging.debug(f"Frames left to drop: {self.frame_drop}")
            else:
                _, tmp_frame = self.stream.read()
            if tmp_frame is None:
                self.eos = True
                logging.debug("End of stream.")
        else:
            tmp_frame = cv2.imread(self.filename)
            self.eos  = True
            logging.debug("End of stream.")

        if tmp_frame is not None:
            tmp_frame = self.__resizeFrame(tmp_frame, self.resolution)
            tmp_frame = self.__changeColorSpace(tmp_frame, self.color_format)
            frame = bytearray(tmp_frame.tobytes())

        return frame

    # Write frame to destination
    def _writeFrame(self, frame):
        if not self.active:
            return

        try:
            decoded_frame = np.frombuffer(frame, dtype=np.uint8)
            decoded_frame = decoded_frame.reshape((self.resolution[0], self.resolution[1], 3))
            bgr_frame = self.__changeColorSpace(decoded_frame, self.RGB888)

            if self.filename == "":
                cv2.imshow(self.filename, bgr_frame)
                cv2.waitKey(10)
            else:
                if self.video:
                    self.stream.write(np.uint8(bgr_frame))
                    self.frame_index += 1
                else:
                    cv2.imwrite(self.filename, bgr_frame)
        except Exception:
            pass

    # Run Video Server
    def run(self):
        logging.info("Video server started")

        try:
            conn = self.listener.accept()
            logging.info(f'Connection accepted {self.listener.address}')
        except Exception:
            logging.error("Connection not accepted")
            return

        while True:
            try:
                recv = conn.recv()
            except EOFError:
                return

            cmd     = recv[0]  # Command
            payload = recv[1:] # Payload

            if  cmd == self.SET_FILENAME:
                logging.info("Set filename called")
                filename_valid = self._setFilename(payload[0], payload[1], payload[2])
                conn.send(filename_valid)

            elif cmd == self.STREAM_CONFIGURE:
                logging.info("Stream configure called")
                configuration_valid = self._configureStream(payload[0], payload[1], payload[2], payload[3])
                conn.send(configuration_valid)

            elif cmd == self.STREAM_ENABLE:
                logging.info("Enable stream called")
                self._enableStream(payload[0])
                conn.send(self.active)

            elif cmd == self.STREAM_DISABLE:
                logging.info("Disable stream called")
                self._disableStream()
                conn.send(self.active)

            elif cmd == self.FRAME_READ:
                logging.info("Read frame called")
                frame = self._readFrame()
                conn.send_bytes(frame)
                conn.send(self.eos)

            elif cmd == self.FRAME_WRITE:
                logging.info("Write frame called")
                frame = conn.recv_bytes()
                self._writeFrame(frame)

            elif cmd == self.CLOSE_SERVER:
                logging.info("Close server connection")
                self.stop()

    # Stop Video Server
    def stop(self):
        self._disableStream()
        if (self.mode == MODE_Output) and (self.filename == ""):
            try:
                cv2.destroyAllWindows()
            except Exception:
                pass
        self.listener.close()
        logging.info("Video server stopped")


# Validate IP address
def ip(ip):
    try:
        _ = ipaddress.ip_address(ip)
        return ip
    except:
        raise argparse.ArgumentTypeError(f"Invalid IP address: {ip}!")

def parse_arguments():
    formatter = lambda prog: argparse.HelpFormatter(prog, max_help_position=41)
    parser = argparse.ArgumentParser(formatter_class=formatter, description="VSI Video Server")

    parser_optional = parser.add_argument_group("optional")
    parser_optional.add_argument("--ip", dest="ip",  metavar="<IP>",
                                        help=f"Server IP address (default: {default_address[0]})",
                                        type=ip, default=default_address[0])
    parser_optional.add_argument("--port", dest="port",  metavar="<TCP Port>",
                                        help=f"TCP port (default: {default_address[1]})",
                                        type=int, default=default_address[1])
    parser_optional.add_argument("--authkey", dest="authkey",  metavar="<Auth Key>",
                                        help=f"Authorization key (default: {default_authkey})",
                                        type=str, default=default_authkey)

    return parser.parse_args()

if __name__ == '__main__':
    args = parse_arguments()
    Server = VideoServer((args.ip, args.port), args.authkey)
    try:
        Server.run()
    except KeyboardInterrupt:
        Server.stop()

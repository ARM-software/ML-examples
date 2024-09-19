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

# Python VSI Video Client module

try:
    import time
    import atexit
    import logging
    import subprocess
    from multiprocessing.connection import Client, Connection
    from os import path, getcwd
    from os import name as os_name
except ImportError as err:
    print(f"VSI:Video:ImportError: {err}")
    raise
except Exception as e:
    print(f"VSI:Video:Exception: {type(e).__name__}")
    raise


class VideoClient:
    def __init__(self):
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
        self.conn = None

    def connectToServer(self, address, authkey):
        for _ in range(50):
            try:
                self.conn = Client(address, authkey=authkey.encode('utf-8'))
                if isinstance(self.conn, Connection):
                    break
                else:
                    self.conn = None
            except Exception:
                self.conn = None
            time.sleep(0.01)

    def setFilename(self, filename, mode):
        self.conn.send([self.SET_FILENAME, getcwd(), filename, mode])
        filename_valid = self.conn.recv()

        return filename_valid

    def configureStream(self, frame_width, frame_height, color_format, frame_rate):
        self.conn.send([self.STREAM_CONFIGURE, frame_width, frame_height, color_format, frame_rate])
        configuration_valid = self.conn.recv()

        return configuration_valid

    def enableStream(self, mode):
        self.conn.send([self.STREAM_ENABLE, mode])
        stream_active = self.conn.recv()

        return stream_active

    def disableStream(self):
        self.conn.send([self.STREAM_DISABLE])
        stream_active = self.conn.recv()

        return stream_active

    def readFrame(self):
        self.conn.send([self.FRAME_READ])
        data = self.conn.recv_bytes()
        eos  = self.conn.recv()

        return data, eos

    def writeFrame(self, data):
        self.conn.send([self.FRAME_WRITE])
        self.conn.send_bytes(data)

    def closeServer(self):
        try:
            if isinstance(self.conn, Connection):
                self.conn.send([self.CLOSE_SERVER])
                self.conn.close()
        except Exception as e:
            logging.error(f'Exception occurred on cleanup: {e}')


# User registers
REG_IDX_MAX               = 12  # Maximum user register index used in VSI
MODE                      = 0   # Regs[0]  // Mode: 0=Input, 1=Output
CONTROL                   = 0   # Regs[1]  // Control: enable, flush
STATUS                    = 0   # Regs[2]  // Status: active, buf_empty, buf_full, overflow, underflow, eos
FILENAME_LEN              = 0   # Regs[3]  // Filename length
FILENAME_CHAR             = 0   # Regs[4]  // Filename character
FILENAME_VALID            = 0   # Regs[5]  // Filename valid flag
FRAME_WIDTH               = 300 # Regs[6]  // Requested frame width
FRAME_HEIGHT              = 300 # Regs[7]  // Requested frame height
COLOR_FORMAT              = 0   # Regs[8]  // Color format
FRAME_RATE                = 0   # Regs[9]  // Frame rate
FRAME_INDEX               = 0   # Regs[10] // Frame index
FRAME_COUNT               = 0   # Regs[11] // Frame count
FRAME_COUNT_MAX           = 0   # Regs[12] // Frame count maximum

# MODE register definitions
MODE_IO_Msk               = 1<<0
MODE_Input                = 0<<0
MODE_Output               = 1<<0

# CONTROL register definitions
CONTROL_ENABLE_Msk        = 1<<0
CONTROL_CONTINUOS_Msk     = 1<<1
CONTROL_BUF_FLUSH_Msk     = 1<<2

# STATUS register definitions
STATUS_ACTIVE_Msk         = 1<<0
STATUS_BUF_EMPTY_Msk      = 1<<1
STATUS_BUF_FULL_Msk       = 1<<2
STATUS_OVERFLOW_Msk       = 1<<3
STATUS_UNDERFLOW_Msk      = 1<<4
STATUS_EOS_Msk            = 1<<5

# IRQ Status register definitions
IRQ_Status_FRAME_Msk      = 1<<0
IRQ_Status_OVERFLOW_Msk   = 1<<1
IRQ_Status_UNDERFLOW_Msk  = 1<<2
IRQ_Status_EOS_Msk        = 1<<3

# Variables
Video                     = VideoClient()
Filename                  = ""
FilenameIdx               = 0


# Close VSI Video Server on exit
def cleanup():
    Video.closeServer()


# Client connection to VSI Video Server
def init(address, authkey):
    global FILENAME_VALID

    base_dir = path.dirname(__file__)
    server_path = path.join(base_dir, 'vsi_video_server.py')

    logging.info("Start video server")
    if path.isfile(server_path):
        # Start Video Server
        if os_name == 'nt':
            py_cmd = 'python'
        else:
            py_cmd = 'python3.9'
        cmd = f"{py_cmd} {server_path} "\
              f"--ip {address[0]} "\
              f"--port {address[1]} "\
              f"--authkey {authkey}"
        subprocess.Popen(cmd, shell=True)
        # Connect to Video Server
        Video.connectToServer(address, authkey)
        if Video.conn == None:
            logging.error("Server not connected")

    else:
        logging.error(f"Server script not found: {server_path}")

    # Register clean-up function
    atexit.register(cleanup)


## Flush Stream buffer
def flushBuffer():
    global STATUS, FRAME_INDEX, FRAME_COUNT

    STATUS |=  STATUS_BUF_EMPTY_Msk
    STATUS &= ~STATUS_BUF_FULL_Msk

    FRAME_INDEX = 0
    FRAME_COUNT = 0


## VSI IRQ Status register
#  @param IRQ_Status IRQ status register to update
#  @param value status bits to clear
#  @return IRQ_Status return updated register
def wrIRQ(IRQ_Status, value):
    IRQ_Status_Clear = IRQ_Status & ~value
    IRQ_Status &= ~IRQ_Status_Clear

    return IRQ_Status


## Timer Event
#  @param IRQ_Status IRQ status register to update
#  @return IRQ_Status return updated register
def timerEvent(IRQ_Status):

    IRQ_Status |= IRQ_Status_FRAME_Msk

    if (STATUS & STATUS_OVERFLOW_Msk) != 0:
        IRQ_Status |= IRQ_Status_OVERFLOW_Msk

    if (STATUS & STATUS_UNDERFLOW_Msk) != 0:
        IRQ_Status |= IRQ_Status_UNDERFLOW_Msk

    if (STATUS & STATUS_EOS_Msk) != 0:
        IRQ_Status |= IRQ_Status_EOS_Msk

    if (CONTROL & CONTROL_CONTINUOS_Msk) == 0:
        wrCONTROL(CONTROL & ~(CONTROL_ENABLE_Msk | CONTROL_CONTINUOS_Msk))

    return IRQ_Status


## Read data from peripheral for DMA P2M transfer (VSI DMA)
#  @param size size of data to read (in bytes, multiple of 4)
#  @return data data read (bytearray)
def rdDataDMA(size):
    global STATUS, FRAME_COUNT

    if (STATUS & STATUS_ACTIVE_Msk) != 0:

        if Video.conn != None:
            data, eos = Video.readFrame()
            if eos:
                STATUS |= STATUS_EOS_Msk
            if FRAME_COUNT < FRAME_COUNT_MAX:
                FRAME_COUNT += 1
            else:
                STATUS |= STATUS_OVERFLOW_Msk
            if FRAME_COUNT == FRAME_COUNT_MAX:
                STATUS |= STATUS_BUF_FULL_Msk
            STATUS &= ~STATUS_BUF_EMPTY_Msk
        else:
            data = bytearray()

    else:
        data = bytearray()

    return data


## Write data to peripheral for DMA M2P transfer (VSI DMA)
#  @param data data to write (bytearray)
#  @param size size of data to write (in bytes, multiple of 4)
def wrDataDMA(data, size):
    global STATUS, FRAME_COUNT

    if (STATUS & STATUS_ACTIVE_Msk) != 0:

        if Video.conn != None:
            Video.writeFrame(data)
            if FRAME_COUNT > 0:
                FRAME_COUNT -= 1
            else:
                STATUS |= STATUS_UNDERFLOW_Msk
            if FRAME_COUNT == 0:
                STATUS |= STATUS_BUF_EMPTY_Msk
            STATUS &= ~STATUS_BUF_FULL_Msk


## Write CONTROL register (user register)
#  @param value value to write (32-bit)
def wrCONTROL(value):
    global CONTROL, STATUS

    if ((value ^ CONTROL) & CONTROL_ENABLE_Msk) != 0:
        STATUS &= ~STATUS_ACTIVE_Msk
        if (value & CONTROL_ENABLE_Msk) != 0:
            logging.info("Start video stream")
            if Video.conn != None:
                logging.info("Configure video stream")
                configuration_valid = Video.configureStream(FRAME_WIDTH, FRAME_HEIGHT, COLOR_FORMAT, FRAME_RATE)
                if configuration_valid:
                    logging.info("Enable video stream")
                    server_active = Video.enableStream(MODE)
                    if server_active:
                        STATUS |=   STATUS_ACTIVE_Msk
                        STATUS &= ~(STATUS_OVERFLOW_Msk | STATUS_UNDERFLOW_Msk | STATUS_EOS_Msk)
                    else:
                        logging.error("Enable video stream failed")
                else:
                    logging.error("Configure video stream failed")
            else:
                logging.error("Server not connected")
        else:
            logging.info("Stop video stream")
            if Video.conn != None:
                logging.info("Disable video stream")
                Video.disableStream()
            else:
                logging.error("Server not connected")

    if (value & CONTROL_BUF_FLUSH_Msk) != 0:
        value &= ~CONTROL_BUF_FLUSH_Msk
        flushBuffer()

    CONTROL = value


## Read STATUS register (user register)
# @return status current STATUS User register (32-bit)
def rdSTATUS():
    global STATUS

    status = STATUS
    STATUS &= ~(STATUS_OVERFLOW_Msk | STATUS_UNDERFLOW_Msk | STATUS_EOS_Msk)

    return status


## Write FILENAME_LEN register (user register)
#  @param value value to write (32-bit)
def wrFILENAME_LEN(value):
    global STATUS, FILENAME_LEN, FILENAME_VALID, Filename, FilenameIdx

    logging.info("Set new source name length and reset filename and valid flag")
    FilenameIdx = 0
    Filename = ""
    FILENAME_VALID = 0
    FILENAME_LEN = value


## Write FILENAME_CHAR register (user register)
#  @param value value to write (32-bit)
def wrFILENAME_CHAR(value):
    global FILENAME_VALID, Filename, FilenameIdx

    if FilenameIdx < FILENAME_LEN:
        logging.info(f"Append {value} to filename")
        Filename += f"{value}"
        FilenameIdx += 1
        logging.debug(f"Received {FilenameIdx} of {FILENAME_LEN} characters")

    if FilenameIdx == FILENAME_LEN:
        logging.info("Check if file exists on Server side and set VALID flag")
        logging.debug(f"Filename: {Filename}")

        if Video.conn != None:
            FILENAME_VALID = Video.setFilename(Filename, MODE)
        else:
            logging.error("Server not connected")

        logging.debug(f"Filename VALID: {FILENAME_VALID}")


## Write FRAME_INDEX register (user register)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrFRAME_INDEX(value):
    global STATUS, FRAME_INDEX, FRAME_COUNT

    FRAME_INDEX += 1
    if FRAME_INDEX == FRAME_COUNT_MAX:
        FRAME_INDEX = 0

    if (MODE & MODE_IO_Msk) == MODE_Input:
        # Input
        if FRAME_COUNT > 0:
            FRAME_COUNT -= 1
        if FRAME_COUNT == 0:
            STATUS |= STATUS_BUF_EMPTY_Msk
        STATUS &= ~STATUS_BUF_FULL_Msk
    else:
        # Output
        if FRAME_COUNT < FRAME_COUNT_MAX:
            FRAME_COUNT += 1
        if FRAME_COUNT == FRAME_COUNT_MAX:
            STATUS |= STATUS_BUF_FULL_Msk
        STATUS &= ~STATUS_BUF_EMPTY_Msk

    return FRAME_INDEX


## Read user registers (the VSI User Registers)
#  @param index user register index (zero based)
#  @return value value read (32-bit)
def rdRegs(index):
    value = 0

    if   index == 0:
        value = MODE
    elif index == 1:
        value = CONTROL
    elif index == 2:
        value = rdSTATUS()
    elif index == 3:
        value = FILENAME_LEN
    elif index == 4:
        value = FILENAME_CHAR
    elif index == 5:
        value = FILENAME_VALID
    elif index == 6:
        value = FRAME_WIDTH
    elif index == 7:
        value = FRAME_HEIGHT
    elif index == 8:
        value = COLOR_FORMAT
    elif index == 9:
        value = FRAME_RATE
    elif index == 10:
        value = FRAME_INDEX
    elif index == 11:
        value = FRAME_COUNT
    elif index == 12:
        value = FRAME_COUNT_MAX

    return value


## Write user registers (the VSI User Registers)
#  @param index user register index (zero based)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrRegs(index, value):
    global MODE, FRAME_WIDTH, FRAME_HEIGHT, COLOR_FORMAT, FRAME_RATE, FRAME_COUNT_MAX

    if   index == 0:
        MODE = value
    elif index == 1:
        wrCONTROL(value)
    elif index == 2:
        value = STATUS
    elif index == 3:
        wrFILENAME_LEN(value)
    elif index == 4:
        wrFILENAME_CHAR(chr(value))
    elif index == 5:
        value = FILENAME_VALID
    elif index == 6:
        if value != 0:
            FRAME_WIDTH = value
    elif index == 7:
        if value != 0:
            FRAME_HEIGHT = value
    elif index == 8:
        COLOR_FORMAT = value
    elif index == 9:
        FRAME_RATE = value
    elif index == 10:
        value = wrFRAME_INDEX(value)
    elif index == 11:
        value = FRAME_COUNT
    elif index == 12:
        FRAME_COUNT_MAX = value
        flushBuffer()

    return value

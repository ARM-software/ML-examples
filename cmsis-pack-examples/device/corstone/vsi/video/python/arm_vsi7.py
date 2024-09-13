# Copyright (c) 2021-2023 Arm Limited. All rights reserved.

# Virtual Streaming Interface instance 7 Python script

##@addtogroup arm_vsi_py_video
#  @{
#
##@package arm_vsi7_video
#Documentation for VSI peripherals module.
#
#More details.

import logging
import vsi_video

## Set verbosity level
#verbosity = logging.DEBUG
verbosity = logging.ERROR

# [debugging] Verbosity settings
level = { 10: "DEBUG",  20: "INFO",  30: "WARNING",  40: "ERROR" }
logging.basicConfig(format='Py: VSI7: [%(levelname)s]\t%(message)s', level = verbosity)
logging.info("Verbosity level is set to " + level[verbosity])


# Video Server configuration
server_address = ('127.0.0.1', 6003)
server_authkey = 'vsi_video'


# IRQ registers
IRQ_Status = 0

# Timer registers
Timer_Control  = 0
Timer_Interval = 0

# Timer Control register definitions
Timer_Control_Run_Msk      = 1<<0
Timer_Control_Periodic_Msk = 1<<1
Timer_Control_Trig_IRQ_Msk = 1<<2
Timer_Control_Trig_DMA_Msk = 1<<3

# DMA registers
DMA_Control = 0

# DMA Control register definitions
DMA_Control_Enable_Msk    = 1<<0
DMA_Control_Direction_Msk = 1<<1
DMA_Control_Direction_P2M = 0<<1
DMA_Control_Direction_M2P = 1<<1

# User registers
Regs = [0] * 64

# Data buffer
Data = bytearray()


## Initialize
#  @return None
def init():
    logging.info("Python function init() called")
    vsi_video.init(server_address, server_authkey)


## Read interrupt request (the VSI IRQ Status Register)
#  @return value value read (32-bit)
def rdIRQ():
    global IRQ_Status
    logging.info("Python function rdIRQ() called")

    value = IRQ_Status
    logging.debug("Read interrupt request: {}".format(value))

    return value


## Write interrupt request (the VSI IRQ Status Register)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrIRQ(value):
    global IRQ_Status
    logging.info("Python function wrIRQ() called")

    value = vsi_video.wrIRQ(IRQ_Status, value)
    IRQ_Status = value
    logging.debug("Write interrupt request: {}".format(value))

    return value


## Write Timer registers (the VSI Timer Registers)
#  @param index Timer register index (zero based)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrTimer(index, value):
    global Timer_Control, Timer_Interval
    logging.info("Python function wrTimer() called")

    if   index == 0:
        Timer_Control = value
        logging.debug("Write Timer_Control: {}".format(value))
    elif index == 1:
        Timer_Interval = value
        logging.debug("Write Timer_Interval: {}".format(value))

    return value


## Timer event (called at Timer Overflow)
#  @return None
def timerEvent():
    global IRQ_Status

    logging.info("Python function timerEvent() called")

    IRQ_Status = vsi_video.timerEvent(IRQ_Status)


## Write DMA registers (the VSI DMA Registers)
#  @param index DMA register index (zero based)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrDMA(index, value):
    global DMA_Control
    logging.info("Python function wrDMA() called")

    if   index == 0:
        DMA_Control = value
        logging.debug("Write DMA_Control: {}".format(value))

    return value


## Read data from peripheral for DMA P2M transfer (VSI DMA)
#  @param size size of data to read (in bytes, multiple of 4)
#  @return data data read (bytearray)
def rdDataDMA(size):
    global Data
    logging.info("Python function rdDataDMA() called")

    Data = vsi_video.rdDataDMA(size)

    n = min(len(Data), size)
    data = bytearray(size)
    data[0:n] = Data[0:n]
    logging.debug("Read data ({} bytes)".format(size))

    return data


## Write data to peripheral for DMA M2P transfer (VSI DMA)
#  @param data data to write (bytearray)
#  @param size size of data to write (in bytes, multiple of 4)
#  @return None
def wrDataDMA(data, size):
    global Data
    logging.info("Python function wrDataDMA() called")

    Data = data
    logging.debug("Write data ({} bytes)".format(size))

    vsi_video.wrDataDMA(data, size)

    return


## Read user registers (the VSI User Registers)
#  @param index user register index (zero based)
#  @return value value read (32-bit)
def rdRegs(index):
    global Regs
    logging.info("Python function rdRegs() called")

    if index <= vsi_video.REG_IDX_MAX:
        Regs[index] = vsi_video.rdRegs(index)

    value = Regs[index]
    logging.debug("Read user register at index {}: {}".format(index, value))

    return value


## Write user registers (the VSI User Registers)
#  @param index user register index (zero based)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrRegs(index, value):
    global Regs
    logging.info("Python function wrRegs() called")

    if index <= vsi_video.REG_IDX_MAX:
        value = vsi_video.wrRegs(index, value)

    Regs[index] = value
    logging.debug("Write user register at index {}: {}".format(index, value))

    return value


## @}


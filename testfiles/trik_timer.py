__author__ = 'Rostislav Varzar'

import trik_protocol

# Timer address
timer1 = 0x2E

# Timer registers
atctl = 0x00
atper = 0x01
atval = 0x02

# ATCTL bits
at_en = 0x0003

# Enable timer
def timer_enable():
    trik_protocol.write_16bit_reg(timer1, atctl, at_en)

# Disable timer
def timer_disable():
    trik_protocol.write_16bit_reg(timer1, atctl, 0x0000)
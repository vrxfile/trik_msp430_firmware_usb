__author__ = 'Rostislav Varzar'

import termios, fcntl, sys, os
import trik_protocol, trik_motor, trik_encoder, trik_timer
import trik_stty, trik_power
import time

# Defines for menu pages
motor_menu = 0x00
motor_enchanced_menu = 0x01

motnum = trik_motor.motor1
pwmper = 0x0000
pwmdut = 0x0000
motangle = 0x00000000
mottime = 0x00000000
moterr = 0x00000000
motfb = 0x00000000
encval = 0x00000000
motctl = 0x0000


# Init async key press input without press <ENTER>
def init_key_press():
    global fd
    global oldterm
    global oldflags
    fd = sys.stdin.fileno()
    oldterm = termios.tcgetattr(fd)
    newattr = termios.tcgetattr(fd)
    newattr[3] = newattr[3] & ~termios.ICANON & ~termios.ECHO
    termios.tcsetattr(fd, termios.TCSANOW, newattr)
    oldflags = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, oldflags | os.O_NONBLOCK)

# Print text of menu
def print_menu(menu_page):
    if menu_page == motor_menu:
        print_there(0, 1, "MOTORs MENU")
        print_there(0, 2, "Select menu item:")
        print_there(0, 4, "<1/2> Select motor")
        print_there(0, 5, "<3/4> Set PWM period")
        print_there(0, 6, "<5/6> Set PWM duty")
        print_there(0, 7, "<7>   Start motor")
        print_there(0, 8, "<8>   Reverse motor")
        print_there(0, 9, "<9>   Brake motor")
        print_there(0, 10, "<0>   Stop motor")
        print_there(0, 11, "<Q/W> Set angle")
        print_there(0, 12, "<E/R> Set time")
        print_there(0, 13, "<A>   Rotate angle")
        print_there(0, 14, "<S>   Reverse angle")
        print_there(0, 15, "<T>   Rotate time")
        print_there(0, 16, "<Y>   Reverse time")
        print_there(0, 17, "<C>   Clear screen")
        print_there(0, 18, "<TAB> Change device group")
        print_there(0, 19, "<ESC> Exit/Quit")
        print_there(0, 21, "Control register")
        print_there(0, 22, "Feed-back value")
        print_there(0, 23, "Encoder value")
        print_there(0, 24, "Overcurrent errors")

# Print register values
def print_registers(menu_page):
    global pwmper
    global pwmdut
    global motangle
    global mottime
    global motctl
    global motfb
    global encval
    global moterr
    if menu_page == motor_menu:
        print_there(25, 4, "0x%02X     " % motnum)
        print_there(25, 5, "0x%04X     " % pwmper)
        print_there(25, 6, "0x%04X     " % pwmdut)
        print_there(25, 11, "0x%08X     " % motangle)
        print_there(25, 12, "0x%08X     " % mottime)
        print_there(25, 21, "0x%04X     " % motctl)
        print_there(25, 22, "0x%08X     " % motfb)
        print_there(25, 23, "0x%08X     " % encval)
        print_there(25, 24, "0x%08X     " % moterr)

# Print text in certain coordinates
def print_there(x, y, text):
     sys.stdout.write("\x1b7\x1b[%d;%df%s\x1b8" % (y + 1, x + 1, text))
     sys.stdout.flush()

# Init async key press
init_key_press()

# Init Serial TTY device
trik_stty.init_stty()

# Init 12 V power in ARM controller
trik_power.init_power()

# Read all registers of motor
def read_all_data():
    global pwmper
    global pwmdut
    global motangle
    global mottime
    global motctl
    global motfb
    global encval
    global moterr
    pwmper = trik_protocol.get_reg_value(trik_motor.get_motor_period(motnum))
    pwmdut = trik_protocol.get_reg_value(trik_motor.get_motor_duty(motnum))
    motangle = trik_protocol.get_reg_value(trik_motor.get_motor_angle(motnum))
    mottime = trik_protocol.get_reg_value(trik_motor.get_motor_time(motnum))
    motctl = trik_protocol.get_reg_value(trik_motor.get_motor_control(motnum))
    moterr = trik_protocol.get_reg_value(trik_motor.get_motor_overcurrent(motnum))
    motfb = trik_protocol.get_reg_value(trik_motor.get_motor_feedback(motnum))
    encval =  trik_protocol.get_reg_value(trik_encoder.read_encoder(motnum + trik_encoder.encoder1))

# Read all registers of motor
read_all_data()

# Clear screen
os.system("clear")

# Print menu
menu_pg = motor_menu
print_menu(menu_pg)
print_registers(menu_pg)

# Main cycle
try:
    while 1:
        try:
            c = sys.stdin.read(1)
            if c == "1":
                motnum = motnum - 1
                if motnum <= trik_motor.motor1:
                    motnum = trik_motor.motor1
            if c == "2":
                motnum = motnum + 1
                if motnum >= trik_motor.motor4:
                    motnum = trik_motor.motor4
            if c == "3":
                if pwmper > pwmdut:
                    pwmper = pwmper - 100
                    if pwmper <= 0:
                        pwmper = 0
                else:
                    pwmper = pwmdut
                trik_motor.set_motor_period(motnum, pwmper)
            if c == "4":
                pwmper = pwmper + 100
                if pwmper >= 0xFFFF:
                    pwmper = 0xFFFF
                trik_motor.set_motor_period(motnum, pwmper)
            if c == "5":
                pwmdut = pwmdut - 100
                if pwmdut <= 0:
                    pwmdut = 0
                trik_motor.set_motor_duty(motnum, pwmdut)
            if c == "6":
                pwmdut = pwmdut + 100
                if pwmdut >= pwmper:
                    pwmdut = pwmper
                trik_motor.set_motor_duty(motnum, pwmdut)
            if c == "7":
                trik_motor.start_motor(motnum)
            if c == "8":
                trik_motor.reverse_motor(motnum)
            if c == "9":
                trik_motor.brake_motor(motnum)
            if c == "0":
                trik_motor.stop_motor(motnum)
            if c.upper() == "Q":
                motangle = motangle - 10
                if motangle <= 0x00000000:
                    motangle = 0x00000000
                trik_motor.set_motor_angle(motnum, motangle)
            if c.upper() == "W":
                motangle = motangle + 10
                if motangle >= 0xFFFFFFFF:
                    motangle = 0xFFFFFFFF
                trik_motor.set_motor_angle(motnum, motangle)
            if c.upper() == "E":
                mottime = mottime - 10
                if mottime <= 0x00000000:
                    mottime = 0x00000000
                trik_motor.set_motor_time(motnum, mottime)
            if c.upper() == "R":
                mottime = mottime + 10
                if mottime >= 0xFFFFFFFF:
                    mottime = 0xFFFFFFFF
                trik_motor.set_motor_time(motnum, mottime)
            if c.upper() == "A":
                trik_encoder.enable_encoder(motnum + trik_encoder.encoder1, 2, 1, 0)
                trik_motor.rotate_motor_angle(motnum)
            if c.upper() == "S":
                trik_encoder.enable_encoder(motnum + trik_encoder.encoder1, 2, 1, 0)
                trik_motor.reverse_motor_angle(motnum)
            if c.upper() == "T":
                trik_timer.timer_enable()
                trik_motor.rotate_motor_time(motnum)
            if c.upper() == "Y":
                trik_timer.timer_enable()
                trik_motor.reverse_motor_time(motnum)
            if c.upper() == "C":
                os.system("clear")







            read_all_data()
            print_registers(menu_pg)

            if c == chr(0x1B):
                break
        except IOError: pass
finally:
    termios.tcsetattr(fd, termios.TCSAFLUSH, oldterm)
    fcntl.fcntl(fd, fcntl.F_SETFL, oldflags)

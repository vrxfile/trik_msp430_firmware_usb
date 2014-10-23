/*
 * Trik_Motor.c
 *
 *  Created on: 21 ���. 2014 �.
 *      Author: Rostislav Varzar
 */

#include "trik_motor.h"



//API functions
void MOTOR_enableController(uint8_t MOT_NUMBER)
{
    //Busy table
    if (busy_table[MOT_NUMBER]==NNONE)
    {
        busy_table[MOT_NUMBER]=MOT_NUMBER;
        MOT[MOT_NUMBER].MOT_EN = 1;
        //Configure GPIO ports
        switch (MOT_NUMBER)
        {
            case MOTOR1:
                GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN4 | GPIO_PIN5);
                GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN4 | GPIO_PIN5);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2);
                GPIO_setAsInputPin(GPIO_PORT_P1, GPIO_PIN1);
                break;
            case MOTOR2:
                GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN0);
                GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
                GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN6);
                break;
            case MOTOR3:
                GPIO_setOutputLowOnPin(GPIO_PORT_PJ, GPIO_PIN0 | GPIO_PIN1);
                GPIO_setAsOutputPin(GPIO_PORT_PJ, GPIO_PIN0 | GPIO_PIN1);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);
                GPIO_setAsInputPin(GPIO_PORT_P1, GPIO_PIN7);
                break;
            case MOTOR4:
                GPIO_setOutputLowOnPin(GPIO_PORT_PJ, GPIO_PIN2 | GPIO_PIN3);
                GPIO_setAsOutputPin(GPIO_PORT_PJ, GPIO_PIN2 | GPIO_PIN3);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);
                GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN7);
                break;
            default:;
        }
    }
}

void MOTOR_disableController(uint8_t MOT_NUMBER)
{
    busy_table[MOT_NUMBER]=NNONE;
    MOT[MOT_NUMBER].MOT_EN = 0;
    //Configure GPIO ports
           switch (MOT_NUMBER)
           {
               case MOTOR1:
                   GPIO_setAsInputPin(GPIO_PORT_P5, GPIO_PIN4 | GPIO_PIN5);
                   GPIO_setAsInputPin(GPIO_PORT_P1, GPIO_PIN2);
                   break;
               case MOTOR2:
                   GPIO_setAsInputPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);
                   GPIO_setAsInputPin(GPIO_PORT_P1, GPIO_PIN3);
                   break;
               case MOTOR3:
                   GPIO_setAsInputPin(GPIO_PORT_PJ, GPIO_PIN0 | GPIO_PIN1);
                   GPIO_setAsInputPin(GPIO_PORT_P1, GPIO_PIN4);
                   break;
               case MOTOR4:
                   GPIO_setAsInputPin(GPIO_PORT_PJ, GPIO_PIN2 | GPIO_PIN3);
                   GPIO_setAsInputPin(GPIO_PORT_P1, GPIO_PIN5);
                   break;
               default:;
           }
}

void MOTOR_rotationForward(uint8_t MOT_NUMBER)
{
    MOT[MOT_NUMBER].MOT_DIR = 0;
    switch (MOT_NUMBER)
               {
                   case MOTOR1:
                       GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN4);
                       GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN5);
                       break;
                   case MOTOR2:
                       GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN0);
                       GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN1);
                       break;
                   case MOTOR3:
                       GPIO_setOutputHighOnPin(GPIO_PORT_PJ, GPIO_PIN0);
                       GPIO_setOutputLowOnPin(GPIO_PORT_PJ, GPIO_PIN1);
                       break;
                   case MOTOR4:
                       GPIO_setOutputHighOnPin(GPIO_PORT_PJ, GPIO_PIN2);
                       GPIO_setOutputLowOnPin(GPIO_PORT_PJ, GPIO_PIN3);
                       break;
                   default:;
               }
}

void MOTOR_rotationBackward(uint8_t MOT_NUMBER)
{
    MOT[MOT_NUMBER].MOT_DIR = 1;
    switch (MOT_NUMBER)
               {
                   case MOTOR1:
                       GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN5);
                       GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN4);
                       break;
                   case MOTOR2:
                       GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN1);
                       GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0);
                       break;
                   case MOTOR3:
                       GPIO_setOutputHighOnPin(GPIO_PORT_PJ, GPIO_PIN1);
                       GPIO_setOutputLowOnPin(GPIO_PORT_PJ, GPIO_PIN0);
                       break;
                   case MOTOR4:
                       GPIO_setOutputHighOnPin(GPIO_PORT_PJ, GPIO_PIN3);
                       GPIO_setOutputLowOnPin(GPIO_PORT_PJ, GPIO_PIN2);
                       break;
                   default:;
               }
}

void MOTOR_enablePWM(uint8_t MOT_NUMBER)
{
    MOT[MOT_NUMBER].MOT_PWM = 1;
    switch (MOT_NUMBER)
               {
                   case MOTOR1:
                       GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN2);
                       break;
                   case MOTOR2:
                       GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN3);
                       break;
                   case MOTOR3:
                       GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN4);
                       break;
                   case MOTOR4:
                       GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN5);
                       break;
                   default:;
               }
}

void MOTOR_disablePWM(uint8_t MOT_NUMBER)
{
    MOT[MOT_NUMBER].MOT_PWM = 0;
    switch (MOT_NUMBER)
               {
                   case MOTOR1:
                       GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN2);
                       break;
                   case MOTOR2:
                       GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN3);
                       break;
                   case MOTOR3:
                       GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN4);
                       break;
                   case MOTOR4:
                       GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN5);
                       break;
                   default:;
               }
}

//���� ��� ����
void MOTOR_start(uint8_t MOT_NUMBER)
{
    MOT[MOT_NUMBER].MOT_PWR = 1;
    switch (MOT_NUMBER)
               {
                   case MOTOR1:
                       if (!(MOT[MOT_NUMBER].MOT_PWM))
                           GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN2);
                       else
                           TIMER_A_generatePWM(TIMER_A0_BASE,
                                   TIMER_A_CLOCKSOURCE_EXTERNAL_TXCLK,
                                   TIMER_A_CLOCKSOURCE_DIVIDER_1,
                                   10,
                                   TIMER_A_CAPTURECOMPARE_REGISTER_0,
                                   TIMER_A_OUTPUTMODE_SET_RESET,
                                   10);
                       break;
                   case MOTOR2:
                       if (!(MOT[MOT_NUMBER].MOT_PWM))
                           GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3);
                       break;
                   case MOTOR3:
                       if (!(MOT[MOT_NUMBER].MOT_PWM))
                           GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN4);
                       break;
                   case MOTOR4:
                       if (!(MOT[MOT_NUMBER].MOT_PWM))
                           GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN5);
                       break;
                   default:;
               }
}

//���� ��� �������
void MOTOR_stop(uint8_t MOT_NUMBER)
{
    MOT[MOT_NUMBER].MOT_PWR = 0;
    switch (MOT_NUMBER)
               {
                   case MOTOR1:
                       if (!(MOT[MOT_NUMBER].MOT_PWM))
                           GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2);
                       break;
                   case MOTOR2:
                       if (!(MOT[MOT_NUMBER].MOT_PWM))
                           GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
                       break;
                   case MOTOR3:
                       if (!(MOT[MOT_NUMBER].MOT_PWM))
                           GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);
                       break;
                   case MOTOR4:
                       if (!(MOT[MOT_NUMBER].MOT_PWM))
                           GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);
                       break;
                   default:;
               }
}


//Handler
uint8_t MOTOR_hadler(uint8_t MOT_NUMBER)
{
    if (MOT[MOT_NUMBER].MCTL & 0x8000)
    {
        //Enable/disable
        if (!(MOT[MOT_NUMBER].MOT_EN)) MOTOR_enableController(MOT_NUMBER);
        //Forward/backward
        if (MOT[MOT_NUMBER].MCTL & 0x0010)
            if (!(MOT[MOT_NUMBER].MOT_DIR)) MOTOR_rotationBackward(MOT_NUMBER);
        else
            if (MOT[MOT_NUMBER].MOT_DIR) MOTOR_rotationForward(MOT_NUMBER);
        //PWM on/PWM off(0/100%)
        if (MOT[MOT_NUMBER].MCTL & 0x0004)
            if (!(MOT[MOT_NUMBER].MOT_PWM)) MOTOR_enablePWM(MOT_NUMBER);
        else
            if (MOT[MOT_NUMBER].MOT_PWM) MOTOR_disablePWM(MOT_NUMBER);

        //Start/stop
        if (MOT[MOT_NUMBER].MCTL & 0x0003)
            if (!(MOT[MOT_NUMBER].MOT_PWR)) MOTOR_start(MOT_NUMBER);
        else
            if (MOT[MOT_NUMBER].MOT_PWR) MOTOR_stop(MOT_NUMBER);

        //return MOT[MOT_NUMBER].MOT_EN+MOT[MOT_NUMBER].MOT_DIR+MOT[MOT_NUMBER].MOT_PWM+MOT[MOT_NUMBER].MOT_PWR;
        //return 0x55;
    }
    else
    {
        if (MOT[MOT_NUMBER].MOT_EN) MOTOR_disableController(MOT_NUMBER);
        //return 0x00;
    }

return (char)((MOT[MOT_NUMBER].MCTL >> 8)&0xFF);
}

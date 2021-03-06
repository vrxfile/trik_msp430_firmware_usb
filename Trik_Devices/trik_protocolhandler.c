/*
 * Trik_ProtocolHandler.c
 *
 *  Created on: October 21, 2014
 *      Author: Rostislav Varzar
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <msp430f5510.h>
#include <stdint.h>
#include "trik_hardpwm.h"
#include "trik_protocolhandler.h"
#include "trik_motor.h"
#include "trik_sensor.h"
#include "trik_dhtxx.h"
#include "trik_hcsr04.h"
#include "trik_encoder.h"
#include "trik_bsl.h"
#include "trik_devices.h"
#include "trik_async.h"
#include "trik_touch.h"
#include "trik_port.h"
#include "trik_softi2c.h"
#include "trik_softpwm.h"
#include "trik_sc16is7x0.h"
#include "trik_version.h"

uint8_t TO_HEX(uint8_t i)
{
    return (i <= 9 ? '0' + i : 'A' - 10 + i);
}

void char2hex(char *string, uint8_t number)
{
    string[0] = TO_HEX((number & 0x00F0) >> 4);
    string[1] = TO_HEX(number & 0x000F);
    string[2] = '\0';
}

uint32_t hex2num(char *string, uint16_t pos, uint16_t numsize)
{
    uint32_t resnum = 0;
    uint32_t tmpnum = 0;
    char c = 0;
    for (uint16_t i = 0; i < numsize; i++)
    {
        c = toupper(string[pos+i]);
        tmpnum = c > '9' ? c - 'A' + 10 : c - '0';
        resnum |= (tmpnum << ((numsize - i - 1) * 4));
    }
    return resnum;
}

/// Read register response
void PROTOCOL_recvResponse(char *r_str, uint8_t dev_addr, uint8_t resp_code, uint8_t reg_addr, uint32_t reg_val)
{
    char stmp1[MAX_STRING_LENGTH]; //Temp string
    uint8_t t11,t12,t13,t14; //Temp vars
    uint8_t crc; //Checksum

    t11 = (uint8_t)((reg_val & 0xFF000000) >> 24);
    t12 = (uint8_t)((reg_val & 0x00FF0000) >> 16);
    t13 = (uint8_t)((reg_val & 0x0000FF00) >> 8);
    t14 = (uint8_t)(reg_val & 0x000000FF);

    r_str[0] = ':';
    r_str[1] = '\0';

    char2hex(stmp1,dev_addr);
    strcat(r_str,stmp1);

    char2hex(stmp1,resp_code);
    strcat(r_str,stmp1);

    char2hex(stmp1,reg_addr);
    strcat(r_str,stmp1);

    char2hex(stmp1,t11);
    strcat(r_str,stmp1);
    char2hex(stmp1,t12);
    strcat(r_str,stmp1);
    char2hex(stmp1,t13);
    strcat(r_str,stmp1);
    char2hex(stmp1,t14);
    strcat(r_str,stmp1);

    crc = 0 - (dev_addr + resp_code + reg_addr + t11 + t12 + t13 + t14);
    char2hex(stmp1,crc);
    strcat(r_str,stmp1);

    strcat(r_str,"\n\0");
}

/// This function returns true if there's an 0x0D character in the string; and if
/// so, it trims the 0x0D and anything that had followed it.
uint8_t retInString (char* string)
{
    uint8_t retPos = 0,i,len;
    char tempStr[MAX_STRING_LENGTH] = "";

    strncpy(tempStr,string,strlen(string));  // Make a copy of the string
    len = strlen(tempStr);

    // Find 0x0D; if not found, retPos ends up at len
    while ((tempStr[retPos] != 0x0A) && (tempStr[retPos] != 0x0D) &&
           (retPos++ < len)) ;

    // If 0x0A was actually found...
    if ((retPos < len) && (tempStr[retPos] == 0x0A)){

        // Empty the buffer
        for (i = 0; i < MAX_STRING_LENGTH; i++){
            string[i] = 0x00;
        }

        //...trim the input string to just before 0x0D
        strncpy(string,tempStr,retPos);

        //...and tell the calling function that we did so
        return (True) ;

    } else if (tempStr[retPos] == 0x0A) {

        // Empty the buffer
        for (i = 0; i < MAX_STRING_LENGTH; i++){
            string[i] = 0x00;
        }

        //...trim the input string to just before 0x0D
        strncpy(string,tempStr,retPos);

        //...and tell the calling function that we did so
        return (True) ;
    } else if (retPos < len){
        // Empty the buffer
        for (i = 0; i < MAX_STRING_LENGTH; i++){
            string[i] = 0x00;
        }

        //...trim the input string to just before 0x0D
        strncpy(string,tempStr,retPos);

        //...and tell the calling function that we did so
        return (True) ;
    }

    return (False) ; // Otherwise, it wasn't found
}

/// Protocol handler
uint8_t PROTOCOL_handler(char *in_str, char *out_str)
{
	uint8_t devaddr1 = 0;   //Device address
	uint8_t func1 = 0;      //Function number
	uint8_t regaddr1 = 0;   //Register address
	uint32_t regval1 = 0;   //Register value
	uint8_t crc1 = 0;       //Cheksum
	uint8_t crc2 = 0;       //Calculated checksum

	// Find errirs in received packet

	// Start condition error
	if (in_str[0]!=':')
	{
        PROTOCOL_recvResponse(out_str,0x00,0x80,0x00,START_ERROR);
		return START_ERROR;
	}

	// Incorrect packet length
	if ((strlen(in_str)!=9) && (strlen(in_str)!=17))
	{
		PROTOCOL_recvResponse(out_str,0x00,0x80,0x00,LENGTH_ERROR);
		return LENGTH_ERROR;
	}

	// Get device address
	devaddr1 = hex2num(in_str, 1, NUM_BYTE);

	// Get function
	func1 = hex2num(in_str, 3, NUM_BYTE);

	// Get register address
	regaddr1 = hex2num(in_str, 5, NUM_BYTE);

	// Get register value
    regval1 = hex2num(in_str, 7, NUM_DWORD);

    // Device addresses range
	if ((devaddr1>MAX_DEVICES) && (devaddr1!=BSL))
	{
	    PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,DEV_ADDR_ERROR);
		return DEV_ADDR_ERROR;
	}

	// Motor registers addresses range
	// if (((devaddr1>=MOTOR1) && (devaddr1<=MOTOR4)) && (regaddr1>0x06))
	if (((devaddr1<=MOTOR4)) && (regaddr1>0x07))
	{
	    PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_ADDR_ERROR);
		return REG_ADDR_ERROR;
	}

	// Sensor registers addresses range
	if (((devaddr1>=SENSOR1) && (devaddr1<=SENSOR18)) && (regaddr1>0x03))
	{
	    PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_ADDR_ERROR);
		return REG_ADDR_ERROR;
	}

	// Encoder registers addresses range
	if (((devaddr1>=ENCODER1) && (devaddr1<=ENCODER4)) && (regaddr1>0x02))
	{
	    PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_ADDR_ERROR);
	    return REG_ADDR_ERROR;
	}

    // Port registers addresses range
    if (((devaddr1>=PORT1) && (devaddr1<=PORTJ)) && (regaddr1>0x09))
    {
        PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_ADDR_ERROR);
        return REG_ADDR_ERROR;
    }

    // Hardware PWM registers addresses range
    if (((devaddr1>=HPWM1) && (devaddr1<=HPWM4)) && (regaddr1>0x03))
    {
        PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_ADDR_ERROR);
        return REG_ADDR_ERROR;
    }

    // Software PWM registers addresses range
    if (((devaddr1>=SPWM1) && (devaddr1<=SPWM14)) && (regaddr1>0x03))
    {
        PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_ADDR_ERROR);
        return REG_ADDR_ERROR;
    }

    // I2C registers addresses range
    if (((devaddr1>=I2C1) && (devaddr1<=I2C7)) && (regaddr1>0x09))
    {
        PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_ADDR_ERROR);
        return REG_ADDR_ERROR;
    }

    // USART registers addresses range
    if (((devaddr1>=USART1) && (devaddr1<=USART7)) && (regaddr1>0x03))
    {
        PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_ADDR_ERROR);
        return REG_ADDR_ERROR;
    }

    // Timer registers addresses range
    if ((devaddr1==ASYNCTIMER) && (regaddr1>0x03))
    {
        PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_ADDR_ERROR);
        return REG_ADDR_ERROR;
    }

    // Touch controller registers addresses range
    if ((devaddr1==TOUCHDEVICE) && (regaddr1>0x09))
    {
        PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_ADDR_ERROR);
        return REG_ADDR_ERROR;
    }

    // BSL registers addresses range
    if ((devaddr1==BSL) && (regaddr1>0x01))
    {
        PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_ADDR_ERROR);
        return REG_ADDR_ERROR;
    }

    // Version control registers addresses range
    if ((devaddr1==VERSIONCTRL) && (regaddr1>0x0C))
    {
        PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_ADDR_ERROR);
        return REG_ADDR_ERROR;
    }

	// Function number check
	if ((func1!=FUNC_WRITE) && (func1!=FUNC_READ))
	{
	    PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,FUNC_CODE_ERROR);
		return FUNC_CODE_ERROR;
	}

	// CRC check
	switch (func1)
	{
        case FUNC_WRITE:
            crc1 = hex2num(in_str, 15, NUM_BYTE);
            break;
        case FUNC_READ:
            crc1 = hex2num(in_str, 7, NUM_BYTE);
            break;
        default:
            break;
	}

	if ((func1==FUNC_WRITE))
	    crc2=0-(devaddr1+func1+regaddr1+
	            (uint8_t)(regval1 & 0x000000FF)+(uint8_t)((regval1 & 0x0000FF00) >> 8)+
	            (uint8_t)((regval1 & 0x00FF0000) >> 16)+(uint8_t)((regval1 & 0xFF000000) >> 24));
	if ((func1==FUNC_READ))
	    crc2=0-(devaddr1+func1+regaddr1);
	if (crc1!=crc2)
	{
	    PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,CRC_ERROR);
	    return CRC_ERROR;
	}

	// Hadle of function 0x03 - write single register
	if ((func1==FUNC_WRITE) && (strlen(in_str)==17))
	{
	    // Motors
	    // if ((devaddr1>=MOTOR1) && (devaddr1<=MOTOR4))
	    if ((devaddr1<=MOTOR4))
	    {
	        if (regaddr1==MMDUT)
	            MOT[devaddr1].MDUT = regval1;
	        if (regaddr1==MMPER)
	            MOT[devaddr1].MPER = regval1;
	        if (regaddr1==MMANG)
	            MOT[devaddr1].MANG = regval1;
	        if (regaddr1==MMTMR)
	            MOT[devaddr1].MTMR = regval1;
	        if (regaddr1==MMVAL)
	            MOT[devaddr1].MVAL = regval1;
            if (regaddr1==MMERR)
                MOT[devaddr1].MERR = regval1;
	        if (regaddr1==MMCTL)
	            MOT[devaddr1].MCTL = regval1;
            // Error register values
            if ((MOT[devaddr1].MPER==0) || (MOT[devaddr1].MDUT>MOT[devaddr1].MPER))
            {
                PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,REG_INC_ERROR);
                return REG_INC_ERROR;
            }
            // Enable (start) / disable (stop)
            if (MOT[devaddr1].MCTL & MOT_ENABLE)
            {
                MOTOR_start(devaddr1);
            }
            else
            {
                MOTOR_disable(devaddr1);
            }
            PROTOCOL_recvResponse(out_str,devaddr1,func1,regaddr1,MOT[devaddr1].MSTA);
            return NO_ERROR;
	    }

	    // Encoders
        if ((devaddr1>=ENCODER1) && (devaddr1<=ENCODER4))
        {
            if (regaddr1==EEVAL)
                ENC[devaddr1-ENCODER1].EVAL = regval1;
            if (regaddr1==EECTL)
            {
                ENC[devaddr1-ENCODER1].ECTL = regval1;
                if (ENC[devaddr1-ENCODER1].ECTL & ENC_ENABLE)
                {
                    ENCODER_enableController(devaddr1);
                }
                else
                {
                    ENCODER_disableController(devaddr1);
                }
            }
            PROTOCOL_recvResponse(out_str,devaddr1,func1,regaddr1,ENC[devaddr1-ENCODER1].ESTA);
            return NO_ERROR;
        }

        // Sensors
        if ((devaddr1>=SENSOR1) && (devaddr1<=SENSOR18))
        {
            if (regaddr1==SSIDX)
                SENS[devaddr1-SENSOR1].SIDX = regval1;
            if (regaddr1==SSCTL)
            {
                SENS[devaddr1-SENSOR1].SCTL = regval1;
                if (SENS[devaddr1-SENSOR1].SCTL & SENS_ENABLE)
                {
                	SENSOR_enableController(devaddr1);
                }
                else
                {
                	SENSOR_disableController(devaddr1);
                }
            }
            PROTOCOL_recvResponse(out_str,devaddr1,func1,regaddr1,SENS[devaddr1-SENSOR1].SSTA);
            return NO_ERROR;
        }

        // Ports
        if ((devaddr1>=PORT1) && (devaddr1<=PORTJ))
        {
            PORT_write(devaddr1, regaddr1, regval1);
            PROTOCOL_recvResponse(out_str,devaddr1,func1,regaddr1,NO_ERROR);
            return NO_ERROR;
        }

        // Hardware PWMs
        if ((devaddr1>=HPWM1) && (devaddr1<=HPWM4))
        {
            if (regaddr1==HPPDUT)
                HPWM[devaddr1-HPWM1].HPDUT = regval1;
            if (regaddr1==HPPPER)
                HPWM[devaddr1-HPWM1].HPPER = regval1;
            if (regaddr1==HPPCTL)
                HPWM[devaddr1-HPWM1].HPCTL = regval1;
            // Enable (start) / disable (stop)
            if (HPWM[devaddr1-HPWM1].HPCTL & HPWM_ENABLE)
            {
                HPWM_enable(devaddr1);
            }
            else
            {
                HPWM_disable(devaddr1);
            }
            PROTOCOL_recvResponse(out_str,devaddr1,func1,regaddr1,NO_ERROR);
            return NO_ERROR;
        }

        // Software PWMs
        if ((devaddr1>=SPWM1) && (devaddr1<=SPWM14))
        {
            if (regaddr1==SPPDUT)
                SPWM[devaddr1-SPWM1].SPDUT = regval1;
            if (regaddr1==SPPPER)
                SPWM[devaddr1-SPWM1].SPPER = regval1;
            if (regaddr1==SPPCTL)
                SPWM[devaddr1-SPWM1].SPCTL = regval1;
            SPWM_handler(devaddr1);
            PROTOCOL_recvResponse(out_str,devaddr1,func1,regaddr1,NO_ERROR);
            return NO_ERROR;
        }

        // Software I2Cs
        if ((devaddr1>=I2C1) && (devaddr1<=I2C7))
        {
            if (regaddr1==IIDEV)
                I2C[devaddr1-I2C1].IDEV = regval1;
            if (regaddr1==IIREG)
                I2C[devaddr1-I2C1].IREG = regval1;
            if (regaddr1==IIDAT)
                I2C[devaddr1-I2C1].IDAT = regval1;
            if (regaddr1==IIERR)
                I2C[devaddr1-I2C1].IERR = regval1;
            if (regaddr1==IIIDX)
                I2C[devaddr1-I2C1].IIDX = regval1;
            if (regaddr1==IIVAL)
                I2C[devaddr1-I2C1].IVAL = regval1;
            if (regaddr1==IIPAR)
                I2C[devaddr1-I2C1].IPAR = regval1;
            if (regaddr1==IIDEL)
                Idelay = regval1;
            if (regaddr1==IICTL)
            {
                I2C[devaddr1-I2C1].ICTL = regval1;
                I2C_handler(devaddr1);
            }
            PROTOCOL_recvResponse(out_str,devaddr1,func1,regaddr1,NO_ERROR);
            return NO_ERROR;
        }

        // USARTs
        if ((devaddr1>=USART1) && (devaddr1<=USART7))
        {
            if (regaddr1==UUCTL)
            {
                USART[devaddr1-USART1].UCTL = regval1;
                if (regval1 & USART_RST)
                {
                	I2C_init(devaddr1 - USART1 + I2C1);
                	USART_reset(devaddr1);
                	I2C_disable(devaddr1 - USART1 + I2C1);
                }
                if (regval1 & USART_EN)
                {
                	I2C_init(devaddr1 - USART1 + I2C1);
                	USART_init(devaddr1, regval1);
                }
                else
                {
                	I2C_disable(devaddr1 - USART1 + I2C1);
                	USART_disable(devaddr1);
                }
            }
            if (regaddr1==UUSPD)
            {
            	USART[devaddr1-USART1].USPD = regval1;
            	I2C_init(devaddr1 - USART1 + I2C1);
            	USART_set_speed(devaddr1, regval1);
            }
            if (regaddr1==UUDAT)
            {
            	USART[devaddr1-USART1].UDAT = regval1;
            	USART_transmit_byte(devaddr1, regval1);
            }
            PROTOCOL_recvResponse(out_str,devaddr1,func1,regaddr1,NO_ERROR);
            return NO_ERROR;
        }

        // Async timer
        if ((devaddr1==ASYNCTIMER))
        {
            if (regaddr1==AATPER)
                ASYNCTMR.ATPER = regval1 + MAX_DEVICES;
            if (regaddr1==AATVAL)
                ASYNCTMR.ATVAL = regval1;
            if (regaddr1==AATCTL)
            {
                ASYNCTMR.ATCTL=regval1;
                if (ASYNCTMR.ATCTL & AT_EN)
                {
                	enableTimer_B();
                }
                else
                {
                	disableTimer_B();
                }
            }
            PROTOCOL_recvResponse(out_str,devaddr1,func1,regaddr1,NO_ERROR);
            return NO_ERROR;
        }

        // Touch controller
        if ((devaddr1==TOUCHDEVICE))
        {
            if (regaddr1==TTMOD)
            {
                TOUCH.TMOD = regval1;
                if (regval1) resetTouch();
            }
            if (regaddr1==TMINX)
                TOUCH.MINX = regval1;
            if (regaddr1==TMAXX)
                TOUCH.MAXX = regval1;
            if (regaddr1==TMINY)
                TOUCH.MINY = regval1;
            if (regaddr1==TMAXY)
                TOUCH.MAXY = regval1;
            if (regaddr1==TSCRX)
                TOUCH.SCRX = regval1;
            if (regaddr1==TSCRY)
                TOUCH.SCRY = regval1;
            if (regaddr1==TCURX)
                TOUCH.CURX = regval1;
            if (regaddr1==TCURY)
                TOUCH.CURY = regval1;
            PROTOCOL_recvResponse(out_str,devaddr1,func1,regaddr1,NO_ERROR);
            return NO_ERROR;
        }

	    // BSL
	    if ((devaddr1==BSL) && (regaddr1==0x00))
	    {
	        PROTOCOL_recvResponse(out_str,devaddr1,func1,regaddr1,BSL_enterBSL(regval1));
	        return NO_ERROR;
	    }

	    // If not found any devices
	    PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,DEV_ADDR_ERROR);
	    return DEV_ADDR_ERROR;
	}

	// Function 0x05 - read single register
    if ((func1==FUNC_READ) && (strlen(in_str)==9))
    {
        // Motors
        // if ((devaddr1>=MOTOR1) && (devaddr1<=MOTOR4))
        if ((devaddr1<=MOTOR4))
        {
            if (regaddr1==MMCTL)
                PROTOCOL_recvResponse(out_str,devaddr1,MOT[devaddr1].MSTA,regaddr1,MOT[devaddr1].MCTL);
            if (regaddr1==MMDUT)
                PROTOCOL_recvResponse(out_str,devaddr1,MOT[devaddr1].MSTA,regaddr1,MOT[devaddr1].MDUT);
            if (regaddr1==MMPER)
                PROTOCOL_recvResponse(out_str,devaddr1,MOT[devaddr1].MSTA,regaddr1,MOT[devaddr1].MPER);
            if (regaddr1==MMANG)
                PROTOCOL_recvResponse(out_str,devaddr1,MOT[devaddr1].MSTA,regaddr1,MOT[devaddr1].MANG);
            if (regaddr1==MMTMR)
                PROTOCOL_recvResponse(out_str,devaddr1,MOT[devaddr1].MSTA,regaddr1,MOT[devaddr1].MTMR);
            if (regaddr1==MMVAL)
                PROTOCOL_recvResponse(out_str,devaddr1,MOT[devaddr1].MSTA,regaddr1,MOT[devaddr1].MVAL);
            if (regaddr1==MMERR)
                PROTOCOL_recvResponse(out_str,devaddr1,MOT[devaddr1].MSTA,regaddr1,MOT[devaddr1].MERR);
            if (regaddr1==MMVER)
                PROTOCOL_recvResponse(out_str,devaddr1,MOT[devaddr1].MSTA,regaddr1,MOTOR_VERSION);
            return NO_ERROR;
        }

        // Encoders
        if ((devaddr1>=ENCODER1) && (devaddr1<=ENCODER4))
        {
            if (regaddr1==EECTL)
                PROTOCOL_recvResponse(out_str,devaddr1,ENC[devaddr1-ENCODER1].ESTA,regaddr1,ENC[devaddr1-ENCODER1].ECTL);
            if (regaddr1==EEVAL)
                PROTOCOL_recvResponse(out_str,devaddr1,ENC[devaddr1-ENCODER1].ESTA,regaddr1,ENC[devaddr1-ENCODER1].EVAL);
            if (regaddr1==EEVER)
                PROTOCOL_recvResponse(out_str,devaddr1,ENC[devaddr1-ENCODER1].ESTA,regaddr1,ENCODER_VERSION);
            return NO_ERROR;
        }

        // Sensors
        if ((devaddr1>=SENSOR1) && (devaddr1<=SENSOR18))
        {
            if (regaddr1==SSCTL)
                PROTOCOL_recvResponse(out_str,devaddr1,SENS[devaddr1-SENSOR1].SSTA,regaddr1,SENS[devaddr1-SENSOR1].SCTL);
            if (regaddr1==SSIDX)
                PROTOCOL_recvResponse(out_str,devaddr1,SENS[devaddr1-SENSOR1].SSTA,regaddr1,SENS[devaddr1-SENSOR1].SIDX);
            if (regaddr1==SSVAL)
            {
                if ((SENS[devaddr1-SENSOR1].SCTL & SENS_ENABLE) && (SENS[devaddr1-SENSOR1].SCTL & SENS_READ))
                {
                    switch (SENS[devaddr1-SENSOR1].SIDX)
                    {
                    	case DIGITAL_INP:
                    		SENS[devaddr1-SENSOR1].SVAL=SENSOR_read_digital(devaddr1);
                    		break;
                    	case ANALOG_INP:
                    		SENS[devaddr1-SENSOR1].SVAL=SENSOR_read_analog(devaddr1);
                    		break;
                    	case DHTXX_TEMP:
                        	SENS[devaddr1-SENSOR1].SVAL=DHT_getTemp(devaddr1);
                    		break;
                    	case DHTXX_HUM:
                        	SENS[devaddr1-SENSOR1].SVAL=DHT_getHum(devaddr1);
                    		break;
                    	case HCSR_DIST:
                        	SENS[devaddr1-SENSOR1].SVAL=HCSR04_get_time_us(devaddr1);
                    		break;
                        default:
                        	SENS[devaddr1-SENSOR1].SVAL=0xFFFFFFFF;
                            break;
                    }
                	PROTOCOL_recvResponse(out_str,devaddr1,SENS[devaddr1-SENSOR1].SSTA,regaddr1,SENS[devaddr1-SENSOR1].SVAL);
                }
                else
                {
                	PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,DEV_EN_ERROR);
                }

            }
            if (regaddr1==SSVER)
                PROTOCOL_recvResponse(out_str,devaddr1,SENS[devaddr1-SENSOR1].SSTA,regaddr1,SENSOR_VERSION);
            return NO_ERROR;
        }

        // Hardware PWMs
        if ((devaddr1>=HPWM1) && (devaddr1<=HPWM4))
        {
            if (regaddr1==HPPCTL)
                PROTOCOL_recvResponse(out_str,devaddr1,HPWM[devaddr1-HPWM1].HPSTA,regaddr1,HPWM[devaddr1-HPWM1].HPCTL);
            if (regaddr1==HPPDUT)
                PROTOCOL_recvResponse(out_str,devaddr1,HPWM[devaddr1-HPWM1].HPSTA,regaddr1,HPWM[devaddr1-HPWM1].HPDUT);
            if (regaddr1==HPPPER)
                PROTOCOL_recvResponse(out_str,devaddr1,HPWM[devaddr1-HPWM1].HPSTA,regaddr1,HPWM[devaddr1-HPWM1].HPPER);
            if (regaddr1==HPPVER)
                PROTOCOL_recvResponse(out_str,devaddr1,HPWM[devaddr1-HPWM1].HPSTA,regaddr1,HPWM_VERSION);
            return NO_ERROR;
        }

        // USARTs
        if ((devaddr1>=USART1) && (devaddr1<=USART7))
        {
            if (regaddr1==UUCTL)
                PROTOCOL_recvResponse(out_str,devaddr1,USART[devaddr1-USART1].USTA,regaddr1,USART[devaddr1-USART1].UCTL);
            if (regaddr1==UUSPD)
                PROTOCOL_recvResponse(out_str,devaddr1,USART[devaddr1-USART1].USTA,regaddr1,USART[devaddr1-USART1].USPD);
            // need status function !!!!!!!!!!!!!!
            if (regaddr1==UUSTA)
                PROTOCOL_recvResponse(out_str,devaddr1,USART[devaddr1-USART1].USTA,regaddr1,USART[devaddr1-USART1].USTA);
            if (regaddr1==UUDAT)
            {
            	if (regval1 & USART_EN)
            	{
            		if (USART_is_data_in_buffer(regval1))
					{
            			PROTOCOL_recvResponse(out_str,devaddr1,USART[devaddr1-USART1].USTA,regaddr1,
            					USART_receive_byte(devaddr1));
					}
            		else
            		{
            			PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,BUFFER_ERROR);
            		}
            	}
            	else
            	{
            		PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,DEV_EN_ERROR);
            	}
            }
            return NO_ERROR;
        }

        // Software PWMs
        if ((devaddr1>=SPWM1) && (devaddr1<=SPWM14))
        {
            if (regaddr1==SPPCTL)
                PROTOCOL_recvResponse(out_str,devaddr1,SPWM[devaddr1-SPWM1].SPSTA,regaddr1,SPWM[devaddr1-SPWM1].SPCTL);
            if (regaddr1==SPPDUT)
                PROTOCOL_recvResponse(out_str,devaddr1,SPWM[devaddr1-SPWM1].SPSTA,regaddr1,SPWM[devaddr1-SPWM1].SPDUT);
            if (regaddr1==SPPPER)
                PROTOCOL_recvResponse(out_str,devaddr1,SPWM[devaddr1-SPWM1].SPSTA,regaddr1,SPWM[devaddr1-SPWM1].SPPER);
            if (regaddr1==SPPVER)
                PROTOCOL_recvResponse(out_str,devaddr1,SPWM[devaddr1-SPWM1].SPSTA,regaddr1,SPWM_VERSION);
            return NO_ERROR;
        }

        // Software I2Cs
        if ((devaddr1>=I2C1) && (devaddr1<=I2C7))
        {
            I2C_handler(devaddr1);
            if (regaddr1==IICTL)
                PROTOCOL_recvResponse(out_str,devaddr1,I2C[devaddr1-I2C1].ISTA,regaddr1,I2C[devaddr1-I2C1].ICTL);
            if (regaddr1==IIDEV)
                PROTOCOL_recvResponse(out_str,devaddr1,I2C[devaddr1-I2C1].ISTA,regaddr1,I2C[devaddr1-I2C1].IDEV);
            if (regaddr1==IIREG)
                PROTOCOL_recvResponse(out_str,devaddr1,I2C[devaddr1-I2C1].ISTA,regaddr1,I2C[devaddr1-I2C1].IREG);
            if (regaddr1==IIDAT)
                PROTOCOL_recvResponse(out_str,devaddr1,I2C[devaddr1-I2C1].ISTA,regaddr1,I2C[devaddr1-I2C1].IDAT);
            if (regaddr1==IIERR)
                PROTOCOL_recvResponse(out_str,devaddr1,I2C[devaddr1-I2C1].ISTA,regaddr1,I2C[devaddr1-I2C1].IERR);
            if (regaddr1==IIIDX)
                PROTOCOL_recvResponse(out_str,devaddr1,I2C[devaddr1-I2C1].ISTA,regaddr1,I2C[devaddr1-I2C1].IIDX);
            if (regaddr1==IIVAL)
                PROTOCOL_recvResponse(out_str,devaddr1,I2C[devaddr1-I2C1].ISTA,regaddr1,I2C[devaddr1-I2C1].IVAL);
            if (regaddr1==IIPAR)
                PROTOCOL_recvResponse(out_str,devaddr1,I2C[devaddr1-I2C1].ISTA,regaddr1,I2C[devaddr1-I2C1].IPAR);
            if (regaddr1==IIDEL)
                PROTOCOL_recvResponse(out_str,devaddr1,I2C[devaddr1-I2C1].ISTA,regaddr1,Idelay);
            if (regaddr1==IIVER)
                PROTOCOL_recvResponse(out_str,devaddr1,I2C[devaddr1-I2C1].ISTA,regaddr1,I2C_VERSION);
            return NO_ERROR;
        }

        // Ports
        if ((devaddr1>=PORT1) && (devaddr1<=PORTJ))
        {
            if (regaddr1==PVER)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,PORT_VERSION);
            else
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,PORT_read(devaddr1,regaddr1));
            return NO_ERROR;
        }

        // Async timer
        if ((devaddr1==ASYNCTIMER))
        {
            if (regaddr1==AATCTL)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,ASYNCTMR.ATCTL);
            if (regaddr1==AATPER)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,ASYNCTMR.ATPER);
            if (regaddr1==AATVAL)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,ASYNCTMR.ATVAL);
            if (regaddr1==AATVER)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,ATIMER_VERSION);
            return NO_ERROR;
        }

        // Touch controller
        if ((devaddr1==TOUCHDEVICE))
        {
            if (regaddr1==TTMOD)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,TOUCH.TMOD);
            if (regaddr1==TMINX)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,TOUCH.MINX);
            if (regaddr1==TMAXX)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,TOUCH.MAXX);
            if (regaddr1==TMINY)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,TOUCH.MINY);
            if (regaddr1==TMAXY)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,TOUCH.MAXY);
            if (regaddr1==TSCRX)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,TOUCH.SCRX);
            if (regaddr1==TSCRY)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,TOUCH.SCRY);
            if (regaddr1==TCURX)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,TOUCH.CURX);
            if (regaddr1==TCURY)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,TOUCH.CURY);
            if (regaddr1==TCVER)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,TOUCH_VERSION);
            return NO_ERROR;
        }

        // BSL
        if ((devaddr1==BSL))
        {
            if (regaddr1==BSLVER)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,BSL_VERSION);
            return NO_ERROR;
        }

        // Main control version module
        if ((devaddr1==VERSIONCTRL))
        {
            if (regaddr1==MAIN_VER_REG_1)
                strcpy(out_str,MAIN_VERSION_TXT);
            if (regaddr1==MAIN_VER_REG_2)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,MAIN_VERSION);
            if (regaddr1==MOT_VER_REG)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,MOTOR_VERSION);
            if (regaddr1==SENS_VER_REG)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,SENSOR_VERSION);
            if (regaddr1==ENC_VER_REG)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,ENCODER_VERSION);
            if (regaddr1==PORT_VER_REG)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,PORT_VERSION);
            if (regaddr1==PWM_VER_REG)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,HPWM_VERSION);
            if (regaddr1==SPWM_VER_REG)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,SPWM_VERSION);
            if (regaddr1==ATMR_VER_REG)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,ATIMER_VERSION);
            if (regaddr1==TOUCH_VER_REG)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,TOUCH_VERSION);
            if (regaddr1==I2C_VER_REG)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,I2C_VERSION);
            if (regaddr1==USART_VER_REG)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,USART_VERSION);
            if (regaddr1==BSL_VER_REG)
                PROTOCOL_recvResponse(out_str,devaddr1,NO_ERROR,regaddr1,BSL_VERSION);
            return NO_ERROR;
        }

    }

    PROTOCOL_recvResponse(out_str,devaddr1,func1+0x80,regaddr1,LENGTH_ERROR);
    return LENGTH_ERROR;
}


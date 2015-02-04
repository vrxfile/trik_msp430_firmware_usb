/*
 * trik_pmm.c
 *
 *  Created on: Fabruary 4, 2015
 *      Author: Rostislav Varzar
 */

#include <assert.h>
#include <stdint.h>
#include <msp430f5510.h>
#include "trik_pmm.h"
#include "trik_regaccess.h"

uint16_t _PMM_setVCoreUp( uint8_t level)
{
        uint32_t PMMRIE_backup, SVSMHCTL_backup, SVSMLCTL_backup;

        //The code flow for increasing the Vcore has been altered to work around
        //the erratum FLASH37.
        //Please refer to the Errata sheet to know if a specific device is affected
        //DO NOT ALTER THIS FUNCTION

        //Open PMM registers for write access
        HWREG8(PMM_BASE + OFS_PMMCTL0_H) = 0xA5;

        //Disable dedicated Interrupts
        //Backup all registers
        PMMRIE_backup = HWREG16(PMM_BASE + OFS_PMMRIE);
        HWREG16(PMM_BASE + OFS_PMMRIE) &= ~(SVMHVLRPE | SVSHPE | SVMLVLRPE |
                                            SVSLPE | SVMHVLRIE | SVMHIE |
                                            SVSMHDLYIE | SVMLVLRIE | SVMLIE |
                                            SVSMLDLYIE
                                            );
        SVSMHCTL_backup = HWREG16(PMM_BASE + OFS_SVSMHCTL);
        SVSMLCTL_backup = HWREG16(PMM_BASE + OFS_SVSMLCTL);

        //Clear flags
        HWREG16(PMM_BASE + OFS_PMMIFG) = 0;

        //Set SVM highside to new level and check if a VCore increase is possible
        HWREG16(PMM_BASE + OFS_SVSMHCTL) = SVMHE | SVSHE | (SVSMHRRL0 * level);

        //Wait until SVM highside is settled
        while ((HWREG16(PMM_BASE + OFS_PMMIFG) & SVSMHDLYIFG) == 0) ;

        //Clear flag
        HWREG16(PMM_BASE + OFS_PMMIFG) &= ~SVSMHDLYIFG;

        //Check if a VCore increase is possible
        if ((HWREG16(PMM_BASE + OFS_PMMIFG) & SVMHIFG) == SVMHIFG) {
                //-> Vcc is too low for a Vcore increase
                //recover the previous settings
                HWREG16(PMM_BASE + OFS_PMMIFG) &= ~SVSMHDLYIFG;
                HWREG16(PMM_BASE + OFS_SVSMHCTL) = SVSMHCTL_backup;

                //Wait until SVM highside is settled
                while ((HWREG16(PMM_BASE + OFS_PMMIFG) & SVSMHDLYIFG) == 0) ;

                //Clear all Flags
                HWREG16(PMM_BASE +
                        OFS_PMMIFG) &= ~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG |
                                         SVMLVLRIFG | SVMLIFG |
                                         SVSMLDLYIFG
                                         );

                //Restore PMM interrupt enable register
                HWREG16(PMM_BASE + OFS_PMMRIE) = PMMRIE_backup;
                //Lock PMM registers for write access
                HWREG8(PMM_BASE + OFS_PMMCTL0_H) = 0x00;
                //return: voltage not set
                return STATUS_FAIL;
        }

        //Set also SVS highside to new level
        //Vcc is high enough for a Vcore increase
        HWREG16(PMM_BASE + OFS_SVSMHCTL) |= (SVSHRVL0 * level);

        //Wait until SVM highside is settled
        while ((HWREG16(PMM_BASE + OFS_PMMIFG) & SVSMHDLYIFG) == 0) ;

        //Clear flag
        HWREG16(PMM_BASE + OFS_PMMIFG) &= ~SVSMHDLYIFG;

        //Set VCore to new level
        HWREG8(PMM_BASE + OFS_PMMCTL0_L) = PMMCOREV0 * level;

        //Set SVM, SVS low side to new level
        HWREG16(PMM_BASE + OFS_SVSMLCTL) = SVMLE | (SVSMLRRL0 * level) |
                                           SVSLE | (SVSLRVL0 * level);

        //Wait until SVM, SVS low side is settled
        while ((HWREG16(PMM_BASE + OFS_PMMIFG) & SVSMLDLYIFG) == 0) ;

        //Clear flag
        HWREG16(PMM_BASE + OFS_PMMIFG) &= ~SVSMLDLYIFG;
        //SVS, SVM core and high side are now set to protect for the new core level

        //Restore Low side settings
        //Clear all other bits _except_ level settings
        HWREG16(PMM_BASE + OFS_SVSMLCTL) &= (SVSLRVL0 + SVSLRVL1 + SVSMLRRL0 +
                                             SVSMLRRL1 + SVSMLRRL2
                                             );

        //Clear level settings in the backup register,keep all other bits
        SVSMLCTL_backup &=
                ~(SVSLRVL0 + SVSLRVL1 + SVSMLRRL0 + SVSMLRRL1 + SVSMLRRL2);

        //Restore low-side SVS monitor settings
        HWREG16(PMM_BASE + OFS_SVSMLCTL) |= SVSMLCTL_backup;

        //Restore High side settings
        //Clear all other bits except level settings
        HWREG16(PMM_BASE + OFS_SVSMHCTL) &= (SVSHRVL0 + SVSHRVL1 +
                                             SVSMHRRL0 + SVSMHRRL1 +
                                             SVSMHRRL2
                                             );

        //Clear level settings in the backup register,keep all other bits
        SVSMHCTL_backup &=
                ~(SVSHRVL0 + SVSHRVL1 + SVSMHRRL0 + SVSMHRRL1 + SVSMHRRL2);

        //Restore backup
        HWREG16(PMM_BASE + OFS_SVSMHCTL) |= SVSMHCTL_backup;

        //Wait until high side, low side settled
        while (((HWREG16(PMM_BASE + OFS_PMMIFG) & SVSMLDLYIFG) == 0) ||
               ((HWREG16(PMM_BASE + OFS_PMMIFG) & SVSMHDLYIFG) == 0)) ;

        //Clear all Flags
        HWREG16(PMM_BASE + OFS_PMMIFG) &= ~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG |
                                            SVMLVLRIFG | SVMLIFG | SVSMLDLYIFG
                                            );

        //Restore PMM interrupt enable register
        HWREG16(PMM_BASE + OFS_PMMRIE) = PMMRIE_backup;

        //Lock PMM registers for write access
        HWREG8(PMM_BASE + OFS_PMMCTL0_H) = 0x00;

        return STATUS_SUCCESS;
}

uint16_t _PMM_setVCoreDown( uint8_t level)
{
        uint32_t PMMRIE_backup, SVSMHCTL_backup, SVSMLCTL_backup;

        //The code flow for decreasing the Vcore has been altered to work around
        //the erratum FLASH37.
        //Please refer to the Errata sheet to know if a specific device is affected
        //DO NOT ALTER THIS FUNCTION

        //Open PMM registers for write access
        HWREG8(PMM_BASE + OFS_PMMCTL0_H) = 0xA5;

        //Disable dedicated Interrupts
        //Backup all registers
        PMMRIE_backup = HWREG16(PMM_BASE + OFS_PMMRIE);
        HWREG16(PMM_BASE + OFS_PMMRIE) &= ~(SVMHVLRPE | SVSHPE | SVMLVLRPE |
                                            SVSLPE | SVMHVLRIE | SVMHIE |
                                            SVSMHDLYIE | SVMLVLRIE | SVMLIE |
                                            SVSMLDLYIE
                                            );
        SVSMHCTL_backup = HWREG16(PMM_BASE + OFS_SVSMHCTL);
        SVSMLCTL_backup = HWREG16(PMM_BASE + OFS_SVSMLCTL);

        //Clear flags
        HWREG16(PMM_BASE + OFS_PMMIFG) &= ~(SVMHIFG | SVSMHDLYIFG |
                                            SVMLIFG | SVSMLDLYIFG
                                            );

        //Set SVM, SVS high & low side to new settings in normal mode
        HWREG16(PMM_BASE + OFS_SVSMHCTL) = SVMHE | (SVSMHRRL0 * level) |
                                           SVSHE | (SVSHRVL0 * level);
        HWREG16(PMM_BASE + OFS_SVSMLCTL) = SVMLE | (SVSMLRRL0 * level) |
                                           SVSLE | (SVSLRVL0 * level);

        //Wait until SVM high side and SVM low side is settled
        while ((HWREG16(PMM_BASE + OFS_PMMIFG) & SVSMHDLYIFG) == 0 ||
               (HWREG16(PMM_BASE + OFS_PMMIFG) & SVSMLDLYIFG) == 0) ;

        //Clear flags
        HWREG16(PMM_BASE + OFS_PMMIFG) &= ~(SVSMHDLYIFG + SVSMLDLYIFG);
        //SVS, SVM core and high side are now set to protect for the new core level

        //Set VCore to new level
        HWREG8(PMM_BASE + OFS_PMMCTL0_L) = PMMCOREV0 * level;

        //Restore Low side settings
        //Clear all other bits _except_ level settings
        HWREG16(PMM_BASE + OFS_SVSMLCTL) &= (SVSLRVL0 + SVSLRVL1 + SVSMLRRL0 +
                                             SVSMLRRL1 + SVSMLRRL2
                                             );

        //Clear level settings in the backup register,keep all other bits
        SVSMLCTL_backup &=
                ~(SVSLRVL0 + SVSLRVL1 + SVSMLRRL0 + SVSMLRRL1 + SVSMLRRL2);

        //Restore low-side SVS monitor settings
        HWREG16(PMM_BASE + OFS_SVSMLCTL) |= SVSMLCTL_backup;

        //Restore High side settings
        //Clear all other bits except level settings
        HWREG16(PMM_BASE + OFS_SVSMHCTL) &= (SVSHRVL0 + SVSHRVL1 + SVSMHRRL0 +
                                             SVSMHRRL1 + SVSMHRRL2
                                             );

        //Clear level settings in the backup register, keep all other bits
        SVSMHCTL_backup &=
                ~(SVSHRVL0 + SVSHRVL1 + SVSMHRRL0 + SVSMHRRL1 + SVSMHRRL2);

        //Restore backup
        HWREG16(PMM_BASE + OFS_SVSMHCTL) |= SVSMHCTL_backup;

        //Wait until high side, low side settled
        while (((HWREG16(PMM_BASE + OFS_PMMIFG) & SVSMLDLYIFG) == 0) ||
               ((HWREG16(PMM_BASE + OFS_PMMIFG) & SVSMHDLYIFG) == 0)) ;

        //Clear all Flags
        HWREG16(PMM_BASE + OFS_PMMIFG) &= ~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG |
                                            SVMLVLRIFG | SVMLIFG | SVSMLDLYIFG
                                            );

        //Restore PMM interrupt enable register
        HWREG16(PMM_BASE + OFS_PMMRIE) = PMMRIE_backup;
        //Lock PMM registers for write access
        HWREG8(PMM_BASE + OFS_PMMCTL0_H) = 0x00;
        //Return: OK
        return STATUS_SUCCESS;
}

bool _PMM_setVCore( uint8_t level)
{
        assert(
                (PMM_CORE_LEVEL_0 == level) ||
                (PMM_CORE_LEVEL_1 == level) ||
                (PMM_CORE_LEVEL_2 == level) ||
                (PMM_CORE_LEVEL_3 == level)
                );

        uint8_t actlevel;
        bool status = STATUS_SUCCESS;

        //Set Mask for Max. level
        level &= PMMCOREV_3;

        //Get actual VCore
        actlevel = (HWREG16(PMM_BASE + OFS_PMMCTL0) & PMMCOREV_3);

        //step by step increase or decrease
        while ((level != actlevel) && (status == STATUS_SUCCESS)) {
                if (level > actlevel)
                        status = _PMM_setVCoreUp(++actlevel);
                else
                        status = _PMM_setVCoreDown(--actlevel);
        }

        return status;
}


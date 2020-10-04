/*------------------------------------------------------------------------------------------------------
 * CE2826 module
 *______________________________________________________________________________________________________
 *
 *
 *
 *
 *
 *
 *
 *
 *___________________________________________________________________________________________
 *
 * Created: 28-Sep-2020 20:50:14
 *  Author: VAL
 *------------------------------------------------------------------------------------------------------- 
 *
 *
 *-------------------------------------------------------------------------------------------------------
 *	History:
 *		-	29-Sep-2020
 *          Creation of this module.
 *
 *-------------------------------------------------------------------------------------------------------
 *	Features:
 *
 *-------------------------------------------------------------------------------------------------------
 */

#ifndef CE2826_H
#define CE2826_H

#include "standart_interface.h"

//Device address
#define CE2826_ADDRESS  0x6A

//
#define READ            0x01
#define WRITE           0x00

//Registers addresses
#define CE2826_CREG0    0x00 //Control register 0 / Default = 0x80 / FMT[7:6],NBIT[5:4],AMUTE[3],DEEMP[2],FSMPL[1:0]
#define CE2826_CREG1    0x01 //Control register 1 / Default = 0x80 / AUTODET[7],FS384[6],CKDIV4[5],SCDIV2[4],ADCEN[3],MUTE56[2],MUTE43[1],MUTE12[0]
#define CE2826_VOLREG0  0x02 //Volume refisters 1(L,R) - 3(L,R) channels/ Default = 0x80
#define CE2826_VOLREG1  0x03
#define CE2826_VOLREG2  0x04
#define CE2826_VOLREG3  0x05
#define CE2826_VOLREG4  0x06
#define CE2826_VOLREG5  0x07

typedef struct{
    unsigned FMT    : 2;    //Digital serial bus format select
    // 00 - Right justified format
    // 01 - Left justified format
    // 10 - I2S format (default)
    // 11 - TDM Multi channel time division multiplex format (only DIN1 using)
    unsigned NBIT   : 2;    //n of bits / define serial audio input resolution for right justified and TDM mode
    // 00 - 16-bit resolution (default)
    // 01 - 20-bit resolution
    // 10 - 24-bit resolution
    // 11 - 32-bit resolution
    unsigned AMUTE  : 1;    //Auto-mute detection enable
    // 0  - Auto-mute enabled (default)
    // 1  - No auto-mute
    unsigned DEEMP  : 1;    //Enable de-empasis
    // 0  - Normal (default)
    // 1  - enable de-empasis     
    unsigned FSMPL  : 2;    //Interpolation filter selection / Work only when AUTODET bit set to 0
    // 0x - 44.1kHz or 48kHz sampling. (default)
    // 10 - 96kHz sampling.
    // 11 - 192kHz sampling.
}CREG0;

typedef struct{
    unsigned AUTODET: 1;    //Auto detection of serial audio input data sampling rate clock frequency
    // 0  - do not use autodetect
    // 1  - enable auto-detection (default)
    unsigned FS384  : 1;    //384fs or 256fs control for PLL clock output / Work only when AUTODET bit set to 0
    // 0  - PLL takes the reference clock and multiplies it by 2 to generate a 512 bit clock
    // 1  - PLL takes the reference clock and multiplies it by 4/3 to generate a 512 bit clock
    unsigned CKDIV4 : 1;    //Clock divider enable control / Work only when AUTODET bit set to 0
    // 0  - do not enable input clock divider by 4
    // 1  - enable input clock divider by 4
    unsigned CKDIV2 : 1;    //Clock divider enable control / Work only when AUTODET bit set to 0
    // 0  - do not enable input clock divider by 2
    // 1  - enable input clock divider by 2
    unsigned ADCEN  : 1;    //Enable ADC
    // 0  - ADC is disabled
    // 1  - ADC is enabled
    unsigned MUTE56 : 1;    //Mute control for channels 5 and 6
    // 0  - do not mute channels 5 and 6
    // 1  - mute channels 5 and 6
    unsigned MUTE34 : 1;    //Mute control for channels 4 and 3
    // 0  - do not mute channels 3 and 4
    // 1  - mute channels 3 and 4
    unsigned MUTE12 : 1;    //Mute control for channels 2 and 1
    // 0  - do not mute channels 1 and 2
    // 1  - mute channels 1 and 2
}CREG1;

void CE2826_write_creg(uint8_t addr, uint8_t data);

void CE2826_read_creg(uint8_t addr);

#endif

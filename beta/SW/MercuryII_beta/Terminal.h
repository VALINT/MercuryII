/*------------------------------------------------------------------------------------------------------
 * Terminal (serial connection with PC or other devices)
 * Realize like Arduino serial connection for debugging or data communication.
 *______________________________________________________________________________________________________
 *  __    __ __ __     __   _____	 _____  ____  ____  __  __  _  __  _    _    _
 *  \ \  / //  \\ \    \_\ / ___/	|_   _|| ___|| |\ \|  \/  || ||  \| |  / \  | |
 *   \ \/ //    \\ \___ |/ \___ \	  | |  | ___|| |/ /| |\/| || || |\  | / _ \ | |__
 *    \__//__/\__\\____\   /____/     |_|  |____||_|\_\|_|  |_||_||_| \_|/_/ \_\|____|
 *_______________________________________________________________________________________________________
 *
 *  Created: 14-Oct-2018 23:05:14
 *  Author: VAL
 *------------------------------------------------------------------------------------------------------ 
 *
 *  Attempt to make this module universal for different architectures.
 *///------------------------------------------------------------------------------------------------------ 
 
//For true work needed to use font - "terminal"


#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "standart_interface.h"

//#define PuTTy

char line[100];

#define ARRAY &(line[0])
#define PrintLn1(text1) sprintf(line, text1); _uart_send_array(ARRAY); _uart_write_char('\n'); _uart_write_char(13);
#define PrintLn2(text1, text2) sprintf(line, text1, text2); _uart_send_array(ARRAY); _uart_write_char('\n'); _uart_write_char(13);
#define PrintLn3(text1, text2, text3) sprintf(line, text1, text2, text3); _uart_send_array(ARRAY); _uart_write_char('\n'); _uart_write_char(13);
#define PrintLn4(text1, text2, text3, text4) sprintf(line, text1, text2, text3, text4); _uart_send_array(ARRAY); _uart_write_char('\n'); _uart_write_char(13);

#define PrintLnS(text1)  _uart_send_array_s(text1); _uart_write_char('\n'); _uart_write_char(13);
#define PrintStrS(text1) _uart_send_array_s(text1);
#define PrintSymb(text1) _uart_write_char(text1);

#define PrintStr1(text1) sprintf(line, text1,'\0'); _uart_send_array(ARRAY);
#define PrintStr2(text1, text2) sprintf(line, text1, text2,'\0'); _uart_send_array(ARRAY);
#define PrintStr3(text1, text2, text3) sprintf(line, text1, text2, text3,'\0'); _uart_send_array(ARRAY);
#define PrintStr4(text1, text2, text3, text4) sprintf(line, text1, text2, text3, text4); _uart_send_array(ARRAY);

#define NewLine  _uart_write_char('\n'); _uart_write_char(13);

//Next Function Print next in terminal.
//*______________________________________________________________________________________________________");
//*  __    __ __ __     __   _____	 _____  ____  ____  __  __  _  __  _    _    _
//*  \ \  / //  \\ \    \_| / ___/	|_   _|| ___|| |\ \|  \/  || ||  \| |  / \  | |
//*   \ \/ //    \\ \___ \/ \___ \	  | |  | ___|| |/ /| |\/| || || |\  | / _ \ | |__
//*    \__//__/\\__\\____\   /____/   |_|  |____||_|\_\|_|  |_||_||_| \_|/_/ \_\|____|
//*______________________________________________________________________________________________________");

void hi(void)
{
	PrintLnS("*______________________________________________________________________________________________________");
	PrintLnS("*  __    __ __ __     __   _____	 _____  ____  ____  __  __  _  __  _    _    _");
	PrintLnS("*  \\ \\  / //  \\\\ \\    \\_| / ___/	|_   _|| ___|| |\\ \\|  \\/  || ||  \\| |  / \\  | |");
	PrintLnS("*   \\ \\/ //    \\\\ \\___ \\/ \\___ \\	  | |  | ___|| |/ /| |\\/| || || |\\  | / _ \\ | |__");
	PrintLnS("*    \\__//__/\\__\\\\____\\   /____/          |_|  |____||_|\\_\\|_|  |_||_||_| \\_|/_/ \\_\\|____|");
	PrintLnS("*_____________________________________________________________________________________________________");
}
#endif /* TERMINAL_H_ */

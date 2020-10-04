#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "Terminal.h"
#include "pff.h"
#include "diskio.h"
#include "stdint.h"
#include "stdbool.h"
#include "core_cm3.h"
#include "CE2826.h"

#define BUF_SIZE            512

//Buttons
#define PREVIOUS            1
#define PAUSE		        2
#define NEXT		        3
#define NONE		        0

//Main FSM states
#define POR			        0		    // First state, active after power-on or after reset
#define IDLE		        1  		    // Idle mode after POR, wait button action or UART command
#define CCPM_I		        10			// Initiated by UART
#define CCPM_OD		        11			// 
#define CCPM_EOF	        12			//
#define CCPM_STOP	        13			//
#define CCPM_RD		        14			//
#define APM_I		        20			// Initializate auto-playing mode
#define APM_CF		        21			// Count tracks(songs) amount on disc
#define APM_OD		        22			// Open root dir
#define APM_RD		        23			// Read dir
#define APM_OF		        24			// Open needed file
#define APM_SC		        25			// Set playing parameters 
#define APM_WB		        26			// Wait EOF or buttons event
#define APM_EOF		        27			// End of file procedure 
#define APM_BUT		        28			// Buttons procedure
#define FAIL		        255			// Failure mode. Blinking LED in cycle 

//UART FSM states
#define WAIT		        0
#define LINE		        1
#define	PLAY		        2

#define SETUP_TIM2()        RCC->APB1ENR  |= RCC_APB1ENR_TIM2EN;/*Timer2 clocking enable*/ \
                            TIM2->PSC      = 36;/*Set Timer2 prescaler to 32*/ \
                            TIM2->CR1     |= 0x0001;/*Enable Timer2 (TIM2->CR1 = TIM_CR1_CEN)*/ \
                            TIM2->EGR     |= 0x0001/**/   
#define SET_TIM2_AAR(x)     TIM2->ARR      = x/**/ 
#define EN_TIM2_INT()       TIM2->DIER    |= 0x0001;/**/ \
                            NVIC->ISER[0] |= 0x10000000/*Timer2 interrupt enable*/  
#define DIS_TIM2_INT()      TIM2->DIER    &= ~0x0001/**/ 
#define CLR_TIM2_INT_F()    TIM2->SR      &= ~TIM_SR_UIF/**/ 
//------------------------------------------------------------------------------
#define SETUP_TIM3()        RCC->APB1ENR  |= RCC_APB1ENR_TIM3EN; /*Timer3 clocking enable*/ \
                            TIM3->PSC      = 1;/**/ \
                            TIM3->CR1     |= TIM_CR1_CEN;/*Enable Timer3*/ \
                            TIM3->CCMR2   = (TIM_CCMR2_OC4PE | TIM_CCMR2_OC3PE | TIM_CCMR2_OC4M_0 | TIM_CCMR2_OC3M_0 | \
                                             TIM_CCMR2_OC4M_2| TIM_CCMR2_OC4M_1| TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1);\
                            TIM3->CCER     = (TIM_CCER_CC4E | TIM_CCER_CC3E);\
                            TIM3->CCR3     = 0;/**/ \
                            TIM3->CCR4     = 0;/**/ \
                            TIM3->ARR      = 255;\
                            TIM3->EGR     |= TIM_EGR_UG/*?*/ 
#define SET_TIM3_ARR(x)     TIM3->ARR      = 255/**/ 
#define EN_TIM3_GEN()       TIM3->CR1     |= TIM_CR3_CEN/**/ 
#define DIS_TIM3_GEN()      TIM3->CR1     &= ~TIM_CR3_CEN/**/ 
#define SET_TIM3_CH1(x)     TIM3->CCR3     = x/**/ 
#define SET_TIM3_CH2(x)     TIM3->CCR4     = x/**/ 
//---------------------------------------------------------------------------------
#define EN_USART1_INT()     USART1->CR1   |= (USART_CR1_TE | USART_CR1_RXNEIE); \
                            NVIC->ISER[1] |= 0x00000020;
#define DIS_USART1_INT()    USART1->CR1   &= ~USART_CR1_RXNEIE;
//---------------------------------------------------------------------------------
#define LED_OFF()           GPIOC->ODR    |= GPIO_ODR_ODR13/**/ 
#define LED_ON()            GPIOC->ODR    &= ~GPIO_ODR_ODR13/**/ 
#define LED_INVERT()        GPIOC->ODR    ^= GPIO_ODR_ODR13/**/ 

void delay(unsigned int nCount);

//Player struct
typedef struct wav_data
{
	uint16_t size;
	uint16_t sampleRate;
	uint8_t  timerCoef;
	uint16_t fileCounter;
	uint16_t neededFile;
	uint16_t currentFile;
} WAV;

void initWAV(WAV* obj);
void incFC(WAV* obj);
void decSize(WAV* obj);
void setSize(WAV* obj, char *buffer);
void setSampleRate(WAV* obj, char *buffer);

//Variables used in interrupts
char        buf[BUF_SIZE*2];			            //Data buffer, 
uint16_t    sell = 0;			                    //Data buffer's current sell
uint8_t	    channel = 1;                            //Amount of audio channels
uint8_t     mode = POR;			                    //Main FSM state
uint8_t     cmode = WAIT;                           //UART FSM state
uint8_t     UART_B_Event = NONE;                    //
uint8_t     UART_L_Pointer = 0;                     //
uint8_t     UART_GOTO_CCPM = 0;                     //
bool    	UART_OUT_DATA  = true;                  //
bool        end_of_buffer  = false;                 //
bool        first_half     = true;                  //
char        name[13];				                //File name, can be write from UART
char        flname[18] = {'C','C','P','M','/'};     //
FATFS       fs;                                     //
DIR         dir;                                    //
FILINFO     fno;                                    //
WORD        s1;                                     //  
FRESULT     result;                                 //

USART_InitTypeDef USART1_InitStruct;

void TIM2_IRQHandler(void)
{
    
    //Set PWM value
    SET_TIM3_CH1(buf[sell]);
    SET_TIM3_CH2((channel == 2)? buf[sell+1]:buf[sell]);
    TIM2->SR &= ~TIM_SR_UIF;
    TIM2->CNT = 0;
    sell += channel;
    sell &= 1023;
    if(sell == 0 || sell == 512)
    {
        end_of_buffer = true;
        first_half    = sell < 512;
    }
}

void USART1_IRQHandler(void)
{
	UART_B_Event = NONE;
	uint8_t UARTtemp = USART1->DR;
	switch(cmode)
	{
		case WAIT:
			if (UARTtemp == 'S') 
			{
				for(uint8_t i = 0; i < 13; i++)
					name[i] = 0;
				cmode = LINE;
				break;	
			}
			else if (UARTtemp == 'R')
				mode = CCPM_RD;
			else if (UARTtemp == 'O')
				UART_OUT_DATA = ~UART_OUT_DATA;				
			else if(mode == APM_WB || mode == IDLE)
			{
					 if(UARTtemp == 'B') UART_B_Event = PREVIOUS;
				else if(UARTtemp == 'P') UART_B_Event = PAUSE;
				else if(UARTtemp == 'N') UART_B_Event = NEXT;
				break;
			}			
			break;
		case LINE:
			if(UARTtemp == 8)
			{
				if(UART_L_Pointer != 0) UART_L_Pointer--;
			}
			else if(UARTtemp == '<')
				cmode = WAIT;			
			else if(UARTtemp != '.')
			{
				name[UART_L_Pointer] = UARTtemp;
				UART_L_Pointer++;
			}
			else
			{
				name[UART_L_Pointer] = '.';
				name[UART_L_Pointer+1] = 'W';
				name[UART_L_Pointer+2] = 'A';
				name[UART_L_Pointer+3] = 'V';
				UART_L_Pointer = 0;
				UART_GOTO_CCPM = 1;
				UART_B_Event = 0;
				if(UART_OUT_DATA)
				{
					_uart_send_array(name);
					NewLine;
				}					
				cmode = PLAY;
			}
			break;
		case PLAY:
			if(UARTtemp == 'F')
			{
				UART_GOTO_CCPM = 255;
				cmode = WAIT;	
				UART_B_Event = 0;
				UART_L_Pointer = 0;
			}					
			break;
	}
	if(UART_OUT_DATA)
	{
		_uart_write_char(UARTtemp);
		NewLine;
	}		
};

int8_t isWAV(char *string);

int main(void)
{		
	WAV         player;
	initWAV(&player);
	uint16_t    current_song = 0;
	uint16_t    needed_song = 1;
	uint8_t     temp8 = 0;
	bool        playing = false;
	
    delay(1000);
    
    while(1)
    {
		switch(mode)
		{
//----------------------------------------------------------------------------------------------------------------------
// mode = 0 // POR MODE // Start initialization after power on or after reset.
//----------------------------------------------------------------------------------------------------------------------
			case POR:
				//Set up GPIO
				// Enable clock for GPIOB
                RCC->APB2ENR  |= RCC_APB2ENR_IOPAEN;                    // GPIO A enable clock
                RCC->APB2ENR  |= RCC_APB2ENR_IOPBEN;                    // GPIO B enable clock
                RCC->APB2ENR  |= RCC_APB2ENR_IOPCEN;                    // GPIO C enable Clock
                GPIOB->CRL     = (GPIO_CRL_MODE1_0 | GPIO_CRL_MODE0_0 | GPIO_CRL_MODE1_1 | GPIO_CRL_MODE0_1);
                GPIOB->CRL    |= (GPIO_CRL_CNF1_1 | GPIO_CRL_CNF0_1);
                GPIOC->CRH    |= GPIO_CRH_MODE13_0;
                //Buttons
                //Init SPI
                _spi_init();
                //Only in debug version
                LED_OFF();
				//Init SD
				for(uint8_t i = 0; i < 5; i++)	//Try to initialize SD card. 5 tries with 1s gap between tries.
				{
					delay(1000);
					result = pf_mount(&fs);
					if(result == FR_OK)
					{
						mode = IDLE;							// If try is successful go to IDLE mode 
						break;
					}						
					else
						mode = FAIL;							// If all tries are failed go to FAIL mode
				}
                //Init PWM
                SETUP_TIM3();
			    //Init UART
				_uart_init();
                NewLine;
                _uart_send_array("Try to initializate I2C - ");
                if (_i2c_init() == 0) _uart_send_array("OK");
                else                  _uart_send_array("BUS BUSY");
                NewLine;
                
               // _uart_send_array("Try to read 00h register - ");
                //CE2826_read_creg(0x00);
                //_uart_write_char(48 + 7*((I2C1->DR >> 4) > 9) + (I2C1->DR >> 4));
                //_uart_write_char(48 + 7*((I2C1->DR & 0x0F) > 9) + (I2C1->DR & 0x0F));
                CE2826_write_creg(0x00, 0xa5);
                NewLine;
                //_uart_send_array("Try to read 00h register - ");
                //CE2826_read_creg(0x00);
                _uart_write_char(48 + 7*((I2C1->DR >> 4) > 9) + (I2C1->DR >> 4));
                _uart_write_char(48 + 7*((I2C1->DR & 0x0F) > 9) + (I2C1->DR & 0x0F));
                
                /*
                for(int i = 0; i < 127; i++)
                {
                    delay(200);
                    _i2c_start_operation();
                    
                    _uart_write_char(48 + 7*((i >> 4) > 9) + (i >> 4));
                    _uart_write_char(48 + 7*((i & 0x0F) > 9) + (i & 0x0F));    
                    
                    if(_i2c_send_byte(i << 1) == I2C_OK)
                    {
                        _uart_send_array(".H address successful!");
                        NewLine;
                    }
                    else
                    {
                        _uart_send_array(".H address fail!");
                        NewLine;
                    }
                    
                    _i2c_stop_operation();
                }*/
                
                
                
                EN_USART1_INT();
                // Enable interrupts
                __enable_irq();
				break;
//----------------------------------------------------------------------------------------------------------------------
// mode = 1 // IDLE MODE // player wait UART command or buttons request
//----------------------------------------------------------------------------------------------------------------------
			case IDLE:
                    LED_OFF();
					//temp8 = buttons();
					temp8 = NONE;
                    // Need to delate in release code
                    mode = APM_I;
                    if((temp8 != NONE) || (UART_B_Event != NONE))
					{
						UART_B_Event = NONE; 
						mode = APM_I;
					}		
					if(UART_GOTO_CCPM == 1)
					{
						UART_GOTO_CCPM = 0;
						mode = CCPM_I;				
					}					
					break;
					// Here not described UART behavior because all UART commands handle in UART receive interruption
//----------------------------------------------------------------------------------------------------------------------
// mode =  20// APM_I MODE // Auto playing mode initialization. Read all in root DIR and count all WAV files. 
//----------------------------------------------------------------------------------------------------------------------
			case APM_I:
					result = pf_opendir(&dir,"");
					if(result != FR_OK)							//If read access is failed go to FAIL mode
					{
						mode = FAIL;
						break;
					}
					else
					{
						while(1)
						{
							result = pf_readdir(&dir,&fno);		// Get file or DIR name
							if(result != FR_OK)					// If read access if failed go to FAIL mode.
							{
								mode = FAIL;
								break;
							}
							else
							{
								int8_t temp = isWAV(fno.fname);
								if(temp == 1)
								{
								// increment songs counter
									incFC(&player);				// If current read file if WAV increment files counter
								}
								else if(temp == -1)				// If current read file has 0 length name then it is the last file  
								{
								// stop counting
									mode = APM_OD;				// All file are read, go to APM_OD mode
									break;
								}
							}							
						}
					}					
					break;
//----------------------------------------------------------------------------------------------------------------------
// mode = 22// APM_OD MODE // Auto playing mode open DIR, Open root dir to start read files from begin.
//----------------------------------------------------------------------------------------------------------------------
			case APM_OD:
					result = pf_opendir(&dir,"");
					if(result != FR_OK)							//If read access is failed go to FAIL mode
					{
						mode = FAIL;
						break;
					}
					mode = APM_RD;								//If read access successful go to APM_RD mode 
					break;
//----------------------------------------------------------------------------------------------------------------------
// mode = 23// APM_RD MODE // Auto playing mode read DIR, Read root dir to find needed file.
//----------------------------------------------------------------------------------------------------------------------
			case APM_RD:
					result = pf_readdir(&dir,&fno);
					if(result != FR_OK)							//IF read access is failed go to FAIL mode
					{
						mode = FAIL;
						break;
					}
					mode = APM_OF;								//IF read access successful go to APM_OF mode 
					break;
//----------------------------------------------------------------------------------------------------------------------
// mode = 24// APM_OF MODE // Auto playing mode open file. Open needed file. 
//----------------------------------------------------------------------------------------------------------------------
			case APM_OF:
					if(isWAV(fno.fname) == 1)					            //If last read file is WAV
					{
						current_song++;
						if(current_song == needed_song){		            //If this file is needed for me
							pf_open(fno.fname);					            //Open this file and read metadata
							pf_read(&buf, BUF_SIZE, &s1);
							setSampleRate(&player, &buf[0]);		        //Get sample rate
							setSize(&player, &buf[0]);				        //Get file size
							pf_read(&buf, BUF_SIZE, &s1);		            //Read next block to start playing
							if(UART_OUT_DATA)					            //If output mode enabled
							{	// Print filename in console
                                NewLine;
								_uart_send_array(fno.fname);		
								NewLine;
								// Print sample rate i console
								_uart_send_array("Sample rate - ");
								_uart_write_char(48 +  player.sampleRate          / 10000);
								_uart_write_char(48 + (player.sampleRate % 10000)/ 1000);
								_uart_write_char(48 + (player.sampleRate % 1000)/ 100);
								_uart_write_char(48 + (player.sampleRate % 100)/ 10);
								_uart_write_char(48 +  player.sampleRate % 10);
								NewLine;
								// Print channels amount in console
								_uart_send_array("Chan - ");
								_uart_write_char(channel+48);
								NewLine;
								// Print timer coefficient in console
								_uart_send_array("TC - ");
								_uart_write_char( player.timerCoef		  / 100 + 48);
								_uart_write_char((player.timerCoef % 100) / 10  + 48);
								_uart_write_char( player.timerCoef % 10		    + 48);
							}
							mode = APM_SC;}						//Start playing go to APM_SC mode
						else									//If this file isn't needed for me
							mode = APM_RD;						//Read next file
					}
					else										//If last read file isn't WAV
						mode = APM_RD;							//Read next file
					break;
//----------------------------------------------------------------------------------------------------------------------
// mode =  25// APM_SC MODE // auto playing mo de set configuration // Preparing to playing, set timers configurations. 
//----------------------------------------------------------------------------------------------------------------------
			case APM_SC:
                    SETUP_TIM2();                               //Setup sample timer
                    SET_TIM2_AAR(player.timerCoef);             //Set timer coeficient
                    EN_TIM2_INT();                              //Enable Timer2
                    SETUP_TIM3();                               //Setup Timer3 (PWM)
					//LED on
                    LED_ON();
                    NewLine;
                    _uart_send_array("Size - ");
                    _uart_write_char(48 +  player.size         / 10000);
                    _uart_write_char(48 + (player.size % 10000)/ 1000);
                    _uart_write_char(48 + (player.size % 1000) / 100);
                    _uart_write_char(48 + (player.size % 100)  / 10);
                    _uart_write_char(48 +  player.size % 10);
					playing = true;
					mode = APM_WB;
					break;
//----------------------------------------------------------------------------------------------------------------------
// mode = 26// APM_WB MODE // Auto playing mode wait buttons or EOF. Wait buttons event or end of file
//----------------------------------------------------------------------------------------------------------------------
			case APM_WB:
                        if(end_of_buffer)			// If pointer near end of buffer read fresh data 	
						{
                            end_of_buffer = false;
							if(first_half) result = pf_read(&buf[512], BUF_SIZE, &s1);
                            else result = pf_read(&buf[0], BUF_SIZE, &s1);
                            if(result != FR_OK)						//IF read access is failed go to FAIL mode
                            {
                                //DIS_TIM2_INT();                     //Disable Timer2
                                NewLine;
                                _uart_send_array("Content reading error!");
                                NewLine;
                                //mode = FAIL;
                                //break;
                            }
							decSize(&player);					    // Decrease amount of blocks to end of file.
						}
						if(player.size == 0)					    // If end of file read play new *.WAV file
						{   
                            DIS_TIM2_INT();
                            NewLine;
                            _uart_send_array("Size - ");
                            _uart_write_char(48 +  player.size          / 10000);
                            _uart_write_char(48 + (player.size % 10000) / 1000);
                            _uart_write_char(48 + (player.size % 1000)  / 100);
                            _uart_write_char(48 + (player.size % 100)   / 10);
                            _uart_write_char(48 +  player.size % 10);
                            NewLine;
                            PrintLnS("New file");
                            NewLine;
							needed_song++;
							current_song = 0;
							mode = APM_OD;
						}
						//temp8 = buttons();
                        temp8 = NONE;
						if(temp8 != NONE || UART_B_Event != NONE)						// If buttons request go to buttons handler
							mode = APM_BUT;
						if(UART_GOTO_CCPM == 1)
							mode = CCPM_I;					
					break;
//----------------------------------------------------------------------------------------------------------------------
// mode = 27// APM_BUT MODE // Auto playing mode buttons hander.
//----------------------------------------------------------------------------------------------------------------------
			case APM_BUT:
					if(temp8 == PREVIOUS || UART_B_Event == PREVIOUS)	//Previous track (handles button event and UART event)
					{
						UART_B_Event = NONE;
						if(current_song == 1)							//If current song first - play last in queue
						{
							needed_song = player.fileCounter;
							current_song = 0;
							mode = APM_OD;
						}
						else											//If current song not first - play previous track in queue
						{
							needed_song = current_song-1;
							current_song = 0;
							mode = APM_OD;
						}
						DIS_TIM2_INT();
						delay(200);
					}	 						
					else if(temp8 == PAUSE || UART_B_Event == PAUSE)	//Play/pause track (handles button event and UART event)
					{
						UART_B_Event = NONE;
						if(playing)										//If track playing - pause
						{
							playing = false;
							DIS_TIM2_INT();
							delay(200);
						}
						else											//If pause - cotinue to play 
						{
							playing = true;
							delay(200);
							EN_TIM2_INT();
						}
						mode = APM_WB;
					}							
					else if(temp8 == NEXT || UART_B_Event == NEXT)	    //Next track (handles button event and UART event)
					{
						UART_B_Event = NONE;
						if(current_song == player.fileCounter)		    //If current song last in queue - play first
						{
							needed_song = 1;
							current_song = 0;
							mode = APM_OD;
						}
						else										    //If current track not last - play next track
						{
							needed_song = current_song+1;
							current_song = 0;
							mode = APM_OD;
						}
						DIS_TIM2_INT();
						delay(200);
					}
					break;
//----------------------------------------------------------------------------------------------------------------------
// mode = 10// CCPM_I //Command controllable playing mode initialization
//----------------------------------------------------------------------------------------------------------------------
			case CCPM_I:			
					for(uint8_t i = 5; i < 18; i++)				        //Merge directive path and file name
					{
						flname[i] = name[i-5]; 
					}
					result = pf_open(flname);					        //Open file in CCPM directive(folder)
					if(result != FR_OK)							        //If read access is failed go to FAIL mode
					{
                        NewLine;
						_uart_write_char('E');					        //Error
						mode = IDLE;
						cmode = WAIT;
						break;
					}
					pf_read(&buf, BUF_SIZE, &s1);
					setSampleRate(&player, &buf[0]);			        //Get sample rate
					setSize(&player, &buf[0]);					        //Get file size
					pf_read(&buf, BUF_SIZE, &s1);				        //Read next block to start playing
					SETUP_TIM2();
                    SET_TIM2_AAR(player.timerCoef);
                    LED_ON();
					playing = true;
					mode = CCPM_STOP;
					break;
//----------------------------------------------------------------------------------------------------------------------
// mode = 11// CCPM_STOP //Command controllable playing mode wait stop event
//----------------------------------------------------------------------------------------------------------------------
			case CCPM_STOP:
					if(end_of_buffer && player.size)			// If pointer near end of buffer read fresh data 	
					{
                        end_of_buffer = false;
						if(first_half)
                            result = pf_read(&buf[512], BUF_SIZE, &s1);
                        else
                            result = pf_read(&buf[0], BUF_SIZE, &s1);
                        if(result != FR_OK)						//IF read access is failed go to FAIL mode
                        {
                            DIS_TIM2_INT();                     //Disable Timer2
                            NewLine;
                            _uart_send_array("Content reading error!");
                            NewLine;
                            mode = FAIL;
                            break;
                        }
						decSize(&player);					    // Decrease amount of blocks to end of file.
					}
					if(player.size == 0 || UART_GOTO_CCPM == 255)	    //If end of file read play new *.WAV file
					{
						current_song = 0;
						mode = CCPM_EOF;
						break;
					}					
					break;
//----------------------------------------------------------------------------------------------------------------------
// mode = 12// CCPM_EOF //Command controllable playing mode end of file
//----------------------------------------------------------------------------------------------------------------------
			case CCPM_EOF:											    //If end of file return 'F' and return to IDLE mode
					NewLine;
                    _uart_write_char('F');
					mode = IDLE;
					cmode = WAIT;
					DIS_TIM2_INT();
					break;
//----------------------------------------------------------------------------------------------------------------------
// mode = 14// CCPM_RD //Command controllable playing mode read directive
//----------------------------------------------------------------------------------------------------------------------			
			case CCPM_RD:										        //Return all file names from CCPM DIR
					result = pf_opendir(&dir,"CCPM");
					if(result != FR_OK)							        //If read access is failed go to FAIL mode
					{
                        NewLine;
						_uart_write_char('E');
						mode = IDLE;
						cmode = WAIT;
						break;
					}
					while(1)
					{
						pf_readdir(&dir,&fno);
						_uart_send_array(fno.fname);
						NewLine;
						if(fno.fname[0] == '\0')
						{
							mode = IDLE;
							break;		
						}											
					}
					break;
//----------------------------------------------------------------------------------------------------------------------
// mode =  255// FAIL MODE// Blink LED in infinite loop. One possible way to go out is reset.
//----------------------------------------------------------------------------------------------------------------------
			case FAIL: 
					while(1)
					{
                        LED_INVERT();
						delay(300);
					}
		}			
    }
}

void initWAV(WAV* obj)						                // Init. structure
{
	obj->currentFile = 0;
	obj->fileCounter = 0;
	obj->neededFile = 0;
	obj->sampleRate = 0;
	obj->size = 0;
	obj->timerCoef = 0;
}

void incFC(WAV* obj)						                // Increment file counter
{
	obj->fileCounter++;
}

void decSize(WAV* obj)						                // Decrement file size
{
	obj->size--;
}

void setSize(WAV* obj, char *buffer)		                // Set file size 
{
	uint32_t temp = ((uint32_t)(*(buffer+4))|(((uint32_t)*(buffer+5))<<8)|(((uint32_t)*(buffer+6))<<16)|(((uint32_t)*(buffer+7))<<24));
	obj->size = (uint16_t)(temp>>9);
}

void setSampleRate(WAV* obj, char *buffer)	                // Set sample rate and time coefficient
{
	channel = *(buffer+22);
	//channel = 2;
	obj->sampleRate = (((uint16_t)(*(buffer + 25)))<<8) | *(buffer+24);
	obj->timerCoef = (uint8_t)((2000000/obj->sampleRate)+1);
}


int8_t buttons()							                // Read buttons state
{
	return NONE;	
}

int8_t isWAV(char *string)					                //Check if the most resently read file is *.WAV
{
	for(uint8_t i = 0; i < 13; i++)
	{
		if(*(string+i) == '\0')				                //If name have 0 length it is end of directive 
		{
			if(i == 0)
				return -1;					
			return 0;
		}
		else if(((*(string+i) == '.') && (*(string+i+1) == 'W') && (*(string+i+2) == 'A') && (*(string+i+3) == 'V'))||
				((*(string+i) == '.') && (*(string+i+1) == 'w') && (*(string+i+2) == 'a') && (*(string+i+3) == 'v')))
		return 1;
	}
    return 0;
}

// Delay function
void delay(unsigned int nCount)
{
	unsigned int i, j;
	
	for (i = 0; i < nCount; i++)
		for (j = 0; j < 0x2AFF; j++);
}

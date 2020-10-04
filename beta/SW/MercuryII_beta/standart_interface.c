/*------------------------------------------------------------------------------------------------------
 * Standard interface module
 *______________________________________________________________________________________________________
 *  __    __ __ __     __   _____	 _____ _____   __    __  _  ____    __    ____  _____ 
 *  \ \  / //  \\ \    \_\ / ___/	/ ___/|_   _| /  \  |  \| ||  _ \  /  \  | |\ \|_   _|
 *   \ \/ //    \\ \___ |/ \___ \	\___ \  | |  /    \ | |\  || |_|| /    \ | |/ /  | |  
 *    \__//__/\__\\____\   /____/	/____/  |_| /__/\__\|_| \_||____//__/\__\|_|\_\  |_| 
 *					 _  __  _  _____  ____  ____  ____    _    ____  ____ 
 *					| ||  \| ||_   _|| ___|| |\ \| ___|  / \  /  __\| ___|
 *					| || |\  |  | |  | ___|| |/ /| ___| / _ \ | |__ | ___|
 *					|_||_| \_|  |_|  |____||_|\_\|_|   /_/ \_\\____/|____|
 *_______________________________________________________________________________________________________
 *
 * Created: 18-Sep-2018 23:05:14
 *  Author: VAL
 *///------------------------------------------------------------------------------------------------------ 

#include "standart_interface.h"

//---------------------------------------------------------------------------------------------------------
//UART part
#ifdef _AVR
    void _uart_init(void)
    {
        #ifdef atmega328
            //UBRR0H = UBRRL_value;                        /* defined in setbaud.h */
            UBRR0L = UBRRL_value;
            #if USE_2X
                UCSR0A |= (1 << U2X0);
            #else
                UCSR0A &= ~(1 << U2X0);
            #endif
            /* Enable USART transmitter/receiver */
            UCSR0B = (1 << TXEN0) | (1 << RXEN0);
            UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);   /* 8 data bits, 1 stop bit */
        #endif
        
        #ifdef atmega8
            //UBRR0H = UBRRL_value;                        /* defined in setbaud.h */
            UBRRL = UBRRL_value;
            #if USE_2X
                UCSRA |= (1 << U2X);
            #else
                UCSRA &= ~(1 << U2X);
            #endif
            /* Enable USART transmitter/receiver */
            UCSRB = (1 << RXCIE)|(1 << TXEN) | (1 << RXEN);
            UCSRC = (1 << URSEL) | (3 << UCSZ0);   /* 8 data bits, 1 stop bit */
        #endif
        
    }

    void _uart_write_char(char data)
    {
        #ifdef atmega328
            while(! (UCSR0A & (1 << UDRE0))){}
            UDR0 = data;
        #endif
        
        #ifdef atmega8
            while(! (UCSRA & (1 << UDRE))){}
            UDR = data;
        #endif
    }

    void _uart_send_array(char* data)
    {
        while(*data != '\0')
        {
            uart_write_char(*data);
            data++;
        }
    }
    
    void _uart_send_array_s(const char* data)
    { 
        while(*data != '\0')
        {
            uart_write_char(*data);
            data++;
        }
    }
#endif
    
#ifdef _STM32
    void _uart_init()
    {
        GPIO_InitTypeDef GPIO_InitStruct;
        USART_InitTypeDef USART1_InitStruct;
        
        // Enable clock for USART1 unit
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
        
        // Configure USART pins
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;  //RX / TX outputs
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;   	
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
        GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;  //RX / TX outputs
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;   	
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
        GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        // Configure USART1
        USART1_InitStruct.USART_BaudRate = BAUD;
        USART1_InitStruct.USART_WordLength = USART_WordLength_8b;
        USART1_InitStruct.USART_StopBits = USART_StopBits_1;
        USART1_InitStruct.USART_Parity = USART_Parity_No;
        USART1_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
        USART1_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_Init(USART1, &USART1_InitStruct);
        USART_Cmd(USART1, ENABLE);
    }
    
    void _uart_write_char(char data)
    {
        USART_SendData(USART1, data );
        while(!USART_GetFlagStatus(USART1, USART_FLAG_TC));
    }

    void _uart_send_array(char* data)
    {
        while(*data != '\0')
        {
            USART_SendData(USART1, *data );
            while(!USART_GetFlagStatus(USART1, USART_FLAG_TC));
            data ++;
        }
    }
    
    void _uart_send_array_s(const char* data)
    {
        while(*data != '\0')
        {
            USART_SendData(USART1, *data );
            while(!USART_GetFlagStatus(USART1, USART_FLAG_TC));
            data ++;
        }
    }   
#endif
//---------------------------------------------------------------------------------------------------------
// I2C part
    
#ifdef _AVR
    void _i2c_init(void)
    {
        TWBR = 0xFF;
    }

    void _i2c_start_operation(void)
    {
        TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
        while ((TWCR & (1<<TWINT)) == 0);
    }

    void _i2c_stop_operation(void)
    {
        TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
    }

    void _i2c_send_byte(uint8_t byte)
    {
        TWDR = byte;
        TWCR = (1 << TWINT) | (1 << TWEN);
        while ((TWCR & (1 << TWINT)) == 0);
    }

    void _i2c_send_data(uint8_t data, uint8_t addres)
    {
        i2c_start_operation();
        i2c_send_byte(addres);
        i2c_send_byte(data);
        i2c_stop_operation();
    }

    //void i2c_send_array(uint8_t &arr_start, uint8_t lenght);

    uint8_t _i2c_reseive_byte(void)
    {
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
        while ((TWCR & (1 << TWINT)) == 0);
        return TWDR;
    }

    uint8_t _i2c_reseive_last_byte(void)
    {	
        TWCR = (1 << TWINT) | (1 << TWEN);
        while ((TWCR & (1 << TWINT)) == 0);
        return TWDR;
    }
#endif
   
#ifdef _STM32
    I2C_ERR _i2c_init(void)
    {
        //Enable i2c clocking
        RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
        
        //Enable GPIO alternate function
        //In thas case i use I2C1, it tied to PB7, PB6
        
        //Enable PORTB clocking
        RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
        
        //Set PB6 and PB7 to alternate function mode open drain
        GPIOB->CRL    |= (GPIO_CRL_MODE6_0 | GPIO_CRL_MODE7_0 | GPIO_CRL_MODE6_1 | GPIO_CRL_MODE7_1);   // Output max speed
        GPIOB->CRL    |= (GPIO_CRL_CNF6_0  | GPIO_CRL_CNF7_0  | GPIO_CRL_CNF6_1  | GPIO_CRL_CNF7_1);    // Alternate function output Open-drain
        
        /*
        //I2C initialization
        I2C1->CR2   = 36;   //FREQ[5:0] /d20 periferal speed 20MHz Tpclk = 1/20MHz = 50nS
        I2C1->CCR   = ( I2C_CCR_FS | I2C_CCR_DUTY | 2 ); // Fast mode I2C / Duty (tl/th = 16/9) / CCR = 2 (to reach 400kHz speed)
        I2C1->TRISE = 12;//300nS/50nS + 1 (acordingly to reference manual) 300nS - Rising edge of sygnal (from datasheet)
        I2C1->CR1   = I2C_CR1_PE; //Periferal enable
        */
        
        //I2C initialization
        I2C1->CR2   = 36;   //FREQ[5:0] /d36 periferal speed 36MHz Tpclk = 1/26MHz = 27.777[7]nS
        I2C1->CCR   = 180;  // Standart mode / CCR = 180 (to reach 100kHz speed)
        I2C1->TRISE = 37;   // 1000nS/27.7[7]nS + 1 (acordingly to reference manual) 1000nS - Rising edge of sygnal (from datasheet)
        I2C1->CR1   = ( I2C_CR1_PE); //Periferal enable
        
        //if pizdec start out magic spell))
        if(I2C1->SR2 == (I2C_SR2_BUSY) )
        {
            //Let begin out dance across blue pill (( 
            //1. Disable I2C peripheral
            I2C1->CR1   &=~I2C_CR1_PE;
            
            //2. Configure SCL & SDA as output opend drain, hight
            GPIOB->CRL    |= (GPIO_CRL_MODE6_0 | GPIO_CRL_MODE7_0 | GPIO_CRL_MODE6_1 | GPIO_CRL_MODE7_1);   // Output max speed
            GPIOB->CRL    &=~(GPIO_CRL_CNF6_0  | GPIO_CRL_CNF7_0  | GPIO_CRL_CNF6_1  | GPIO_CRL_CNF7_1);    // Clear old values
            GPIOB->CRL    |= (GPIO_CRL_CNF6_0  | GPIO_CRL_CNF7_0);                                          // Output Open-drain
            GPIOB->ODR    |= (GPIO_ODR_ODR6 | GPIO_ODR_ODR7);                                               // Set high at PB6 & PB7
            
            //3. Check SCL & SDA high level
            if((GPIOB->IDR & (GPIO_IDR_IDR6 | GPIO_IDR_IDR7)) != (GPIO_ODR_ODR6 | GPIO_ODR_ODR7)) 
                return I2C_BUS_FAULT;
            
            //4. Config SDA to low level
            GPIOB->ODR    &=~( GPIO_ODR_ODR7 );
            
            //5. Check SDA low level
            if((GPIOB->IDR & (GPIO_IDR_IDR7))) //If high state
                return I2C_SDA_FAULT;
            
            //6. Config SCL to low level
            GPIOB->ODR    &=~( GPIO_ODR_ODR6 );
            
            //7. Check SCL low level
            if((GPIOB->IDR & (GPIO_IDR_IDR6))) //If high state
                return I2C_SCL_FAULT;
            
            //8. Config SCL to high level
            GPIOB->ODR    |= ( GPIO_ODR_ODR6 );
            
            //9. Check SCL high level
            if(!(GPIOB->IDR & (GPIO_IDR_IDR6))) //If low state
                return I2C_SCL_FAULT;
            
            //10. Config SDA to high level
            GPIOB->ODR    |= ( GPIO_ODR_ODR7 );
            
            //11. Check SDA high level
            if(!(GPIOB->IDR & (GPIO_IDR_IDR7))) //If low state
                return I2C_SDA_FAULT;
            
            GPIOB->ODR  &=~ ( GPIO_ODR_ODR6 | GPIO_ODR_ODR7 );
            
            //12. Configure SDA & SCL to alternate function open-drain
            GPIOB->CRL  &=~(GPIO_CRL_CNF6_0  | GPIO_CRL_CNF7_0  | GPIO_CRL_CNF6_1  | GPIO_CRL_CNF7_1);
            GPIOB->CRL  |= (GPIO_CRL_CNF6_0  | GPIO_CRL_CNF7_0  | GPIO_CRL_CNF6_1  | GPIO_CRL_CNF7_1);
            
            //13. Set SWRST bit in I2C_CR1 register
            I2C1->CR1   |= (I2C_CR1_SWRST);  
            
            //14. Clear SWRST bit in I2C_CR1 register
            I2C1->CR1   &=~(I2C_CR1_SWRST);
            
            //15. Enable I2C peripheral
            I2C1->CR1   |= I2C_CR1_PE;
            
            if(I2C1->SR2 == (I2C_SR2_BUSY) )
                return I2C_BUSY;
        }
        
        // The result of my tests - this magic spell is working)))
        return I2C_OK;
    }
    
    void _i2c_start_operation(void)
    {
        //Generate start condition
        I2C1->CR1   |= I2C_CR1_START;
        
        //Wait till start condition ready
        while( !(I2C1->SR1 & (I2C_SR1_SB)) ){};
    }
    
    void _i2c_stop_operation(void)
    {
        //Generate stop condition
        SET_BIT(I2C1->CR1 , I2C_CR1_STOP);
    }
    
    I2C_ERR _i2c_send_byte(uint8_t byte)
    {
        I2C1->DR = byte;
        
        while( !(I2C1->SR1 & I2C_SR1_TXE) )
        {
            if(I2C1->SR1 & I2C_SR1_AF)
            {
                I2C1->SR1 &=~ I2C_SR1_AF;
                return(I2C_ACK_FAIL);
            }
        }

        return(I2C_OK);
    }
    
    uint8_t _i2c_receive_byte(void)
    {
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        
        while( !(I2C1->SR1 & I2C_SR1_RXNE) ){}
            
        return(I2C1->DR);
    }
    
    uint8_t _i2c_reseive_last_byte(void)
    {
        SET_BIT(I2C1->CR1 , !I2C_CR1_ACK);
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        
        while( !(I2C1->SR1 & I2C_SR1_RXNE) ){}
            
        return(I2C1->DR);
    }
#endif
//---------------------------------------------------------------------------------------------------------
// SPI part
//---------------------------------------------------------------------------------------------------------

#ifdef _AVR   
    void _spi_init(void)
    {
      //
      SPI_DDRX |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS);
      SPI_DDRX &= ~(1<<SPI_MISO);
      
      SPI_PORTX |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS)|(1<<SPI_MISO);
       
      //
      SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(0<<SPR0);
     // SPSR = (1<<SPI2X);
    }

    //
    void _spi_writeByte(uint8_t data)
    {
       SPDR = data; 
       while(!(SPSR & (1<<SPIF)));
    }

    //
    uint8_t _spi_readByte(void)
    {  
       SPDR = 0xff;
       while(!(SPSR & (1<<SPIF)));
       return SPDR; 
    }

    //
    uint8_t _spi_writeReadByte(uint8_t data)
    {  
       SPDR = data;
       while(!(SPSR & (1<<SPIF)));
       return SPDR; 
    }

    //
    void _spi_writeArray(uint8_t num, uint8_t *data)
    {
       while(num--){
          SPDR = *data++;
          while(!(SPSR & (1<<SPIF)));
       }
    }

    //
    void _spi_writeReadArray(uint8_t num, uint8_t *data)
    {
       while(num--){
          SPDR = *data;
          while(!(SPSR & (1<<SPIF)));
          *data++ = SPDR; 
       }
    }
#endif
    
#ifdef _STM32
    void _spi_init()
    {
        GPIO_InitTypeDef GPIO_InitStruct;
        SPI_InitTypeDef SPI1_InitStruct;
        // Enable clock for SPI1 unit
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
        
        // Configure SPI pins
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
        GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
        GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;   	
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
        GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        // Configure SPI1
        SPI1_InitStruct.SPI_Mode = SPI_Mode_Master;
        SPI1_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
        SPI1_InitStruct.SPI_DataSize = SPI_DataSize_8b;
        SPI1_InitStruct.SPI_NSS = SPI_NSS_Soft;
        SPI1_InitStruct.SPI_CPOL = SPI_CPOL_Low;
        SPI1_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
        SPI1_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
        SPI1_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
        SPI_Init(SPI1, &SPI1_InitStruct);
        SPI_Cmd(SPI1, ENABLE);
    }
    
    void _spi_writeByte(uint8_t data)
    {
        SPI_I2S_SendData(SPI1, data);
        while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));
    }
    uint8_t _spi_readByte(void)
    {
        SPI_I2S_SendData(SPI1, 0xff);
        while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));       
        return((uint8_t)SPI_I2S_ReceiveData(SPI1));
    }
    //
    uint8_t _spi_writeReadByte(uint8_t data)
    {
        SPI_I2S_SendData(SPI1, data);
        while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));       
        return((uint8_t)SPI_I2S_ReceiveData(SPI1));     
    }
    //
    void _spi_writeArray(uint8_t num, uint8_t *data)
    {
       while(num--){
           SPI_I2S_SendData(SPI1, *data);
           while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE)); 
           data++;
       }
    }
    //
    void _spi_writeReadArray(uint8_t num, uint8_t *data)
    {
       while(num--){
           SPI_I2S_SendData(SPI1, *data);
           while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));       
           *data = ((uint8_t)SPI_I2S_ReceiveData(SPI1)); 
           data++;           
       }
    }
#endif

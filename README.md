# Primitive_WAV_player-Prj.Mercury-
It's  WAV player on STM32F103C8 microcontroller.

Alpha version:

    Direct port of Mercury project from AVR to STM32 without inners logic rebuild.

  Description:
  
    Primitive wav player on STM32F103C8.
    Automaticaly reads some needed data from RIFF chain on begin of file.
    - File size;
    - Sample rate;
    - Channels amount;
  
    This parameters automaticaly applies befor file start play.
    Can works only with 8-bit unsigned int PCM sound;
    One and two channels(8000 - 44100 Samples/s for 1 Channel and 8000 - 16000 Samples/s for 2 channels).
    Two playing modes APM (plays files only from root dir) and CCPM mode (plays files from CCPM dir, controled only by UART)
  
  Features:

	  Auto playing mode(APM)
		  UART commands: Prev = 'B', Play/Pause = 'P', Next = 'N'
		  If you want to activate this mode press any key or send UARD command('B', 'P' or 'N')
		
    Command controllable playing mode(CCPM)
      Available UART commands:
      'S' - Start line
      '.' - End line and play
      'O' - Enable output mode
      'F' - Finish CCPM
      'R' - Read CCPM DIR  
      '<' - Stop line reading
      'B' - Previous track (only for APM)
      'P' - Play / Pause	 (only for APM)
      'N' - Next track	 (only for APM)
    If you want to enable CCPM mode you need send 'S' command and then name (without.WAV) and '.' - end line command
    after, file with chosen name will play from CCPM dir on SD card. If the file will finish player will return 'F'.
    If file with chosen name is not existed, player will return 'E' and will go to IDLE state.
    If you want to abort CCPM playing send 'F' command.

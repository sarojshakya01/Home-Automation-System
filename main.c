/*
 * Smart_Home_Security_Final_Code.c
 *
 * Created: 7/22/2014 9:28:20 AM
 * Author: Saroj Shakya
 * Copyright: Saroj Shakya and His Team
 */ 

//NOTE 1: to active voice_call, replace '/*#' by '///*#' and '#*/' by '//#*/'  
//		  (press Ctrl+A, Ctrl+H, Find what: /*#, Replace with: ///*#, click 'Replace all')
//		  (press Ctrl+A, Ctrl+H, Find what: #*/, Replace with: //#*/, click 'Replace all')
//NOTE 2: to active send_message, replace '//send_message' by 'send_message'
//		  (press Ctrl+A, Ctrl+H, Find what: //send_message, Replace with: send_message, click 'Replace all')

//---Code starts from here---//

#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>

//---LCD Port Definition---//
#define LCD_DATA_PORT PORTA
#define LCD_DATA_DDR DDRA
#define LCD_RS 2
#define LCD_EN 3
//-------------------------//

//---Input Port Definition---//
#define INPUT_DDR DDRB
#define INPUT_PIN PINB
#define INPUT_PORT PORTB
//-------------------------//

//---Keypad port definition---//
#define KEY_PAD_DDR DDRC
#define KEY_PAD_ROW PORTC
#define KEY_PAD_COLUMN PINC
//-------------------------//

//---Output port definition---//
#define OUTPUT_PORT PORTD
#define OUTPUT_DDR DDRD
//-------------------------//

//---delay time definition---//
#define sensing_duration 3000
#define missed_call_gap 4000
#define call_duration 15000
#define buzzer_led_delay 60
#define GSM_operation_delay 800
#define motor_rotating_time 4000
#define door_opening_time 5000
//-------------------------//

//#define num_Devindra +9779843715762
//#define num_Narayan +9779849914078
//#define num_Sagar +9779841143577
//#define num_Saroj +9779841491322

int main();		//main function definition due to former function call
void door_open_close(); //door_open_close function definition due to former function call

//---8 bit variable declaration---//
unsigned char  Fire, LPG, Gate, Fire_LPG, Fire_Gate, LPG_Gate, Fire_LPG_Gate, temp_input, owner, call_main_count=0;
unsigned char keyPressed_count, skip_set, do_compare, n, change_password_value, password_exist=1;
unsigned char eeprom_data_a, abc, wrong_password_count, first_time, OK=1;
unsigned char read_SMS, skip_display_SMS;
//-------------------------//

//---Array declaration---//
unsigned char set_password[16]; //array to store existing password
unsigned char entered_password[16]; //array to store entered password
unsigned char received_data[200]; //array to store received data
unsigned char open_door[10] = "OPEN DOOR";
unsigned char cloz_door[10] = "CLOZ DOOR";
//-------------------------//

//---LCD Code starts from here---//
void LCD_cmnd(unsigned char cmnd) //function to send command to LCD Module
{
	LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (cmnd & 0xF0); //send upper 4 bit
	LCD_DATA_PORT &= ~(1<<LCD_RS); //0b11111011 //RS = 0
	LCD_DATA_PORT |= 1<<LCD_EN; //0b00001000 //EN = 1
	_delay_us(50);
	LCD_DATA_PORT &= ~(1<<LCD_EN); //0b11110111 //EN = 0
	_delay_us(200);
	LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (cmnd << 4); //send lower 4 bit
	LCD_DATA_PORT |= 1<<LCD_EN; //0b00001000 //EN = 1
	_delay_us(50);
	LCD_DATA_PORT &= ~(1<<LCD_EN); //0b11110111 //EN = 0
}
void LCD_data(unsigned char data) //Function to send data to LCD Module
{
	LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (data & 0xF0);
	LCD_DATA_PORT |= 1<<LCD_RS; //0b00000100 //RS = 1
	LCD_DATA_PORT |= 1<<LCD_EN; //0b00001000
	_delay_us(50);
	LCD_DATA_PORT &= ~(1<<LCD_EN); //0b11110111
	_delay_us(2000);
	LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (data << 4);
	LCD_DATA_PORT |= 1<<LCD_EN; //0b00001000
	_delay_us(2000);
	LCD_DATA_PORT &= ~(1<<LCD_EN); //0b11110111
}
void LCD_initialize(void) //Function to initialize LCD Module
{
	LCD_DATA_DDR = 0xFC;
	LCD_DATA_PORT &= ~(1<<LCD_EN); //0b11110111;
	_delay_ms(200);
	LCD_cmnd(0x33);
	_delay_ms(20);
	LCD_cmnd(0x32);
	_delay_ms(20);
	LCD_cmnd(0x28);
	_delay_ms(20);
	LCD_cmnd(0x0E);
	_delay_ms(20);
	LCD_cmnd(0x01);
	_delay_ms(20);
}
void LCD_clear(void) //Function to clear the LCD Screen
{
	LCD_cmnd(0x01);
	_delay_ms(2);
}
void LCD_print(char * str) //Function to print the string on LCD Screen
{
	unsigned char i=0;
	while(str[i] != 0)
	{
		LCD_data(str[i]);
		i++;
		_delay_ms(5);
	}
}
void LCD_set_curser(unsigned char y, unsigned char x) //Function to move the Curser at (y,x) position
{
	if(y==1)
	LCD_cmnd(0x7F+x);
	else if(y==2)
	LCD_cmnd(0xBF+x);
}
void LCD_num(unsigned char num) //Function to display number
{
	//LCD_data(num/100 + 0x30);
	//num = num%100;
	LCD_data(num/10 + 0x30);
	LCD_data(num%10 + 0x30);
}
//---LCD Code ends here---//

//---Port initialization function definitions---//
void port_initialize()//Input and Output initialization//
{
	INPUT_DDR = 0b00100100;
	OUTPUT_DDR = 0xFC;
}
void key_pad_initialize() //keypad initialization
{
	KEY_PAD_DDR = 0x0F; //make row output and column input
	_delay_ms(20);		
	KEY_PAD_ROW = 0xFF; //pull up the input pin
	_delay_ms(20);
	KEY_PAD_COLUMN = 0x00;
	_delay_ms(20);
}
//-------------------------//

//---UART related function definition---//
void usart_initialize()//USART initialization//
{
	UCSRB = (1<<TXEN) | (1<<RXEN); //enable tx and rx pin
	UCSRC = (1<<UCSZ0)|(1<<UCSZ1)|(1<<URSEL); //character size 8, 1 stop bit and reg select bit = 1
	UBRRL = 0x67; //baud rate 9600
}
void usart_send_char(unsigned char txdata)//Function to send single character serially//
{
	while(!(UCSRA&(1<<UDRE)));
	UDR = txdata;
}
void usart_send_string(char *str)//Function to send string serially//
{
	unsigned char i=0;
	while(str[i] != 0)
	{
		usart_send_char(str[i]);
		i++;
		_delay_ms(5);
	}
}
unsigned char usart_receive_char()
{
	while( !(UCSRA & (1<<RXC)) );
	return UDR;
}
void usart_receive_string()
{
	unsigned char i=0;
	while(i != 90)
	{
		received_data[i] = usart_receive_char();
		i++;
	}
}
//-------------------------//

//---GSM related functions---//
void dc_motor_forward();
void dc_motor_backward();
void send_message()//Function to send the message
{
	usart_initialize();
	LCD_clear();
	LCD_print("SMS Sending... ");
	LCD_set_curser(2,1);
	LCD_print("To +9779841491322");
	usart_send_string("AT\r\n");		// Transmit AT to the module – GSM Modem sendsOK
	_delay_ms(GSM_operation_delay);
	usart_send_string("ATE0\r\n");		// Echo Off
	_delay_ms(GSM_operation_delay);
	usart_send_string("AT+CMGF=1\r\n");	// Switch to text mode
	_delay_ms(GSM_operation_delay);
	usart_send_string("AT+CMGS=\"+9779841491322\"\r\n"); // Send SMS to a cell number
	_delay_ms(GSM_operation_delay);
	usart_send_string("Message From Home Security System:\n"); // Input SMS Data
	if (Fire == 1)			usart_send_string("Fire alert!!!");
	if (LPG == 1)			usart_send_string("LPG alert!!!");
	if (Gate == 1)			usart_send_string("Gate alert!!!");
	if (Fire_LPG == 1)		usart_send_string("Fire+LPG alert!!!");
	if (Fire_Gate == 1)		usart_send_string("Fire+Gate alert!!!");
	if (LPG_Gate == 1)		usart_send_string("LPG+Gate alert!!!");
	if (Fire_LPG_Gate == 1)	usart_send_string("Fife+LPG+Gate alert!!!");
	usart_send_char(0x1A);			// Ctrl-Z indicates end of SMS
	LCD_clear();
	LCD_print(" Message Sent  ");
	LCD_set_curser(2,1);
	LCD_print("Successfully!!!");
	_delay_ms(1500);
}
void voice_call() //Function for voice call
{
	usart_initialize();
	usart_send_string("AT\r\n");		// Transmit AT to the module – GSM Modem sendsOK
	_delay_ms(GSM_operation_delay);
	LCD_clear();
	if ((INPUT_PIN & 0x13) != temp_input) main();
	LCD_print("Calling...  ");
	LCD_set_curser(2,2);
	switch(owner)
	{
		case 1:
		LCD_print("+9779849914078");
		usart_send_string("ATE0\r\n");		// Echo Off
		_delay_ms(GSM_operation_delay);
		usart_send_string("ATD+9779849914078;\r\n"); // Voice call to a cell number
		break;
		
		case 2:
		LCD_print("+9779843715762");
		usart_send_string("ATE0\r\n");		// Echo Off
		_delay_ms(GSM_operation_delay);
		usart_send_string("ATD+9779843715762;\r\n"); // Voice call to a cell number
		break;
		
		default:
		LCD_print("+9779841491322");
		usart_send_string("ATE0\r\n");		// Echo Off
		_delay_ms(GSM_operation_delay);
		usart_send_string("ATD+9779841491322;\r\n"); // Voice call to a cell number
	}
	_delay_ms(6000);
	LCD_clear();
	LCD_print("     Done!!!");
}
void call_abort() //Function to abort the  current call
{
	usart_initialize();
	usart_send_string("AT\r\n");		// Transmit AT to the module – GSM Modem sendsOK
	_delay_ms(GSM_operation_delay);
	usart_send_string("ATE0\r\n");		// Echo Off
	_delay_ms(GSM_operation_delay);
	LCD_clear();
	usart_send_string("ATH\r\n"); //End call
	LCD_print("  Call Aborted");
	_delay_ms(GSM_operation_delay);
}
void read_message()
{
	usart_send_string("AT\r\n");		// Transmit AT to the module – GSM Modem sendsOK
	_delay_ms(500);
	//usart_send_string("ATE0\r\n");		// Echo Off
	_delay_ms(500);
	if ((INPUT_PIN & 0x13) != temp_input) main();
	usart_send_string("AT+CMGF=1\r\n");	// Switch to text mode
	_delay_ms(500);
	usart_send_string("AT+CMGR=1\r\n"); // Read Message from inbox
	//usart_send_string("AT+CMGL=\"ALL\"\r\n");
	usart_receive_string();
	LCD_print("a");
	read_SMS = 1;
}
void delete_message()
{
	usart_send_string("AT\r\n");		// Transmit AT to the module – GSM Modem sendsOK
	_delay_ms(500);
	usart_send_string("ATE0\r\n");		// Echo Off
	_delay_ms(500);
	usart_send_string("AT+CMGF=1\r\n");	// Switch to text mode
	_delay_ms(500);
	usart_send_string("AT+CMGD=1\r\n");
}
void display_message()
{
	unsigned char i,j=0;
	LCD_clear();
	LCD_print("Message From:");
	LCD_set_curser(2,1);
	for (i=35;i<49;i++)
	{
		LCD_data(received_data[i]);
	}
	_delay_ms(3000);
	
	LCD_clear();
	LCD_print("Received SMS is:");
	LCD_set_curser(2,3);
	for (i=78;i<87;i++)
	{
		LCD_data(received_data[i]);
	}
	_delay_ms(2000);
	
	if (skip_display_SMS != 2)
	{
		for (i=78;i<87;i++)
		{
			if (received_data[i] == open_door[j]) j++;
			else break;
		}
		if(j==9) dc_motor_backward();
		j=0;
		
		for (i=78;i<87;i++)
		{
			if (received_data[i] == cloz_door[j]) j++;
			else break;
		}
		if(j==9) dc_motor_forward();
		j=0;
	}
	
	LCD_clear();
	LCD_print("Received Date:");
	LCD_set_curser(2,1);
	for (i=55;i<69;i++)
	{
		LCD_data(received_data[i]);
	}
	skip_display_SMS = 2;
	_delay_ms(1500);
}
//-------------------------//
 
//---Function to initiate and stop buzzer---//
void stop_buzzer()//Function to stop the buzzer beep
{
	while(1)
	{
		unsigned temp = INPUT_PIN & 0b10010011;
		LCD_clear();
		OUTPUT_PORT &= 0x7F;
		LCD_print("INVESTIGATING...");
		LCD_set_curser(2,1);
		LCD_print("SYSTEM WORKING..");
		_delay_ms(2000);
		while((INPUT_PIN & 0x87) == temp);
		break;
	}
	main();
}
void on_buzzer()//Function to start the buzzer beep
{
	OUTPUT_PORT = 0x80;
	temp_input =  0x13 & INPUT_PIN;
}
void on_buzzer_led()//Function to on the buzzer and led
{
	OUTPUT_PORT = 0xC0;
	temp_input =  0x13 & INPUT_PIN;
}
void on_buzzer_fan()//Function to on the buzzer and fan
{
	OUTPUT_PORT = 0xA0;
	temp_input =  0x13 & INPUT_PIN;
}
void on_buzzer_led_fan()//Function to on the buzzer,led and fan
{
	OUTPUT_PORT = 0xE0;
	temp_input =  0x13 & INPUT_PIN;
}
//-------------------------//

//---Function Declaration due to former call---//
void buzzer();
void buzzer_led();
void buzzer_fan();
void buzzer_led_fan();
//---Function Declaration ends here---//

//---continuous led blink, buzzer bip and fan on function---//
void buzzer()//Function to beep the buzzer continuously
{
	while(1)
	{
		OUTPUT_PORT = 0x80; //enable buzzer
		_delay_ms(buzzer_led_delay);
		if ((INPUT_PIN &= 0b10000000) == 0x80) door_open_close();
		if ((INPUT_PIN &= 0b00001000) == 0x08) buzzer_led();
		OUTPUT_PORT = 0x00; //disable buzzer
		_delay_ms(buzzer_led_delay);
		if ((INPUT_PIN &= 0x13) != temp_input) break; //if previous input is not equal to updated input then break
		INPUT_PORT = 0x40; //for enabling the stop_buzzer input
		if ((INPUT_PIN &= 0b01000000) == 0x40) stop_buzzer(); //if stop_buzzer input is low then call stop_buzzer function
	}
	OUTPUT_PORT &= 0x03;
}
void buzzer_led()//Function to beep the buzzer and blink the led continuously
{
	while(1)
	{
		OUTPUT_PORT = 0b11000000; //enable buzzer and enable led
		_delay_ms(buzzer_led_delay);
		OUTPUT_PORT = 0b01000000; //disable buzzer
		_delay_ms(buzzer_led_delay);
		if ((INPUT_PIN &= 0b10000000) == 0x80) door_open_close();
		OUTPUT_PORT = 0b10000000; //enable buzzer and disable led
		if ((INPUT_PIN &= 0b00001000) == 0x00) buzzer();
		_delay_ms(buzzer_led_delay);
		OUTPUT_PORT = 0b00000000; //disable buzzer
		_delay_ms(buzzer_led_delay);
		if((INPUT_PIN &= 0x13) != temp_input)  break;
		INPUT_PORT = 0x40; //for enabling the stop_buzzer input
		if ((INPUT_PIN &= 0b01000000) == 0x40) stop_buzzer(); //if stop_buzzer input is low then call stop_buzzer function
	}
	OUTPUT_PORT &= 0x03;
}
void buzzer_fan()//Function to beep the buzzer and on the fan continuously
{
	while(1)
	{
		OUTPUT_PORT = 0xA0; //enable buzzer and enable fan
		_delay_ms(buzzer_led_delay);
		if ((INPUT_PIN &= 0b10000000) == 0x80) door_open_close();
		if ((INPUT_PIN &= 0b00001000) == 0x08) buzzer_led_fan();
		OUTPUT_PORT = 0x20; //disable buzzer and keeping fan enable
		_delay_ms(buzzer_led_delay);
		if ((INPUT_PIN &= 0x13) != temp_input) break; //if previous input is not equal to updated input then break
		INPUT_PORT = 0x40; //for enabling the stop_buzzer input
		if ((INPUT_PIN &= 0b01000000) == 0x40) stop_buzzer(); //if stop_buzzer input is low then call stop_buzzer function
	}
	OUTPUT_PORT &= 0x03;
}
void buzzer_led_fan()//Function to beep the buzzer, blink the led and on the fan continuously
{
	while(1)
	{
		OUTPUT_PORT = 0xE0; //enable buzzer and enable led and enable fan
		_delay_ms(buzzer_led_delay);
		OUTPUT_PORT = 0x60; //disable buzzer
		_delay_ms(buzzer_led_delay);
		if ((INPUT_PIN &= 0b10000000) == 0x80) door_open_close();
		if ((INPUT_PIN &= 0b00001000) == 0x00) buzzer_fan();
		OUTPUT_PORT = 0xA0; //enable buzzer and disable led
		_delay_ms(buzzer_led_delay);
		OUTPUT_PORT = 0x20; //disable buzzer
		_delay_ms(buzzer_led_delay);
		if((INPUT_PIN &= 0x13) != temp_input)  break;
		INPUT_PORT = 0x40; //for enabling the stop_buzzer input
		if ((INPUT_PIN &= 0b01000000) == 0x40) stop_buzzer(); //if stop_buzzer input is low then call stop_buzzer function
	}
	OUTPUT_PORT &= 0x03;
}
//-------------------------//

//---Function to take input from sensor---//
void read_sensor()//Function to read the data from sensors
{
	unsigned char  input;;
	switch(INPUT_PIN & 0b10011011)
	{
		case (0b00000001): input = '1';	Fire = 1;
		break;
		case (0b00000010): input = '2';	LPG = 1;
		break;
		case (0b00010000): input = '3';	Gate = 1;
		break;
		case (0b00001001): input = '4';	Fire = 1;
		break;
		case (0b00001010): input = '5';	LPG = 1;
		break;
		case (0b00011000): input = '6';	Gate = 1;
		break;
		case (0b00000011): input = 'A';	Fire_LPG = 1;
		break;
		case (0b00010001): input = 'B';	Fire_Gate = 1;
		break;
		case (0b00010010): input = 'C';	LPG_Gate = 1;
		break;
		case (0b00001011): input = 'D';	Fire_LPG = 1;
		break;
		case (0b00011001): input = 'E';	Fire_Gate = 1;
		break;
		case (0b00011010): input = 'F';	LPG_Gate = 1;
		break;
		case (0b00010011): input = 'G';	Fire_LPG_Gate = 1;
		break;
		case (0b00011011): input = 'H';	Fire_LPG_Gate = 1;
		break;
		case (0b10000000): input = 'S';
		break;
		case (0b10000001): input = 'S';
		break;
		case (0b10000010): input = 'S';
		break;
		case (0b10010000): input = 'S';
		break;
		case (0b10001001): input = 'S';
		break;
		case (0b10001010): input = 'S';
		break;
		case (0b10011000): input = 'S';
		break;
		case (0b10000011): input = 'S';
		break;
		case (0b10010001): input = 'S';
		break;
		case (0b10010010): input = 'S';
		break;
		case (0b10001011): input = 'S';
		break;
		case (0b10011001): input = 'S';
		break;
		case (0b10011010): input = 'S';
		break;
		case (0b10010011): input = 'S';
		break;
		case (0b10011011): input = 'S';
		break;
		case (0b10001000): input = 'S';
		break;
		default: input = 'X';
	}
	if (input == 'S') door_open_close();
	
	//Condition Start For Day Time Single Event//
	if (input == '1')
	{
		_delay_ms(sensing_duration);
		if(INPUT_PIN == 0b00000001)
		{		
			owner = 1;
			LCD_clear();
			LCD_print("   Fire Alert");
			on_buzzer();
			///*#	
			voice_call();
			_delay_ms(call_duration);
			call_abort();
			_delay_ms(missed_call_gap);
			voice_call();
			owner = 0;
			_delay_ms(call_duration);
			call_abort();
			send_message();
			LCD_clear();
			LCD_print("   Fire Alert");
			//#*/
			buzzer();
			LCD_clear();
		}
	}
	if (input == '2')
	{
		_delay_ms(sensing_duration);
		if(INPUT_PIN == 0b00000010)
		{
			LCD_clear();
			LCD_print("   LPG Alert");
			on_buzzer_fan();
			/*#	
			voice_call();
			_delay_ms(call_duration);
			call_abort();
			_delay_ms(missed_call_gap);
			voice_call();
			_delay_ms(call_duration);
			call_abort();
			send_message();
			LCD_clear();
			LCD_print("   LPG Alert");
			#*/
			buzzer_fan();
			LCD_clear();
		}
	}
	if (input == '3')
	{
		owner = 2;
		LCD_clear();
		LCD_print("   Gate Alert");
		on_buzzer();
		/*#	
		voice_call();
		_delay_ms(call_duration);
		call_abort();
		_delay_ms(missed_call_gap);
		voice_call();
		owner = 0;
		_delay_ms(call_duration);
		call_abort();
		send_message();
		LCD_clear();
		LCD_print("   Gate Alert");
		#*/
		buzzer();
		LCD_clear();
	}
	//Condition END For Day Time Single Event//
	
	//Condition Start For Night Time Single Event//
	if (input == '4')
	{
		_delay_ms(sensing_duration);
		if(INPUT_PIN == 0b00001001)
		{
			owner = 1;
			LCD_clear();
			LCD_print("   Fire Alert");
			on_buzzer_led();
			/*#	
			voice_call();
			_delay_ms(call_duration);
			call_abort();
			_delay_ms(missed_call_gap);
			voice_call();
			owner = 0;
			_delay_ms(call_duration);
			call_abort();
			send_message();
			LCD_clear();
			LCD_print("   Fire Alert");
			#*/
			buzzer_led();
			LCD_clear();
		}
	}
	if (input == '5')
	{
		_delay_ms(sensing_duration);
		if(INPUT_PIN == 0b00001010)
		{
			LCD_clear();
			LCD_print("   LPG Alert");
			on_buzzer_led_fan();
			/*#	
			voice_call();
			_delay_ms(call_duration);
			call_abort();
			_delay_ms(missed_call_gap);
			voice_call();
			_delay_ms(call_duration);
			call_abort();
			send_message();
			LCD_clear();
			LCD_print("   LPG Alert");
			#*/
			buzzer_led_fan();
			LCD_clear();
		}
	}
	if (input == '6')
	{
		owner = 2;
		LCD_clear();
		LCD_print("   Gate Alert");
		on_buzzer_led();
		/*#	
		voice_call();
		_delay_ms(call_duration);
		call_abort();
		_delay_ms(missed_call_gap);
		voice_call();
		owner = 0;
		_delay_ms(call_duration);
		call_abort();
		send_message();
		LCD_clear();
		LCD_print("   Gate Alert");
		#*/
		buzzer_led();
		LCD_clear();
	}
	//Condition END For Night Time Single Event//
	
	//Condition Start For Day Time Two Events//
	if (input == 'A')
	{
		_delay_ms(sensing_duration);
		if(INPUT_PIN == 0b00000011);
		{
			LCD_clear();
			LCD_print(" Fire+LPG Alert");
			on_buzzer();
			/*#	
			voice_call();
			_delay_ms(call_duration);
			call_abort();
			send_message();
			LCD_clear();
			LCD_print(" Fire+LPG Alert");
			#*/
			buzzer();
			LCD_clear();
		}
	}
	if (input == 'B')
	{
		LCD_clear();
		LCD_print("Fire+Gate Alert");
		on_buzzer();
		/*#	
		voice_call();
		_delay_ms(call_duration);
		call_abort();
		send_message();
		LCD_clear();
		LCD_print("Fire+Gate Alert");
		#*/
		buzzer();
		LCD_clear();
	}
	if (input == 'C')
	{
		LCD_clear();
		LCD_print(" LPG+Gate Alert");
		on_buzzer_fan();
		/*#	
		voice_call();
		_delay_ms(call_duration);
		call_abort();
		send_message();
		LCD_clear();
		LCD_print(" LPG+Gate Alert");
		#*/
		buzzer_fan();
		LCD_clear();
	}
	//Condition END For Day Time Two Events//
	
	//Condition Start For Night Time Two Events//
	if (input == 'D')
	{
		_delay_ms(sensing_duration);
		if(INPUT_PIN == 0b00001011);
		{
			LCD_clear();
			LCD_print(" Fire+LPG Alert");
			on_buzzer_led();
			/*#	
			voice_call();
			_delay_ms(call_duration);
			call_abort();
			send_message();
			LCD_clear();
			LCD_print(" Fire+LPG Alert");
			#*/
			buzzer_led();
			LCD_clear();
		}
	}
	if (input == 'E')
	{
		LCD_clear();
		LCD_print("Fire+Gate Alert");
		on_buzzer_led();
		/*#	
		voice_call();
		_delay_ms(call_duration);
		call_abort();
		send_message();
		LCD_clear();
		LCD_print("Fire+Gate Alert");
		#*/
		buzzer_led();
		LCD_clear();
	}
	if (input == 'F')
	{
		LCD_clear();
		LCD_print(" LPG+Gate Alert");
		on_buzzer_led_fan();
		/*#	
		voice_call();
		_delay_ms(call_duration);
		call_abort();
		send_message();
		LCD_clear();
		LCD_print(" LPG+Gate Alert");
		#*/
		buzzer_led_fan();
		LCD_clear();
	}
	//Condition END For Night Time Two Events//
	
	//Condition Start For Day Time All Events//
	if (input == 'G')
	{
		LCD_clear();
		LCD_print("Fire+LPG+Gate");
		LCD_set_curser(2,5);
		LCD_print("Alert");
		on_buzzer();
		/*#	
		voice_call();
		_delay_ms(call_duration);
		call_abort();
		send_message();
		LCD_clear();
		LCD_print("Fire+LPG+Gate");
		LCD_set_curser(2,5);
		LCD_print("Alert");
		#*/
		buzzer();
		LCD_clear();
	}
	//Condition END For Day Time All Events//
	
	//Condition Start For Night Time All Events//
	if (input == 'H')
	{
		LCD_clear();
		LCD_print("Fire+LPG+Gate");
		LCD_set_curser(2,5);
		LCD_print("Alert");
		on_buzzer_led();
		/*#	
		voice_call();
		_delay_ms(call_duration);
		call_abort();
		send_message();
		LCD_clear();
		LCD_print("Fire+LPG+Gate");
		LCD_set_curser(2,5);
		LCD_print("Alert");
		#*/
		buzzer_led();
		LCD_clear();
	}
	if (input == 'X')
	{
		main();
	}
}
//-------------------------//

//---Function to rotate motor motor---//
void dc_motor_forward()//Function to drive the motor in forward direction
{
	LCD_clear();
	LCD_print("Closing.....");
	OUTPUT_PORT |= 0x04;
	_delay_ms(motor_rotating_time);
	OUTPUT_PORT &= 0xFB;
}
void dc_motor_backward()//Function to drive the motor in backward direction
{
	LCD_clear();
	LCD_print("Opening.....");
	OUTPUT_PORT |= 0x08;
	_delay_ms(motor_rotating_time);
	OUTPUT_PORT &= 0xF7;
}
//-------------------------//

//---Function to produce keypressed tone---//
void keypressed_tone()
{
	OUTPUT_PORT |= 0x80;
	_delay_ms(100);
	OUTPUT_PORT &= 0x7F;
	_delay_ms(100);
}
void keypressed_tone1()
{
	unsigned int i;
	for (i=0; i<500; i++)
	{
		OUTPUT_PORT |= 0x80;
		_delay_us(65);
		OUTPUT_PORT &= 0x7F;
		_delay_us(35);
	}
}
void keypressed_tone2()
{
	unsigned int i;
	for (i=0; i<500; i++)
	{
		OUTPUT_PORT |= 0x80;
		_delay_us(80);
		OUTPUT_PORT &= 0x7F;
		_delay_us(20);
	}
}
void keypressed_tone3()
{
	unsigned int i;
	for (i=0; i<500; i++)
	{
		OUTPUT_PORT |= 0x80;
		_delay_us(90);
		OUTPUT_PORT &= 0x7F;
		_delay_us(10);
	}
}
void buzzer_alert()
{
	unsigned char i;
	for (i=0; i<30; i++)
	{
		OUTPUT_PORT |= 0x80;
		_delay_ms(buzzer_led_delay);
		OUTPUT_PORT &= 0x7F;
		_delay_ms(buzzer_led_delay);
		
		OUTPUT_PORT |= 0x80;
		_delay_ms(buzzer_led_delay);
		OUTPUT_PORT &= 0x7F;
		_delay_ms(buzzer_led_delay);
		
		OUTPUT_PORT |= 0x80;
		_delay_ms(buzzer_led_delay);
		OUTPUT_PORT &= 0x7F;
		_delay_ms(buzzer_led_delay+buzzer_led_delay);
	}
}
//-------------------------//

//---Function to write and read eeprom---//
void write_data_at_0x00(unsigned char data_at_0x00)
{
	EEAR = 0x00;
	while(EECR & (1<<EEWE));
	EEDR = data_at_0x00;
	EECR |= (1<<EEMWE);
	EECR |= (1<<EEWE);
}
void write_data_at_0x03(unsigned char data_at_0x03)
{
	EEAR = 0x03;
	while(EECR & (1<<EEWE));
	EEDR = data_at_0x03;
	EECR |= (1<<EEMWE);
	EECR |= (1<<EEWE);
}
void write_data_a_in_eeprom(unsigned char value_a)
{
	EEAR = 0x05;
	while(EECR & (1<<EEWE));
	EEDR = value_a;
	EECR |= (1<<EEMWE);
	EECR |= (1<<EEWE);
}
unsigned char read_data_from_0x00()
{
	EEAR=0x00;
	while(EECR&(1<<EEWE));
	EECR|=(1<<EERE);
	return EEDR;
}
unsigned char read_data_from_0x03()
{
	EEAR=0x03;
	while(EECR&(1<<EEWE));
	EECR|=(1<<EERE);
	return EEDR;
}
void read_data_a_from_eeprom()
{
	EEAR = 0x05;
	while(EECR&(1<<EEWE));
	EECR|=(1<<EERE);
	eeprom_data_a=EEDR;
}
void read_password()
{
	unsigned char i=0;
	for(i=0; i<=eeprom_data_a; i++)
	{
		EEAR = i+0x10;
		while (EECR & (1<<EEWE));
		EECR |= (1<<EERE);
		set_password[i] = EEDR;
		//LCD_data(set_password[i]);
		_delay_ms(50);
	}
}
void save_password()
{
	unsigned char i;
	for (i=0;i<n;i++)
	{
		EEAR=i+0x10;
		while(EECR&(1<<EEWE));
		EEDR=set_password[i];
		EECR|=(1<<EEMWE);
		EECR|=(1<<EEWE);
		_delay_ms(10);
	}
}
//-------------------------//

//---Functions for keypad input, setting password and comparing password---//
void compare_password()//Function to compare the password
{
	unsigned char i,j=0;
	LCD_cmnd(0x0C);
	if(do_compare == 100 && OK == 2) //if there is a existing password then compare else not
	{
		OK = 1;
		for(i=0; i<keyPressed_count; i++)
		{
			if(entered_password[i] == set_password[i]) j++;
			else j--;
		}
		_delay_ms(100);
		do_compare = 50;
		
		if(j == n)
		{
			change_password_value++;
			if(change_password_value == 5) door_open_close();
			else
			{
				change_password_value--;
				for(i=0;i<16;i++)
				{
					entered_password[i]='*';
				}
				dc_motor_backward();
				LCD_clear();
				LCD_print("   WELCOME!!!");
				_delay_ms(door_opening_time);
				dc_motor_forward();
				LCD_clear();
			}
			main();
		}
		else
		{
			wrong_password_count++;
			if (wrong_password_count == 4)
			{
				LCD_clear();
				LCD_print("INVALID Password");
				LCD_print("Don't Try Again!");
				buzzer_alert();
				//voice_call();
				//_delay_ms(call_duration);
				//call_abort()
				main();
			}
			LCD_clear();
			LCD_print("INVALID Password");
			LCD_set_curser(2,3);
			LCD_print("Try Again...");
			_delay_ms(600);
			door_open_close();
		}
	}
	else;
}
void input_key()//Function to get the input from keypad
{
	unsigned char column_value, keyCode, keyPressed = 0, i;
	column_value = 0xFF;
	for(i=0; i<4; i++)
	{
		_delay_ms(1);
		KEY_PAD_ROW = ~(0x01 << i);
		_delay_ms(1);  		  	 		  //delay for port o/p settling
		column_value = KEY_PAD_COLUMN | 0x0F;
		if (column_value != 0xFF)
		{
			_delay_ms(20);
			LCD_cmnd(0x0E);		  		 //for de-bounce
			column_value = KEY_PAD_COLUMN | 0x0F;
			if(column_value == 0xFF) goto OUT;
			keyCode = (column_value & 0xF0) | (0x0F & ~(0x01 << i));
			while (column_value != 0xFF)
			column_value = KEY_PAD_COLUMN | 0x0F;
			_delay_ms(20);   			   //for de-bounce
			switch (keyCode)
			{
				case (0xEE): keyPressed = '7'; keypressed_tone3();
				break;
				case (0xED): keyPressed = '4'; keypressed_tone2();
				break;
				case (0xEB): keyPressed = '1'; keypressed_tone1();
				break;
				case (0xE7): keyPressed = '*';
				break;
				case (0xDE): keyPressed = '8'; keypressed_tone3();
				break;
				case (0xDD): keyPressed = '5'; keypressed_tone2();
				break;
				case (0xDB): keyPressed = '2'; keypressed_tone1();
				break;
				case (0xD7): keyPressed = '0'; keypressed_tone1();
				break;
				case (0xBE): keyPressed = '9'; keypressed_tone3();
				break;
				case (0xBD): keyPressed = '6'; keypressed_tone2();
				break;
				case (0xBB): keyPressed = '3'; keypressed_tone1();
				break;
				case (0xB7): keyPressed = '#';
				break;
				case (0x7E): keyPressed = 'A'; keypressed_tone3();
				break;
				case (0x7D): keyPressed = 'B'; keypressed_tone2();
				break;
				case (0x7B): keyPressed = 'C'; keypressed_tone1();
				break;
				case (0x77): keyPressed = 'D';
				break;
				default	   : keyPressed = 'X';
			}
			if (keyPressed=='1')
			{
				entered_password[keyPressed_count]='1';
				LCD_data('1');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed=='2')
			{
				entered_password[keyPressed_count]='2';
				LCD_data('2');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed=='3')
			{
				entered_password[keyPressed_count]='3';
				LCD_data('3');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed=='4')
			{
				entered_password[keyPressed_count]='4';
				LCD_data('4');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed=='5')
			{
				entered_password[keyPressed_count]='5';
				LCD_data('5');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed=='6')
			{
				entered_password[keyPressed_count]='6';
				LCD_data('6');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed=='7')
			{
				entered_password[keyPressed_count]='7';
				LCD_data('7');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed=='8')
			{
				entered_password[keyPressed_count]='8';
				LCD_data('8');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed=='9')
			{
				entered_password[keyPressed_count]='9';
				LCD_data('9');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed=='0')
			{
				entered_password[keyPressed_count]='0';
				LCD_data('0');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed=='A')
			{
				entered_password[keyPressed_count]='A';
				LCD_data('A');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed=='B')
			{
				entered_password[keyPressed_count]='B';
				LCD_data('B');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed=='C')
			{
				entered_password[keyPressed_count]='C';
				LCD_data('C');
				keyPressed_count++;
				LCD_cmnd(0x10);
				LCD_data('*');
				compare_password();
			}
			if (keyPressed == 'D')
			{
				wrong_password_count = 0;
				if (password_exist == 2)
				{
					LCD_clear();
					LCD_cmnd(0x0C);
					LCD_print("T0 CHANGE THE");
					LCD_set_curser(2,3);
					LCD_print("Password");
					_delay_ms(1000);
					OK=1;
					change_password_value = 4;
					door_open_close();
				}
				else
				{
					LCD_clear();
					LCD_cmnd(0x0C);
					LCD_print("First Set Your");
					LCD_set_curser(2,3);
					LCD_print("Password");
					_delay_ms(1000);
					OK = 1;
					skip_set = 1;
					door_open_close();
				}
			}
			if (keyPressed=='#')
			{
				if(keyPressed_count!=0)
				{
					LCD_cmnd(0x0E);
					LCD_cmnd(0x10);
					keyPressed_count--;
				}
			}
			if ( keyPressed=='*' )
			{
				if( keyPressed_count!=0 && abc!=1 )
				{
					LCD_clear();
					LCD_cmnd(0x0C);
					keypressed_tone();
					LCD_set_curser(1,7);
					LCD_print("OK");
					OK = 2;
					_delay_ms(500);
					compare_password();
				}
			}
		}
		OUT:;
	}
}
void door_open_close()//Function to open or close the door by entering password
{
	unsigned char a,i;
	LCD_clear();
	OUTPUT_PORT &= 0x03;
	key_pad_initialize();
	LCD_cmnd(0x0C);
	keyPressed_count = 0;
	skip_set = read_data_from_0x00();
	if(change_password_value == 4) goto change_password;
	if(change_password_value == 5) goto set_new_password;
	
	if(skip_set != 5)
	{
		set_new_password: //label to set new password
		write_data_at_0x00(5);
		skip_set = read_data_from_0x00();
		change_password_value = 0;
		abc = 0;
		LCD_clear();
		LCD_cmnd(0x0C);
		LCD_print("Set Password:");
		_delay_ms(200);
		LCD_set_curser(2,1);
		
		do
		{
			input_key(); //enter new password un till the OK button is pressed
		}while (OK == 1);
		
		OK = 2;
		
		n = keyPressed_count;
		write_data_a_in_eeprom(n);
		
		password_exist = 2;
		
		_delay_ms(100);
		LCD_clear();
		LCD_cmnd(0x0C);
		LCD_print("Ur Password is:");
		LCD_set_curser(2,1);
		
		for(i=0; i<n; i++) //for displaying entered number
		{
			set_password[i] = entered_password[i];
			a = set_password[i];
			LCD_data(a);
		}
		for(i=0;i<16;i++)
		{
			entered_password[i]='*';
		}
		save_password();
		_delay_ms(1000);
		main();
	}
	
	change_password: //label to change the existing password by entering previous password
	OK = 1;
	password_exist = 2;
	read_data_a_from_eeprom();
	n = eeprom_data_a;
	
	read_password();
	LCD_data('b');
	do_compare = 100;
	keyPressed_count = 0;
	LCD_clear();
	LCD_cmnd(0x0C);
	LCD_print("Enter Password:");
	LCD_set_curser(2,1);
	while(1)
	{
		abc = 2;
		input_key();
	}
}
//-------------------------//

//---Temperature reading function---//
void read_temperature()//Function to read temperature
{
	unsigned char adc_value,temperature;
	ADCSRA = 0x87; //Enable ADC and select clk/128
	ADMUX = 0xE0; //0b1110000, 11 for Vref=2.56, 1 for left justified,
	LCD_cmnd(0x0C);
	LCD_clear();
	LCD_print(" NOTHING HAPPEN");
	LCD_set_curser(2,4);
	LCD_print("TEMP:");
	LCD_set_curser(2,11);
	LCD_data(0xDF);
	LCD_data('C');
	while(1)
	{
		ADCSRA |= 1<<ADSC; //Start conversion in ADC
		while ((ADCSRA & (1<<ADIF)) == 0); //till the end of conversion
		adc_value = ADCH; //save the converted output
		if (adc_value < 10) //function to convert hex to decimal number
		{
			temperature = adc_value + 0x30;
			LCD_set_curser(2,9);
			LCD_data(temperature);
		}
		if (adc_value > 9 ) //if hex is greater than 9, need to separate the digit, done by this function
		{
			if(adc_value > 29)  OUTPUT_PORT = 0x20; else OUTPUT_PORT = 0x00;
			LCD_set_curser(2,9);
			LCD_num(adc_value);
		}
		else
		{
			LCD_clear();
			LCD_print("* TEMP. SENSOR *");
			LCD_set_curser(2,1);
			LCD_print("# NOT WORKING #");
			_delay_ms(600);
			main();
		}
/*
		read_message();
		//delete_message();
		if(read_SMS == 1) display_message();
		_delay_ms(2000);
		LCD_clear();
		LCD_print(" NOTHING HAPPEN");
		LCD_set_curser(2,4);
		LCD_print("TEMP:");
		LCD_set_curser(2,11);
		LCD_data(0xDF);
		LCD_data('C');
*/		
		if ((INPUT_PIN &= 0x93) != 0x00) break; //0b10010011 if any input data is received, while loop is broken
	}
}
//-------------------------//

//---main function---//
int main(void)
{
	Fire = LPG = Gate = Fire_LPG = Fire_Gate = LPG_Gate = Fire_LPG_Gate = 0;
	LCD_initialize();
	_delay_ms(20);
	usart_initialize();
	port_initialize();
	wrong_password_count = 0;
	_delay_ms(20);
	LCD_cmnd(0x0E);
	if(call_main_count == 0)
	{
		LCD_print("  WELCOME!!!");
		LCD_set_curser(2,1);
		/*#
		LCD_print("Initializing");
		_delay_ms(500);
		LCD_print("Initializing.");
		_delay_ms(500);
		LCD_set_curser(2,1);
		LCD_print("Initializing..");
		_delay_ms(500);
		LCD_set_curser(2,1);
		LCD_print("Initializing...");
		_delay_ms(500);
		LCD_set_curser(2,1);
		#*/
		LCD_print("Initializing....");
		_delay_ms(1000);
		call_main_count++;
	}
	owner = 0;
	while (1)
	{
		read_temperature();
		read_sensor();
	}
}
//---main function end---//

//---code ends here---//

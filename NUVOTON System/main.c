//
// The project of Yakir and Kfir 
//
// HC05 Bluetooth module
// pin1 : KEY   N.C
// pin2 : VCC   to Vcc +5V
// pin3 : GND   to GND
// pin4 : TXD   to NUC140 UART0-RX (GPB0)
// pin5 : RXD   to NUC140 UART0-TX (GPB1)
// pin6 : STATE N.C.
//
// Connecting traffic lights and sensors:
// Do not use: (UART0) GPB0, GPB1,(LCD) GPD14, GPD8, GPD9, GPD10, GPD11 
// It is not recommended to use: (BUZZER) GPB11, (INT0) GPB15, (7SEG) GPC4, GPC5, GPC6, GPC7, (4 LED) GPC12, GPC13, GPC14, GPC15, (RGB LED) GPA12, GPA13, GPA14
//
#include "Driver\DrvUART.h"
#include <stdbool.h>	
#include <math.h>
#include "Driver\DrvSYS.h"
#include "Driver\DrvGPIO.h"
#include "LCD_Driver.h"

#define  LEDLCD_ON    DrvGPIO_ClrBit(E_GPD, 14)
#define  LEDLCD_OFF   DrvGPIO_SetBit(E_GPD, 14)
#define  LED1_ON   DrvGPIO_SetBit(E_GPB, 4)
#define  LED2_ON   DrvGPIO_SetBit(E_GPB, 5)
#define  LED3_ON   DrvGPIO_SetBit(E_GPB, 6)
#define  LED4_ON   DrvGPIO_SetBit(E_GPB, 7)
#define  LED1_OFF  DrvGPIO_ClrBit(E_GPB, 4)
#define  LED2_OFF  DrvGPIO_ClrBit(E_GPB, 5)
#define  LED3_OFF  DrvGPIO_ClrBit(E_GPB, 6)
#define  LED4_OFF  DrvGPIO_ClrBit(E_GPB, 7)

#define N 4 // Number of lanes in the traffic square
#define Level_2_length 20 + N // The maximum number of cars in the traffic square
#define Level_3_length 100 // The maximum number of cars in the exit lanes

// index
uint8_t   i = 0;
uint8_t   j = 0;
uint8_t   k = 0;
uint16_t yy = 0;
uint8_t  nv = 0; // numbers vector
uint16_t  a = 0;
uint8_t   b = 0;
uint8_t  il = 0;
uint8_t  ol = 0;

// tables sent from PC
uint8_t Level_1[N];
uint8_t Level_2[Level_2_length][2];
uint8_t Level_3[Level_3_length][2];
bool ac_l1 = false; // accepted table level 1
bool ac_l2 = false; // accepted table level 2
bool ac_l3 = false; // accepted table level 3

// tables built
uint8_t UT[N];			 // usage table
uint8_t PT[N][N][2]; // path table

// timer
bool en_tr = false; // enable timer
float sec = 0;
float dly_sec = 0.0;

// uart + chack
uint8_t cnt_car;
volatile uint8_t comRbuf[300];
volatile uint16_t comRbytes = 0;

// data sent from PC
uint8_t lanes_map[N][2]; // lanes_map[N][0] = time wait  ;  lanes_map[N][1] = cars enter
uint8_t problem_line;
uint8_t num_enter;
uint8_t num_wait;
bool ac_pl  = false;   // accepted problem line
bool ac_ch  = false;   // accepted check
bool ac_lm  = false;   // accepted lane map
bool ac_sms = false;	 // accepted system matching start
bool ac_smf = false;	 // accepted system matching full

// data sent to PC
float report[3][2];
bool solve = false;

void check(void) // chack data
{	
	if (comRbuf[0] == '@')
	{
		if      (comRbuf[1] == '1') // Level 1 
		{
			for (i = 0; i < N; i++) Level_1[i] = 0; 	// init table level 1
			for (nv = 0; nv < 2 * N; nv += 2)	Level_1[nv / 2] = trans( comRbuf[nv + 2] ) * 10 + trans( comRbuf[nv + 3] );	
			ac_l1 = true; 	// table level 1 accepted
		}
		else if (comRbuf[1] == '2') // Level 2 
		{
			for (i = 0; i < (Level_2_length); i++) for (j = 0; j < 2; j++) Level_2[i][j] = 0;	// init table level 2
			for (nv = 0; nv < cnt_car; nv++)
			{ 				
				Level_2[nv][0] = trans( comRbuf[2 + nv * 2] ); 	  	// status
				Level_2[nv][1] = trans( comRbuf[2 + nv * 2 + 1] );  // number in lane
			}
			ac_l2 = true;  // table level 2 accepted
		}
		else if (comRbuf[1] == '3') // level 3 
		{
			for (i = 0; i < Level_3_length; i++) for (j = 0; j < 2; j++) Level_3[i][j] = 0;	// init table level 3
			for (nv = 0; nv < cnt_car; nv++)
			{
				Level_3[nv][0] = trans( comRbuf[2 + nv * 2] ); 	  	// number in lane
				Level_3[nv][1] = trans( comRbuf[2 + nv * 2 + 1] );  // number out lane
			}
			ac_l3 = true; // table level 3 accepted
		}
		else if (comRbuf[1] == 'C') // check 
		{
			num_enter = trans( comRbuf[2] ) * 10 + trans( comRbuf[3] );
			num_wait  = trans( comRbuf[4] ) * 10 + trans( comRbuf[5] );
			ac_ch = true;	// check accepted
		}
		else if (comRbuf[1] == 'P' && comRbuf[2] == 'L') // problem line 
		{
			problem_line = trans( comRbuf[3] );
			ac_pl = true;	// problem line accepted
		}
		else if (comRbuf[1] == 'L' && comRbuf[2] == 'M') // lane map 
		{
			for (i = 0; i < N; i++) for (j = 0; j < 2; j++) lanes_map[i][j] = 0;	// init table lanes map
			for (i = 0; i < N; i++)
			{ 				
				lanes_map[i][0] = trans( comRbuf[3 + i * 5] ) * 100 + trans( comRbuf[4 + i * 5] ) * 10 + trans( comRbuf[5 + i * 5] );
				lanes_map[i][1] = trans( comRbuf[6 + i * 5] ) * 10  + trans( comRbuf[7 + i * 5] );
			}
			ac_lm = true;	// lanes map accepted
		}
		else if (comRbuf[1] == 'S' && comRbuf[2] == 'M' && comRbuf[3] == 'S') // system matching start 
		{
			ac_sms = true;
		}
		else if (comRbuf[1] == 'S' && comRbuf[2] == 'M' && comRbuf[3] == 'F') // system matching full 
		{
			ac_smf = true;	
		}
	}
}
void Delay(float d_sec, bool I_timer) // States if to turn on the interrupt timer and for how long
{
	dly_sec = d_sec;
	en_tr = true;
	TIMER0->TCSR.CRST = 1; // Reset up counter
	TIMER0->TCSR.CEN = 1;  // enable Timer0
	if (!I_timer)	while(en_tr); 
}
void UART_INT_HANDLE(void) //  UART  interrupt subroutine 
{
	uint8_t bInChar[1] = {0xFF};
	char blu_dis[13] = {"+DISC:SUCCESS"}; // Bluetooth disconnected
	bool en_blu_dis = false; // enable Bluetooth disconnected
	
	while(UART0->ISR.RDA_IF==1) 
	{
		DrvUART_Read(UART_PORT0,bInChar,1);
		if (comRbytes < 300) // check if Buffer is full
		{
			if (bInChar[0] != '$') comRbuf[comRbytes] = bInChar[0];
			comRbytes++;
		}
	}
	for (yy = 0; yy < 13; yy++)   // check if comes message to disconnect: "+DISC:SUCCESS"
		if (comRbuf[yy] == blu_dis[yy]) en_blu_dis = true; 
		else en_blu_dis = false; 											
	if (!en_blu_dis) 
		for (yy = 1; yy < 13; yy++) // check if comes message to disconnect: "DISC:SUCCESS"
			if (comRbuf[yy - 1] == blu_dis[yy]) en_blu_dis = true; 
			else en_blu_dis = false;  
	if (en_blu_dis) 							// if comes message to disconnect clear all
		{en_blu_dis = false; comRbytes = 0; for (yy = 0; yy < 300; yy++) comRbuf[yy] = '0';}  
	if (comRbytes > 299 || bInChar[0] == '$') 
		{cnt_car = (comRbytes - 2) / 2; comRbytes = 0; check(); for (yy = 0; yy < 300; yy++) comRbuf[yy] = '0';}
}
void TMR0_IRQHandler(void) // Timer0 interrupt subroutine
{
	static uint32_t msec = 0;

	msec++;
	if(msec == 100)	// 0.1 Second period
	{
		msec = 0;
		if (dly_sec > 0) dly_sec -= 0.1;
		sec += 0.1;
		if (dly_sec < 0.1)
		{
			TIMER0->TCSR.CEN = 0;	// disable Timer0
			en_tr = false;
			sec = 0;
		}
	}
	TIMER0->TISR.TIF = 1; // Write 1 to clear for safty	
}
void GPIOAB_INT_CallBack(uint32_t GPA_IntStatus, uint32_t GPB_IntStatus) // GPIO interrupt subroutine for port (A and B) 
{
	if ((GPA_IntStatus>>0) & 0x01) DrvUART_Write(UART_PORT0, "$pap00$", 7);
	if ((GPA_IntStatus>>1) & 0x01) DrvUART_Write(UART_PORT0, "$pap01$", 7);
	if ((GPA_IntStatus>>2) & 0x01) DrvUART_Write(UART_PORT0, "$pap02$", 7); 
	if ((GPA_IntStatus>>3) & 0x01) DrvUART_Write(UART_PORT0, "$pap03$", 7);
	if ((GPA_IntStatus>>4) & 0x01) DrvUART_Write(UART_PORT0, "$pap04$", 7);
	if ((GPA_IntStatus>>5) & 0x01) DrvUART_Write(UART_PORT0, "$pap05$", 7);
	if ((GPA_IntStatus>>6) & 0x01) DrvUART_Write(UART_PORT0, "$pap06$", 7);
	if ((GPA_IntStatus>>9) & 0x01) DrvUART_Write(UART_PORT0, "$pap09$", 7);
}
void GPIOCDE_INT_CallBack(uint32_t GPC_IntStatus, uint32_t GPD_IntStatus, uint32_t GPE_IntStatus) // GPIO interrupt subroutine for port (C and D and E)
{  
	cnt_car = 0;
}
int circle_of_lanes(int li, int lo) // for Path_Table_fun(), States which path is interfering
{
	uint8_t ret = 0;
	
	do
	{
		if (li < N) li++;
		else li = 1;
		if (li != lo) ret |= (int)pow( 2, (li - 1) );
		else break;
	} while (1);
	return ret;
}
void Usage_Table_fun(void) // Builds the usage table
{
	float NIS = 0;	// the Number of cars In the Square
	
	for (a = 0; a < N; a++) UT[a] = 0; // init Usage Table
	for (a = 0; a < Level_2_length; a++) // a = index of the input lane
		if (Level_2[a][0] == 1) { NIS++; UT[ Level_2[a][1] - 1 ]++; }  
	for (a = 0; a < N; a++)  UT[a] = (int)( (UT[a] / NIS) * 100) ;   // a = index of the input lane
}
void Path_Table_fun(void) //Builds the Path table
{
	float NIL3 = 0;  // Number of cars in the Input Lane in the Level_3 table
	float NIOL3 = 0; // Number of cars in the Input(a) end Output(b) Lanes in the Level_3 table
	
	for (a = 0; a < N; a++) for (b = 0; b < N; b++) { PT[a][b][0] = 0; PT[a][b][1] = 0; } // init Path Table
	for (a = 0; a < N; a++) // a = index of the input lane
	{
		if (Level_1[a] != 0) // enter lane that does not have vehicles does not need to build the path table
		{
			NIL3 = 0;
			for (k = 0; k < Level_3_length; k++) // k = index of the level_3 
				if ( Level_3[k][0] == (a + 1) ) NIL3++; 
			if (NIL3 != 0)
			{
				for (b = 0; b < N; b++) // b = index of the output lane
				{
					NIOL3 = 0;
					for (k = 0; k < Level_3_length; k++) // k = index of the level_3
						if ( ( Level_3[k][0] == (a + 1) ) && ( Level_3[k][1] == (b + 1) ) ) NIOL3++; 
					PT[a][b][0] = (NIOL3 / NIL3) * 100;
					if(PT[a][b][0] != 0) PT[a][b][1] = circle_of_lanes(a + 1, b + 1);
				}
			}
		}
	}
}
void send_ut(void) // send use  table to PC
{
	char CUT[4 * N] = "";
	
	DrvUART_Write(UART_PORT0, "$", 1);
	DrvUART_Write(UART_PORT0, "ut ", 3);
	for (a = 0; a < N; a++) // a = index of the input lane
	{
			CUT[0 + 4 * a] = itrans(UT[a] / 100);
			CUT[1 + 4 * a] = itrans( (UT[a] / 10) % 10 );
			CUT[2 + 4 * a] = itrans(UT[a] % 10);
			CUT[3 + 4 * a] = ' ';
	}
	DrvUART_Write(UART_PORT0 , CUT, 4 * N);
	DrvUART_Write(UART_PORT0, "$", 1);
}
void send_pt(void) // send path table to PC  
{
	char CPT[7 * N * N + 2 * N] = "";
	
	DrvUART_Write(UART_PORT0, "$", 1);
	DrvUART_Write(UART_PORT0, "pt ", 3);
	for (a = 0; a < N; a++)	// a = index of the input lane
	{
		for (b = 0; b < N; b++) //b = index of the output lane 
		{
			CPT[0 + 30 * a + 7 * b] = itrans(PT[a][b][0] / 100);
			CPT[1 + 30 * a + 7 * b] = itrans( (PT[a][b][0] / 10) % 10 );
			CPT[2 + 30 * a + 7 * b] = itrans(PT[a][b][0] % 10);
			CPT[3 + 30 * a + 7 * b] = ' ';
			CPT[4 + 30 * a + 7 * b] = itrans(PT[a][b][1]);
			CPT[5 + 30 * a + 7 * b] = ' ';
			CPT[6 + 30 * a + 7 * b] = ' ';
		}
		CPT[30 * a + 28] = '/';
		CPT[30 * a + 29] = '/';
	}
	DrvUART_Write(UART_PORT0, CPT, 7 * N * N + 2 * N);
	DrvUART_Write(UART_PORT0, "$", 1);
}
void send_report(void) // send report_table + solve to PC  
{
	char CRT[30] = "";
	
	DrvUART_Write(UART_PORT0, "$", 1);
	DrvUART_Write(UART_PORT0, "RT ", 3);
	for (a = 0; a < 3; a++)	// a = index of the input lane
	{
		CRT[0 + 10 * a] = itrans( (int)(report[a][0]) / 100);
		CRT[1 + 10 * a] = itrans( ( (int)(report[a][0]) / 10 ) % 10 );
		CRT[2 + 10 * a] = itrans( (int)(report[a][0]) % 10);
		CRT[3 + 10 * a] = '.';
		CRT[4 + 10 * a] = itrans( (int)(report[a][0] * 10) % 10 );
		CRT[5 + 10 * a] = ' ';
		CRT[6 + 10 * a] = itrans( (int)(report[a][1]) / 10 );
		CRT[7 + 10 * a] = itrans( (int)(report[a][1]) % 10 );
		CRT[8 + 10 * a] = ' ';
		CRT[9 + 10 * a] = '/';
	}
	DrvUART_Write(UART_PORT0, CRT, 30);
	DrvUART_Write(UART_PORT0, "$", 1);
	Delay(0.1, false);
	DrvUART_Write(UART_PORT0, "$SOLVE = " , 9);
	if (solve) DrvUART_Write(UART_PORT0, "TRUE$" , 5);
	else       DrvUART_Write(UART_PORT0, "FALSE$", 6);
}
void fix_problem (void) //Problem solving algorithm
{
	char cpl[16] = "";  			  // char problem line
	uint8_t il_test = 0;
	uint8_t cnt_il = 0;
	uint8_t lane_error = 0;
	uint8_t lane_error_next = 0;
	uint8_t percent_lane_error = 0;
	uint8_t max_percent_u = 101;
	uint8_t max_percent_u_next = 0;
	uint8_t problem_line_bin =	0;
	
	for (il = 0; il < 3; il++) for (ol = 0; ol < 2; ol++) report[il][ol] = 0.0;
	solve = false;
	
	clr_all_panel(); LEDLCD_ON;
	cpl[1] = itrans(problem_line);
	cpl[0] = '0';
	print_lcd(0, "problem:");
	print_lcd(1, cpl);
	problem_line_bin = (int)pow(2, (problem_line - 1));
	do 
	{
		cnt_il += 1;
		if (cnt_il > N) break;
		max_percent_u_next = 0;
		percent_lane_error = 0;
		lane_error = 0;
		for (il = 0; il < N; ++il)	
			if ( ( max_percent_u_next <  UT[il] ) && ( UT[il] < max_percent_u ) ) 
				{max_percent_u_next = UT[il]; il_test = il;}
		for (ol =0; ol < N; ++ol)		
			if ( ( (PT[il_test][ol][1] & problem_line_bin) != 0 ) && (PT[il_test][ol][0] > percent_lane_error) ) 
				{lane_error = il_test + 1; percent_lane_error = PT[il_test][ol][0];}
		if (lane_error == 0) max_percent_u = max_percent_u_next;
	}while(lane_error == 0);
	if (lane_error != 0)
	{
		cpl[1] = itrans(lane_error);
		cpl[0] = '0'; 
		print_lcd(2, "lane error:");
		print_lcd(3, cpl);
		switch(lane_error)
		{
			case 1: LED1_ON; break;
			case 2: LED2_ON; break;
			case 3: LED3_ON; break;
			case 4: LED4_ON; break;
		}
		Delay((float)lanes_map[problem_line - 1][0], true);
		while( sec < (float)(lanes_map[problem_line - 1][0] / 2) ); 
		DrvUART_Write(UART_PORT0, "$CHECK$", 7); while(!ac_ch); ac_ch = false;

		if (num_enter != 0 || num_wait == 0)
		{
			if ( (num_wait == 0) || (num_enter >= lanes_map[problem_line - 1][1]) )
			{
				report[0][0] = (float)(lanes_map[problem_line - 1][0]) / 2;	
				report[0][1] = lane_error; 
				solve = true;
			}
			else 
			{
				while(en_tr); 
				report[0][0] = (float)lanes_map[problem_line - 1][0]; 
				report[0][1] = lane_error;
				DrvUART_Write(UART_PORT0, "$CHECK$", 7); while(!ac_ch); ac_ch = false;
				if (num_enter != 0) solve = true;  
				else solve = false;
			}
		}
		else
		{
			cnt_il = 0;
			do 
			{
				cnt_il += 1;
				if (cnt_il > N) break;
				max_percent_u_next = 0;
				percent_lane_error = 0;
				lane_error_next = 0;
				for (il = 0; il < N; ++il)
					if( (max_percent_u_next <= UT[il]) && (UT[il] < max_percent_u) ) 
						{max_percent_u_next = UT[il]; il_test = il;}
				for (ol =0; ol < N; ++ol)
					if ( ( (PT[il_test][ol][1] & problem_line_bin) != 0 ) && (PT[il_test][ol][0] > percent_lane_error) && ( (il_test + 1) != lane_error) ) 
						{lane_error_next = il_test + 1; percent_lane_error = PT[il_test][ol][0];}
				if (lane_error_next == 0) max_percent_u = max_percent_u_next;
			}while(lane_error_next == 0);
			if (lane_error_next == 0)	
			{
				while(en_tr); 
				report[0][0] = (float)lanes_map[problem_line - 1][0]; 
				report[0][1] = lane_error;
				DrvUART_Write(UART_PORT0, "$CHECK$", 7); while(!ac_ch); ac_ch = false;
				if (num_enter != 0) solve = true;
				else solve = false;
			}
			else
			{
				cpl[1] = itrans(lane_error_next);
				cpl[0] = '0'; 
				print_lcd(2, "lane error new:");
				print_lcd(3, cpl);
				LED1_OFF;	LED2_OFF;	LED3_OFF;	LED4_OFF;
				switch(lane_error_next)
				{
					case 1: LED1_ON; break;
					case 2: LED2_ON; break;
					case 3: LED3_ON; break;
					case 4: LED4_ON; break;
				}
				while(en_tr);
				DrvUART_Write(UART_PORT0, "$CHECK$", 7); while(!ac_ch); ac_ch = false;
				if (num_enter == 0)
				{
					cpl[1] = itrans(lane_error);
					cpl[0] = itrans(lane_error_next);
					print_lcd(2, "lane error new:");
					print_lcd(3, cpl);
					switch(lane_error)
					{
						case 1: LED1_ON; break;
						case 2: LED2_ON; break;
						case 3: LED3_ON; break;
						case 4: LED4_ON; break;
					}
					Delay( (float)(lanes_map[problem_line - 1][0] / 2), false );
					report[0][0] = (float)(lanes_map[problem_line - 1][0]) / 2; report[0][1] = lane_error;
					report[1][0] = (float)(lanes_map[problem_line - 1][0]) / 2; report[1][1] = lane_error_next;
					report[2][0] = (float)(lanes_map[problem_line - 1][0]) / 2; report[2][1] = lane_error * 10 + lane_error_next;
					DrvUART_Write(UART_PORT0, "$CHECK$", 7); while(!ac_ch); ac_ch = false;
					if (num_enter != 0) solve = true;
					else solve = false;
				}
				else
				{
					report[0][0] = (float)(lanes_map[problem_line - 1][0]) / 2; report[0][1] = lane_error;
					report[1][0] = (float)(lanes_map[problem_line - 1][0]) / 2; report[1][1] = lane_error_next;
					solve = true;
				}
			}
		}
	}
	dly_sec = 0.0;
	TIMER0->TCSR.CEN = 0;	
	clr_all_panel(); LEDLCD_OFF; 
	LED1_OFF;	LED2_OFF;	LED3_OFF;	LED4_OFF;
	send_report();
}
int32_t main (void)
{
	UNLOCKREG();
	SYSCLK->PWRCON.XTL12M_EN = 1;
	DrvSYS_Delay(5000);					// Waiting for 12M Xtal stalble
	SYSCLK->CLKSEL0.HCLK_S = 0;
	LOCKREG();

	Initial_panel();  //initialize LCD
	clr_all_panel();  //clear LCD
	InitPIN();
	video();
 	InitTIMER0();	
 	InitUART();
	InitINT_GPIO();
	
	for (a = 0; a < 300; a++) comRbuf[a] = '0';
	
	//Enable the interrupt function
	DrvGPIO_SetIntCallback(GPIOAB_INT_CallBack, GPIOCDE_INT_CallBack);
	DrvUART_EnableInt(UART_PORT0, DRVUART_RDAINT, UART_INT_HANDLE);
	
	LEDLCD_ON; 
	while (!ac_lm)
	{
		print_lcd(1, "    Waiting     "); print_lcd(2, "    lane map    "); 
		if (ac_sms) {ac_sms = false; clr_all_panel(); print_lcd(1, "system matching "); while (!ac_smf); ac_smf = false;}
	}
	ac_lm = false; clr_all_panel(); LEDLCD_OFF;

	while(1)
	{
		if ( (ac_l1) && (ac_l2) && (ac_l3) && (ac_pl) )
		{
			ac_l1 = false;	ac_l2 = false;	ac_l3 = false;	ac_pl = false;
			Usage_Table_fun();	Path_Table_fun();
			send_ut(); send_pt(); 
			if (problem_line != 0) fix_problem ();
		}
		if (ac_sms) {ac_sms = false; clr_all_panel(); LEDLCD_ON; print_lcd(1, "system matching "); while (!ac_smf); ac_smf = false; clr_all_panel(); LEDLCD_OFF;} 
	}
}
////
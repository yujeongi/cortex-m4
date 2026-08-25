#include "device_driver.h"
#include <stdio.h>
#include <string.h>

#define PASSWORD "1234"


# if 0
static void Sys_Init(int baud) 
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
	Keypad_Init();
}
void Main(void)
{
	char key;

	Sys_Init(115200);
	printf("\n4x4 Keypad Test\n");
	printf("Press a key (1-9, 0, A-D, *, #)\n");

	for(;;)
	{
		key = Keypad_Get_Key();

		if(key != 0)
		{
			printf("KEY: %c\n", key);
		}
	}
}

# endif

# if 0
static void Sys_Init(int baud) 
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
	Keypad_Init();
}
void Main(void)
{
	Sys_Init(115200);
	LCD_Init();
	LCD_Print("Hello!");
}
# endif

# if 0

static void Sys_Init(int baud)
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2);
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
	Keypad_Init();
	LCD_Init();
}

// 입력 화면 초기 상태로 세팅 (1행: Enter Password, 2행: 비워둠 + 커서를 2행 0열로)
static void Show_Enter_Screen(void)
{
	LCD_Clear();
	LCD_Set_Cursor(0, 0);
	LCD_Print("Enter Password");
	LCD_Set_Cursor(1, 0);
}

void Main(void)
{
	char key;
	char buffer[5] = {0,};   // 4자리 + null
	int idx = 0;

	Sys_Init(115200);

	printf("\nDoor Lock - Password Test\n");

	Show_Enter_Screen();

	for(;;)
	{
		key = Keypad_Get_Key();

		if(key == 0)
		{
			continue;
		}

		printf("KEY: %c\n", key);

		if(key == '#')
		{
			if(idx == 4)
			{
				buffer[4] = '\0';

				if(strcmp(buffer, PASSWORD) == 0)
				{
					LCD_Clear();
					LCD_Set_Cursor(0, 0);
					LCD_Print("Door Open");
				}
				else
				{
					LCD_Clear();
					LCD_Set_Cursor(0, 0);
					LCD_Print("Wrong Password");
				}
			}
			else
			{
				// 4자리 다 안 채우고 # 누른 경우
				LCD_Clear();
				LCD_Set_Cursor(0, 0);
				LCD_Print("Wrong Password");
			}

			TIM2_Delay(2000);   // 결과 화면 잠깐 보여주고 다시 입력 화면으로
			idx = 0;
			memset(buffer, 0, sizeof(buffer));
			Show_Enter_Screen();
		}
		else if(key >= '0' && key <= '9')
		{
			if(idx < 4)
			{
				buffer[idx++] = key;
				LCD_Data(key);   // 2행에 입력한 숫자 그대로 표시 (마스킹 없이)
			}
		}
		// A, B, C, D, * 등 다른 키는 지금 단계에서는 무시
	}
}
# endif

# if 0
static void Sys_Init(int baud)
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2);
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
	Keypad_Init();
	LCD_Init();
}

void Main(void)
{
	Sys_Init(115200);
	Servo_Init();

	for(;;)
	{
		Servo_Unlock();
		TIM2_Delay(1000);
		Servo_Lock();
		TIM2_Delay(1000);
	}
}
# endif

# if 0
static void Sys_Init(int baud)
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2);
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
	Keypad_Init();
	LCD_Init();
}

void Main(void)
{
	Sys_Init(115200);
	Buzzer_Init();

	for(;;)
	{
		TIM3_Out_Freq_Generation(440);   // 그냥 440Hz(라) 계속 재생
		TIM2_Delay(1000);
		TIM3_Out_Stop();
		TIM2_Delay(1000);
	}
}
# endif

# if 1
static void Sys_Init(int baud)
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2);
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
	Keypad_Init();
	LCD_Init();
	Servo_Init();
	Buzzer_Init();
}

// 입력 대기 화면 (1행: Enter Password, 커서는 2행 0열로)
static void Show_Enter_Screen(void)
{
	LCD_Clear();
	LCD_Set_Cursor(0, 0);
	LCD_Print("Enter Password");
	LCD_Set_Cursor(1, 0);
}

void Main(void)
{
	char key;
	char buffer[5] = {0,};   // 4자리 + null
	int idx = 0;

	Sys_Init(115200);

	printf("\nDoor Lock System Start\n");

	Servo_Lock();          // 시작 시 잠금 상태로 초기화
	Show_Enter_Screen();

	for(;;)
	{
		key = Keypad_Get_Key();

		if(key == 0)
		{
			continue;
		}

		printf("KEY: %c\n", key);

		if(key == '#')
		{
			if(idx == 4 && strcmp(buffer, PASSWORD) == 0)
			{
				// ===== 정답 처리 =====
				LCD_Clear();
				LCD_Set_Cursor(0, 0);
				LCD_Print("Door Open");

				Buzzer_Correct();      // 도-미-솔
				Servo_Unlock();        // 서보 90도 (열림)

				TIM2_Delay(5000);      // 5초 대기

				Buzzer_Close();        // 솔-미-도 (닫히는 소리)
				Servo_Lock();          // 서보 원위치 (잠김)

				LCD_Clear();
				LCD_Set_Cursor(0, 0);
				LCD_Print("Door Closed");

				TIM2_Delay(1500);      // 잠깐 보여주고
			}
			else
			{
				// ===== 오답 처리 =====
				LCD_Clear();
				LCD_Set_Cursor(0, 0);
				LCD_Print("Wrong Password");

				Buzzer_Wrong();        // 낮은음 두 번

				TIM2_Delay(1500);
			}

			idx = 0;
			memset(buffer, 0, sizeof(buffer));
			Show_Enter_Screen();
		}
		else if(key >= '0' && key <= '9')
		{
			if(idx < 4)
			{
				buffer[idx++] = key;
				LCD_Data(key);   // 2행에 입력한 숫자 표시
			}
		}
		// A, B, C, D, * 등은 현재 시나리오에서 사용 안 함
	}
}
# endif
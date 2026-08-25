#include "device_driver.h"
#include <stdio.h>

/*
 * LCD I2C 백팩 (PCF8574) 핀 매핑 - 대부분의 1602 I2C 모듈 공통
 * P0 = RS
 * P1 = RW
 * P2 = EN
 * P3 = Backlight
 * P4~P7 = D4~D7
 *
 * I2C1_SCL => PB6, I2C1_SDA => PB7
 */

#define LCD_I2C_ADDR_WR   0x4E   // PCF8574 기본 주소 0x27 기준 (0x27<<1). 실제 주소 다르면 수정
#define LCD_I2C_ADDR_RD   0x4F

#define LCD_BACKLIGHT     0x08   // P3 = 1 (백라이트 켜짐)
#define LCD_EN            0x04   // P2
#define LCD_RW            0x02   // P1
#define LCD_RS            0x01   // P0

#define LCD_CMD           0
#define LCD_DATA          1

/* ===================== I2C1 하드웨어 초기화 (PB6=SCL, PB7=SDA) ===================== */

static void I2C1_LCD_Init(unsigned int freq)
{
	unsigned int r;
	volatile int i;

	Macro_Set_Bit(RCC->AHB1ENR, 1);                    // Port-B Clock On
	Macro_Set_Bit(RCC->APB1ENR, 21);                   // I2C1 Clock On

	Macro_Clear_Bit(RCC->APB1RSTR, 21);                // I2C1 Reset
	Macro_Set_Bit(RCC->APB1RSTR, 21);
	for(i = 0; i < 1000; i++);
	Macro_Clear_Bit(RCC->APB1RSTR, 21);

	Macro_Write_Block(GPIOB->MODER, 0xf, 0xa, 12);     // PB[7:6] => ALT
	Macro_Write_Block(GPIOB->AFR[0], 0xff, 0x44, 24);  // PB[7:6] => AF04
	Macro_Write_Block(GPIOB->OTYPER, 0x3, 0x3, 6);     // PB[7:6] => Open Drain
	Macro_Write_Block(GPIOB->OSPEEDR, 0xf, 0xa, 12);   // PB[7:6] => Fast Speed
	Macro_Write_Block(GPIOB->PUPDR, 0xf, 0x5, 12);     // PB[7:6] => Internal Pull-up

// I2C1->CR2[5:0] = FREQ 필드: I2C 클럭이 기준으로 삼는 APB1 클럭 속도를 MHz 단위로 알려줌
// (예: PCLK1=48MHz면 48을 넣음 → I2C 컨트롤러가 내부 타이밍 계산할 때 이 값을 기준으로 씀)
Macro_Write_Block(I2C1->CR2, 0x3f, PCLK1 / 1000000, 0);

// I2C1->CR1의 PE(Peripheral Enable) 비트를 0으로 클리어
// → CCR, TRISE 같은 타이밍 레지스터들은 I2C가 disable 상태일 때만 수정 가능하기 때문에
//    설정 전에 반드시 꺼야 함
Macro_Clear_Bit(I2C1->CR1, 0);

// TRISE = SCL 라인이 LOW에서 HIGH로 올라갈 때 걸리는 최대 rise time을 결정하는 레지스터
// 공식: TRISE = (APB1 클럭(MHz) × 최대 허용 rise time(usec)) + 1
// Standard mode(100kHz) 기준 최대 rise time은 1000ns(1usec)라서 그냥 (PCLK1_MHz + 1)로 계산
I2C1->TRISE = (PCLK1 / 1000000) + 1;

// CCR(Clock Control Register) 계산 준비
// r = PCLK1을 (목표 I2C 주파수 × 2)로 나눈 값
// → I2C SCL 클럭 한 주기는 "HIGH 구간 + LOW 구간"으로 나뉘는데,
//   Standard mode에서는 이 둘을 절반씩(50:50)으로 만들기 때문에 2로 나눔
r = PCLK1 / (freq * 2);

// CCR 값이 너무 작으면(4 미만) 최소 허용값인 4로 강제 고정
// (레퍼런스 매뉴얼에서 정한 CCR 최소값 제한 - 너무 작으면 타이밍이 규격을 못 지킴)
I2C1->CCR = ((r < 4) ? 4 : r);

// I2C1->CR1의 SMBUS 비트(1번)를 0으로 클리어
// → SMBus 모드가 아니라 일반 I2C 모드로 동작하게 설정
Macro_Clear_Bit(I2C1->CR1, 1);

// I2C1->CR1의 PE(Peripheral Enable) 비트를 다시 1로 세팅
// → 지금까지 설정한 CCR, TRISE 값을 반영하면서 I2C1 페리퍼럴을 실제로 켬
Macro_Set_Bit(I2C1->CR1, 0);

// I2C1->CR1의 ACK(Acknowledge Enable) 비트를 1로 세팅
// → 데이터를 받을 때(수신 모드) 자동으로 ACK 신호를 보내도록 설정
// (지금 LCD는 쓰기만 하니 당장 필수는 아니지만, 범용 초기화 루틴이라 항상 켜둠)
Macro_Set_Bit(I2C1->CR1, 10);
}

/* ===================== PCF8574로 1바이트 전송 ===================== */

// 이 함수는 "PCF8574 칩한테 8비트 값 하나를 I2C로 보낸다"는 가장 밑바닥 함수예요.
// LCD고 뭐고 상관없이, 그냥 "이 8비트를 그대로 PCF8574의 P0~P7 핀에 실어라"라는 명령임.
static void PCF8574_Write(unsigned char data)
{
	// I2C 버스가 지금 다른 통신 중이 아니고 비어있는(Idle) 상태인지 확인
	// (바쁘면 계속 기다림 - 통신 겹치는 걸 방지)
	while(Macro_Check_Bit_Set(I2C1->SR2, 1));

	// I2C 통신 시작 신호(Start Condition)를 보내라고 요청
	Macro_Set_Bit(I2C1->CR1, 8);
	// 실제로 Start 신호가 하드웨어적으로 나갔는지 확인될 때까지 대기
	while(Macro_Check_Bit_Clear(I2C1->SR1, 0));

	// "나 지금부터 이 주소(LCD_I2C_ADDR_WR)를 가진 슬레이브랑 쓰기(Write) 통신할거야"
	// 라고 버스에 슬레이브 주소를 실어 보냄
	I2C1->DR = LCD_I2C_ADDR_WR;
	// 슬레이브(PCF8574)가 "어 나 그 주소 맞아, 응답할게(ACK)"라고 답할 때까지 대기
	while(Macro_Check_Bit_Clear(I2C1->SR1, 1));
	// ADDR 플래그는 SR1을 읽고 SR2를 읽어야 자동으로 클리어됨 (레지스터 규칙)
	// 그래서 SR2를 그냥 한번 읽어서 버림(안 쓸 거지만 읽어야 플래그가 지워짐)
	(void)I2C1->SR2;

	// 송신 레지스터(DR)가 비어서 새 데이터를 받을 준비가 됐는지 확인
	while(Macro_Check_Bit_Clear(I2C1->SR1, 7));
	// 진짜 보내고 싶은 8비트 데이터를 DR에 실어서 전송
	I2C1->DR = data;
	// 이 바이트가 슬레이브한테 완전히 다 전달됐는지(Byte Transfer Finished) 확인
	while(Macro_Check_Bit_Clear(I2C1->SR1, 2));

	// 통신 끝났다는 Stop 신호를 보냄
	Macro_Set_Bit(I2C1->CR1, 9);
	// Stop 신호는 하드웨어가 보내고 나면 이 비트를 자동으로 0으로 꺼주는데,
	// 그게 될 때까지(=Stop이 실제로 완료될 때까지) 대기
	while(Macro_Check_Bit_Set(I2C1->CR1, 9));
}

/* ===================== HD44780 4비트 니블 전송 ===================== */

// HD44780(진짜 LCD 컨트롤러)은 한 번에 8비트를 다 못 받고,
// 4비트씩("니블" = 4비트 단위) 나눠서 두 번 받는 방식(4비트 모드)으로 통신해요.
// 이 함수는 "니블 하나를 실제로 LCD가 인식하도록 전달"하는 역할이에요.
static void LCD_Write_Nibble(unsigned char nibble, unsigned char mode)
{
	// PCF8574의 8비트 중:
	// - 상위 4비트(nibble & 0xF0)는 실제 데이터 4비트(D4~D7 자리)
	// - LCD_BACKLIGHT(0x08)는 항상 켜서 백라이트 유지 (P3)
	// - mode가 1(데이터)이면 LCD_RS(0x01)를 켜고, 0(명령어)이면 RS는 끔 (P0)
	unsigned char data = (nibble & 0xF0) | LCD_BACKLIGHT | (mode ? LCD_RS : 0);

	// HD44780은 "EN(Enable) 핀이 HIGH에서 LOW로 떨어지는 그 순간"에
	// 그때 D4~D7, RS 값을 읽어들여서 실제로 처리해요.
	// 그래서 순서가 중요함:
	// 1) 먼저 EN=HIGH 상태로 데이터 실어서 보냄 (아직 안 읽음, 준비 단계)
	PCF8574_Write(data | LCD_EN);
	TIM2_Delay(1);   // EN이 HIGH로 유지되는 최소 시간 확보 (너무 빨리 바꾸면 LCD가 못 읽음)

	// 2) EN을 LOW로 내림 -> 이 순간 LCD가 "어 지금 데이터 왔구나" 하고 읽어들임
	PCF8574_Write(data & ~LCD_EN);
	TIM2_Delay(1);   // 다음 명령 넘어가기 전 안정화 시간
}

// 진짜 보내고 싶은 8비트(명령어든 문자데이터든)를
// 니블 두 개로 쪼개서 순서대로 보내는 함수
static void LCD_Send(unsigned char value, unsigned char mode)
{
	// 예: value = 0b10110010 이라면
	LCD_Write_Nibble(value & 0xF0, mode);          // 상위 니블(1011) 먼저 전송
	LCD_Write_Nibble((value << 4) & 0xF0, mode);   // 값을 4비트 왼쪽으로 밀어서 하위 니블(0010)을 상위 자리로 옮긴 뒤 전송
}

// 바깥에서 실제로 부를 함수 두 개 - "명령어 보내기"와 "문자 데이터 보내기"
// 내부적으로는 완전히 같은 LCD_Send를 쓰고, mode(0/1)만 다름
void LCD_Command(unsigned char cmd)
{
	LCD_Send(cmd, LCD_CMD);   // LCD_CMD = 0 (RS=0, "이건 명령어야")
}

void LCD_Data(unsigned char data)
{
	LCD_Send(data, LCD_DATA);  // LCD_DATA = 1 (RS=1, "이건 화면에 찍을 문자야")
}

/* ===================== 초기화 ===================== */

void LCD_Init()
{
	I2C1_LCD_Init(100000);   // I2C1 하드웨어 자체를 100kHz로 켜는 것 (이전에 설명한 그 부분)

	TIM2_Delay(50);   // 전원 인가 직후 LCD 내부 회로가 안정화될 시간을 기다려줌 (데이터시트 권장)

	// ===== HD44780을 "4비트 모드"로 전환하는 특수 시퀀스 =====
	// HD44780은 전원이 켜지면 기본적으로 "8비트 모드"라고 가정하고 있어서,
	// 4비트 모드로 쓰려면 정해진 순서로 0x30을 세 번 반복해서 보내야 함
	// (이건 HD44780 데이터시트에 정해진 "초기화 프로토콜"이라 이유를 따지기보다 정해진 절차를 그대로 따라야 함)
	LCD_Write_Nibble(0x30, LCD_CMD);
	TIM2_Delay(5);
	LCD_Write_Nibble(0x30, LCD_CMD);
	TIM2_Delay(1);
	LCD_Write_Nibble(0x30, LCD_CMD);
	TIM2_Delay(1);

	// 이제 진짜로 "4비트 모드 쓸게"라고 0x20을 보내서 모드 전환 완료
	LCD_Write_Nibble(0x20, LCD_CMD);
	TIM2_Delay(1);

	// ===== 여기서부터는 이제 "정상적인 4비트 모드"로 통신 가능하니 =====
	// ===== LCD_Command()로 일반적인 설정 명령어들을 보냄 =====

	LCD_Command(0x28);   // Function Set: "4비트 모드, 2줄(16x2), 5x8 폰트 쓸게" 라고 설정
	LCD_Command(0x0C);   // Display Control: "화면 켜기, 커서는 안 보이게, 깜빡임도 끄기"
	LCD_Command(0x06);   // Entry Mode Set: "문자 찍고 나면 커서 자동으로 오른쪽으로 이동"
	LCD_Command(0x01);   // Clear Display: "화면 전체 지우기"
	TIM2_Delay(2);        // Clear 명령은 다른 명령보다 처리 시간이 더 오래 걸려서 (약 1.6ms) 넉넉히 대기
}

void LCD_Clear(void)
{
	LCD_Command(0x01);   // 위에서 쓴 것과 같은 Clear 명령
	TIM2_Delay(2);        // 마찬가지로 넉넉히 대기
}

void LCD_Set_Cursor(unsigned char row, unsigned char col)
{
	// HD44780 내부에는 DDRAM이라는 "문자가 저장되는 메모리 공간"이 있는데,
	// 16x2 LCD 기준으로 1행은 주소 0x00부터, 2행은 주소 0x40부터 시작하도록 정해져 있어요
	// (이것도 HD44780 데이터시트에 정해진 하드웨어 규격)
	static const unsigned char row_offset[2] = {0x00, 0x40};

	// 0x80은 "커서 위치를 설정하겠다"는 명령어의 기본 값이고,
	// 거기에 (행 시작주소 + 원하는 열 번호)를 더해서 최종 위치를 지정
	LCD_Command(0x80 | (row_offset[row] + col));
}

void LCD_Print(char *str)
{
	// 문자열 끝(널문자)까지 한 글자씩 반복해서 LCD_Data로 찍음
	// LCD_Data 한 번 호출할 때마다 Entry Mode Set(0x06)에서 설정한대로
	// 커서가 자동으로 오른쪽으로 한 칸씩 이동하기 때문에, 별도 커서 이동 없이 문자열이 이어서 찍힘
	while(*str != 0)
	{
		LCD_Data(*str++);
	}
}
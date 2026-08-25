#include "device_driver.h"

/*
 * 서보모터 PWM 출력 - TIM4 Channel3 (PB8)
 * 주기: 20ms (50Hz) - 서보모터 표준 규격
 * 펄스폭: 500us(0도) ~ 2400us(180도)
 *
 * 주의: TIM2는 딜레이(TIM2_Delay)용, TIM3는 부저(TIM3_Out_*)용으로
 *       이미 쓰고 있어서 서보는 TIM4로 분리함.
 *       TIM4_Repeat 같은 다른 TIM4 기능을 이후에 같이 쓸 계획이면 충돌 주의.
 */

#define SERVO_PERIOD_US   20000   // 20ms
#define SERVO_MIN_PULSE   500     // 0도
#define SERVO_MAX_PULSE   2400    // 180도

void Servo_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 1);   // GPIOB Clock Enable (이미 켜져 있어도 무해)
	Macro_Set_Bit(RCC->APB1ENR, 2);   // TIM4 Clock Enable

	Macro_Write_Block(GPIOB->MODER, 0x3, 0x2, 16);   // PB8 => ALT
	Macro_Write_Block(GPIOB->AFR[1], 0xf, 0x2, 0);   // PB8 => AF02 (TIM4_CH3)

	// 1us 단위로 카운트하도록 PSC 설정
	TIM4->PSC = (unsigned int)(TIMXCLK / 1000000.0 + 0.5) - 1;
	TIM4->ARR = SERVO_PERIOD_US - 1;   // 20ms 주기

	Macro_Write_Block(TIM4->CCMR2, 0xff, 0x60, 0);   // CH3: PWM Mode 1
	TIM4->CCER = (0 << 9) | (1 << 8);                // CH3 Enable, 정극성

	TIM4->CCR3 = (SERVO_MIN_PULSE + SERVO_MAX_PULSE) / 2;   // 초기값: 중립(약 90도)

	Macro_Set_Bit(TIM4->EGR, 0);
	TIM4->CR1 = (1 << 4) | (0 << 3) | (0 << 1) | (1 << 0);   // ARPE On, Counter Enable
}

// angle: 0 ~ 180
void Servo_Set_Angle(int angle)
{
	unsigned int pulse;

	if (angle < 0)   angle = 0;
	if (angle > 180) angle = 180;

	pulse = SERVO_MIN_PULSE + (unsigned int)((SERVO_MAX_PULSE - SERVO_MIN_PULSE) * (angle / 180.0));
	TIM4->CCR3 = pulse;
}

// 도어락 용도로 자주 쓰는 두 상태만 함수로 분리 (원하는 각도로 값 조정)
void Servo_Lock(void)
{
	Servo_Set_Angle(0);
}

void Servo_Unlock(void)
{
	Servo_Set_Angle(90);
}

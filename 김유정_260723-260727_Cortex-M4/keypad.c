#include "device_driver.h"

/*
 * Keypad Row (출력)
 * R1 = PC0
 * R2 = PC1
 * R3 = PC2
 * R4 = PC3
 *
 * Keypad Column (Pull-up)
 * C1 = PC4
 * C2 = PC5
 * C3 = PC6
 * C4 = PC7
 */

#define ROW_ALL 0x0f

static const char key_map[4][4] = {

	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}

};

// 키패드 초기화
void Keypad_Init() {
	unsigned int pin;
	
	// GPIOC CLK Enable
	RCC->AHB1ENR |= (1 << 2);

	// ROW
	for (pin = 0; pin < 4; pin++) {

		// GPIOC 출력 모드 (01)
		GPIOC->MODER &= ~(0x3 << (pin * 2));
		GPIOC->MODER |= (0x1 << (pin * 2));

		// Push-Pull 출력 (0)
		GPIOC->OTYPER &= ~(0x1 << pin);
	}

	// COL
	for (pin = 4; pin < 8; pin++) {

		// GPIOC 입력 모드 (00)
		GPIOC->MODER &= ~(3 << (pin * 2));

		// 기존 pull-up, pull-down 설정 제거 (00: pull-up/down 없음)
		GPIOC->PUPDR &= ~(0x3 << (pin * 2));

		// Pull-up 설정 (01) pull-up일 때 HIGH
		GPIOC->PUPDR |= (1 << (pin * 2));
	}

	// 모든 ROW를 HIGH로 초기화
	GPIOC->ODR |= ROW_ALL;

}

// 현재 눌린 키 검사
static char Keypad_Scan() {

	unsigned int row;
	unsigned int col;
	unsigned int col_pin;
	volatile unsigned int wait;

	for (row = 0; row < 4; row++) {
		GPIOC->ODR |= ROW_ALL; // 모든 ROW HIGH
		GPIOC->ODR &= ~(0x1 << row); // 검사할 ROW만 LOW

		for (wait = 0; wait < 100; wait++); // key 정상 입력됐는지 확인

		for (col = 0; col < 4; col++) {
			col_pin = col + 4;

			if ((GPIOC->IDR & (0x1 << col_pin)) == 0) {

				// 모든 ROW를 HIGH로 복구
				GPIOC->ODR |= ROW_ALL;

				return key_map[row][col];
			}
		}
	}

	// 모든 ROW를 HIGH로 복구
	GPIOC->ODR |= ROW_ALL;

	return 0;
}

// 키 입력 한 번으로 처리
char Keypad_Get_Key() {

	char key;

	key = Keypad_Scan();

	if (key == 0) {
		return 0;
	}

	TIM2_Delay(20);

	// 20ms 후에도 같은 키가 눌려있으면 정상 입력으로 판단
	if (Keypad_Scan() != key) {
		return 0;
	}

	while (Keypad_Scan() != 0); // 버튼이 떼질 때까지 기다림

	TIM2_Delay(20);

	return key;
}


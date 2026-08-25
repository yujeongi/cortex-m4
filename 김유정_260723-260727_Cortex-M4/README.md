# 미니 프로젝트 최종 보고서
## STM32F411 기반 Key Matrix & Servo Motor Door Lock System

---

# 1. 프로젝트 개요 (Overview)

## 1.1 프로젝트 배경 및 목적

### 프로젝트 배경
현대 사회에서 스마트 홈 및 보안 시스템의 중요성이 증가함에 따라, MCU를 활용한 물리적 보안 시스템의 동작 원리를 이해하기 위해 본 프로젝트를 수행하였다.

### 프로젝트 목적
- STM32F411의 GPIO, I2C, Timer/PWM 주변장치를 레지스터 레벨에서 직접 제어
- 4x4 Key Matrix를 이용한 비밀번호 입력 구현
- I2C LCD를 이용한 상태 표시
- Servo Motor를 이용한 도어 개폐 제어
- Buzzer를 이용한 결과 피드백(성공/실패 멜로디)
- 상태(State)에 따른 임베디드 시스템 동작 구현

---

## 1.2 개발 환경

| 항목 | 내용 |
|------|------|
| MCU | STM32F411RE (Nucleo-64) |
| 개발 방식 | HAL 미사용, CMSIS 레지스터 직접 제어(Bare-metal) |
| Language | C |
| 개발 기간 | 2026.07.23~2026.07.24 |

### 사용 부품

- 4x4 Key Matrix
- SG90 Servo Motor
- I2C LCD (1602, PCF8574 백팩)
- Piezo Buzzer
- 온보드 LED (PA5, 이번 시나리오에서는 미사용)

---

# 2. 시스템 구성 (System Architecture)

## 2.1 시스템 블록도

```text
+---------------------------------------------+
|              STM32F411RE MCU                |
|                                             |
|  +-----------------+    +---------------+   |
|  | Keypad_Get_Key  | <--| 4x4 Key Matrix|   |
|  +-----------------+    +---------------+   |
|           |                                 |
|           v                                 |
|  +-----------------+                        |
|  | Password Check  |                        |
|  |  (strcmp)        |                        |
|  +-----------------+                        |
|           |                                 |
|     +-----+------+------------+             |
|     |            |            |             |
|     v            v            v             |
| Servo Motor   Buzzer       I2C LCD          |
| (TIM4 PWM)   (TIM3 PWM)   (상태 표시)        |
+---------------------------------------------+
```

---

## 2.2 핀맵

| Peripheral | STM32 Pin | 설명 |
|------------|-----------|------|
| UART2 (PC 통신) | PA2(TX), PA3(RX) | 115200bps, 로그 출력용 |
| Key Matrix Row | PC0 ~ PC3 | 출력, Push-pull |
| Key Matrix Column | PC4 ~ PC7 | 입력, 내부 Pull-up |
| I2C LCD (SCL/SDA) | PB6, PB7 | I2C1, 100kHz, PCF8574 백팩 |
| Servo Motor | PB8 (TIM4_CH3) | PWM 출력, 50Hz(20ms 주기) |
| Buzzer | PB0 (TIM3_CH3) | PWM 출력, 음계별 주파수 생성 |
| 온보드 LED | PA5 | 초기화만 되어 있으며 현재 시나리오 미사용 |

---

# 3. 구현 내용 (Implementation)

## 3.1 Key Matrix Driver

행(Row)을 순차적으로 LOW로 변경하면서 열(Column)의 입력을 읽어 눌린 키를 확인하는 방식으로 구현하였다. 20ms 뒤 같은 키가 유지되는지 재확인하는 방식으로 디바운싱을 처리하고, 키가 완전히 떼어질 때까지 대기한 뒤 반환한다.

### 주요 기능

- 4x4 Key Scan (Row 출력 / Column 입력, 내부 Pull-up)
- Debouncing 처리 (20ms 재확인)
- 키 반환은 떼어짐(release)까지 확인 후 수행

### 주요 코드

```c
static char Keypad_Scan()
{
	unsigned int row, col, col_pin;
	volatile unsigned int wait;

	for(row = 0; row < 4; row++)
	{
		GPIOC->ODR |= ROW_ALL;
		GPIOC->ODR &= ~(0x1 << row);

		for(wait = 0; wait < 100; wait++);

		for(col = 0; col < 4; col++)
		{
			col_pin = col + 4;
			if((GPIOC->IDR & (0x1 << col_pin)) == 0)
			{
				GPIOC->ODR |= ROW_ALL;
				return key_map[row][col];
			}
		}
	}

	GPIOC->ODR |= ROW_ALL;
	return 0;
}

char Keypad_Get_Key()
{
	char key = Keypad_Scan();
	if(key == 0) return 0;

	TIM2_Delay(20);
	if(Keypad_Scan() != key) return 0;

	while(Keypad_Scan() != 0);
	TIM2_Delay(20);

	return key;
}
```

---

## 3.2 Servo Motor Driver

TIM4의 PWM(Output Compare) 기능을 이용하여 Servo Motor의 각도를 제어하였다. 20ms 주기(50Hz), 펄스폭 500us(0도)~2400us(180도) 규격에 맞춰 CCR 값을 계산한다.

### 주요 기능

- TIM4 Channel3 PWM 초기화 (PB8, AF02)
- 각도(0~180) → 펄스폭(us) 변환
- 잠금(0도) / 잠금해제(90도) 상태 함수 분리

### 주요 코드

```c
void Servo_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 1);
	Macro_Set_Bit(RCC->APB1ENR, 2);

	Macro_Write_Block(GPIOB->MODER, 0x3, 0x2, 16);   // PB8 => ALT
	Macro_Write_Block(GPIOB->AFR[1], 0xf, 0x2, 0);   // PB8 => AF02 (TIM4_CH3)

	TIM4->PSC = (unsigned int)(TIMXCLK / 1000000.0 + 0.5) - 1;
	TIM4->ARR = SERVO_PERIOD_US - 1;

	Macro_Write_Block(TIM4->CCMR2, 0xff, 0x60, 0);
	TIM4->CCER = (0 << 9) | (1 << 8);

	TIM4->CCR3 = (SERVO_MIN_PULSE + SERVO_MAX_PULSE) / 2;

	Macro_Set_Bit(TIM4->EGR, 0);
	TIM4->CR1 = (1 << 4) | (0 << 3) | (0 << 1) | (1 << 0);
}

void Servo_Set_Angle(int angle)
{
	unsigned int pulse;
	if(angle < 0) angle = 0;
	if(angle > 180) angle = 180;

	pulse = SERVO_MIN_PULSE + (unsigned int)((SERVO_MAX_PULSE - SERVO_MIN_PULSE) * (angle / 180.0));
	TIM4->CCR3 = pulse;
}

void Servo_Lock(void)   { Servo_Set_Angle(0); }
void Servo_Unlock(void) { Servo_Set_Angle(90); }
```

---

## 3.3 Buzzer Driver

TIM3의 PWM 기능으로 지정한 주파수를 일정 시간 재생한 뒤 정지하는 방식으로, 성공/실패에 따라 서로 다른 음계 조합을 재생한다.

### 주요 코드

```c
static void Buzzer_Play_Note(unsigned short freq, int duration_ms)
{
	TIM3_Out_Freq_Generation(freq);
	TIM2_Delay(duration_ms);
	TIM3_Out_Stop();
}

void Buzzer_Correct(void)   // 도-미-솔
{
	Buzzer_Play_Note(NOTE_C4, 150);
	TIM2_Delay(30);
	Buzzer_Play_Note(NOTE_E4, 150);
	TIM2_Delay(30);
	Buzzer_Play_Note(NOTE_G4, 250);
}

void Buzzer_Wrong(void)   // 경고음 두 번
{
	Buzzer_Play_Note(NOTE_LOW1, 200);
	TIM2_Delay(80);
	Buzzer_Play_Note(NOTE_LOW2, 200);
}
```

---

## 3.4 Door Lock Logic

### 비밀번호

```
1234
```

### 동작 과정

```text
시작 (Servo_Lock, LCD: "Enter Password")

↓

Keypad로 4자리 입력 (LCD 2행에 실시간 표시)

↓

'#' 입력 시 비밀번호 확인

├── 일치
│   ├── LCD: "Door Open"
│   ├── Buzzer_Correct() 재생 (도-미-솔)
│   ├── Servo_Unlock() (90도)
│   ├── 5초 대기
│   ├── Buzzer_Close() 재생 (솔-미-도)
│   ├── Servo_Lock() (0도)
│   └── LCD: "Door Closed"
│
└── 불일치
    ├── LCD: "Wrong Password"
    └── Buzzer_Wrong() 재생

↓

입력 버퍼 초기화 후 재입력 화면으로 복귀
```

> 현재 버전은 시도 횟수 제한(예: 3회 실패 시 잠금)은 구현되어 있지 않으며,
> 오답 시 즉시 재입력이 가능하다.

---

# 4. 테스트 결과 (Testing)

## 정상 동작

| 테스트 | 결과 |
|---------|------|
| 올바른 비밀번호 입력 | 통과 |
| Servo Unlock(90도) 동작 | 통과 |
| Servo Lock(0도) 복귀 | 통과 |
| LCD 상태 표시 | 통과 |
| Buzzer 성공 멜로디 | 통과 |

## 오류 처리

| 테스트 | 결과 |
|---------|------|
| 비밀번호 오류 | 통과 |
| Buzzer 경고음 출력 | 통과 |
| 입력 버퍼 초기화 | 통과 |



---

# 5. 결론

본 프로젝트에서는 STM32F411의 GPIO, I2C, Timer(PWM)를 레지스터 레벨에서 직접 제어하여 Door Lock System을 구현하였다.

Key Matrix를 이용한 입력 처리, I2C LCD를 통한 상태 표시, Servo Motor 및 Buzzer 제어를 HAL 없이 CMSIS 레지스터로 직접 구현하면서 GPIO 제어, I2C 통신 프로토콜, PWM 생성 원리를 이해할 수 있었으며, 상태 기반(State Machine) 로직을 적용하여 임베디드 시스템의 기본 구조를 경험할 수 있었다.

---

# 6. 향후 개선 사항

- Interrupt(EXTI) 기반 Key 입력 적용
- 시도 횟수 제한 및 일정 시간 입력 잠금 기능 추가
- 비밀번호 변경 기능 추가
- EEPROM 등을 이용한 비밀번호 저장 기능 추가
- Bluetooth/Wi-Fi 모듈 연동을 통한 원격 상태 확인

---

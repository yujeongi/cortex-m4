#include "device_driver.h"

/*
 * 부저 - TIM3 Channel3 (PB0), timer.c의 TIM3_Out_* 함수 재사용
 *
 * 도어락 시나리오
 * - 정답: 도-미-솔 (상승하는 3음, 밝은 느낌)
 * - 오답: 낮은음 두 번 (경고 느낌)
 */

// 음계 주파수 (Hz) - 4옥타브 기준
#define NOTE_C4   262
#define NOTE_E4   330
#define NOTE_G4   392

#define NOTE_LOW1 150   // 오답용 낮은음 1
#define NOTE_LOW2 130   // 오답용 낮은음 2

void Buzzer_Init(void)
{
	TIM3_Out_Init();   // PB0 => TIM3 PWM 출력 설정 (timer.c에 이미 구현됨)
}

// 한 음을 지정 시간(ms) 동안 재생 후 정지
static void Buzzer_Play_Note(unsigned short freq, int duration_ms)
{
	TIM3_Out_Freq_Generation(freq);
	TIM2_Delay(duration_ms);
	TIM3_Out_Stop();
}

// 정답: 도-미-솔 (상승 3음)
void Buzzer_Correct(void)
{
	Buzzer_Play_Note(NOTE_C4, 150);
	TIM2_Delay(30);   // 음 사이 짧은 무음 간격
	Buzzer_Play_Note(NOTE_E4, 150);
	TIM2_Delay(30);
	Buzzer_Play_Note(NOTE_G4, 250);
}

// 오답: 낮은음 두 번 (경고음)
void Buzzer_Wrong(void)
{
	Buzzer_Play_Note(NOTE_LOW1, 200);
	TIM2_Delay(80);
	Buzzer_Play_Note(NOTE_LOW2, 200);
}

// 문 닫힘: 솔-미-도 (하강 3음, 정답 멜로디의 반대 순서)
void Buzzer_Close(void)
{
	Buzzer_Play_Note(NOTE_G4, 150);
	TIM2_Delay(30);
	Buzzer_Play_Note(NOTE_E4, 150);
	TIM2_Delay(30);
	Buzzer_Play_Note(NOTE_C4, 250);
}

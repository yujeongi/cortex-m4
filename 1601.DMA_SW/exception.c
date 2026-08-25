#include "device_driver.h"
#include <stdio.h>

void _Invalid_ISR(void)
{
	unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
	printf("\nInvalid_Exception: %d!\n", r);
	printf("Invalid_ISR: %d!\n", r - 16);
	for(;;);
}

extern volatile int Key_Pressed;

void EXTI15_10_IRQHandler(void)
{
	Key_Pressed = 1;
	
	EXTI->PR = 0x1 << 13;
	NVIC_ClearPendingIRQ(40);
}

extern volatile int Uart_Data_In;
extern volatile unsigned char Uart_Data;

void USART2_IRQHandler(void)
{
	Uart_Data = (unsigned char)USART2->DR;
	Uart_Data_In = 1;
	NVIC_ClearPendingIRQ(38);
}

/* Exception.c */

extern volatile int TIM4_Expired;

void TIM4_IRQHandler(void)
{
	Macro_Clear_Bit(TIM4->SR, 0);
	NVIC_ClearPendingIRQ(30);
	TIM4_Expired = 1;
}

// DMA
// DMA2 Stream0 Interrupt Handler
// 사용자 전역 변수 선언 (메인 파일 상단 또는 글로벌 변수 정의 파일)
volatile int DMA2_STREAM_DONE[8] = {0};

void DMA2_Stream0_IRQHandler(void)
{
    // Stream 0 Transfer Complete Interrupt Flag (TCIF0: Bit 5) 확인
    if (DMA2->LISR & (1UL << 5)) // 또는 DMA_LISR_TCIF0
    {
        // Stream 0의 모든 인터럽트/에러 플래그 클리어 (TCIF0, TEIF0, DMEIF0, FEIF0 등)
        // Bit 5(TC), 4(HT), 3(TE), 2(DME), 0(FE) 모두 1로 써서 Clear (0x3D)
        DMA2->LIFCR = (0x3DUL << 0); 

        // 메인 루프 통보용 완료 플래그 세팅
        DMA2_STREAM_DONE[0] = 1;
    }
}
void DMA2_Stream1_IRQHandler(void)
{
    // Stream 1 Transfer Complete Interrupt Flag (TCIF1: Bit 11) 확인
    if (DMA2->LISR & (1UL << 11)) // 또는 DMA_LISR_TCIF1
    {
        // Stream 1의 모든 인터럽트/에러 플래그 클리어 (TCIF1, HTIF1, TEIF1, DMEIF1, FEIF1)
        // Bit 11(TC), 10(HT), 9(TE), 8(DME), 6(FE) 모두 1로 써서 Clear (0x3D << 6)
        DMA2->LIFCR = (0x3DUL << 6); 

        // 메인 루프 통보용 완료 플래그 세팅
        DMA2_STREAM_DONE[1] = 1;
    }
}

extern volatile uint8_t g_dma_tx_done;

// DMA1 Stream6 인터럽트 핸들러 (USART2_TX 전용)
void DMA1_Stream6_IRQHandler(void)
{
    // Stream6 Transfer Complete Interrupt Flag (TCIF6) 확인
    if (DMA1->HISR & DMA_HISR_TCIF6)
    {
        // 인터럽트 플래그 클리어 (HIFCR 레지스터의 CTCIF6 비트에 1 쓰기)
        DMA1->HIFCR = DMA_HIFCR_CTCIF6;
        
        g_dma_tx_done = 1;
    }
}
#include "device_driver.h"
#include "stm32f411xe.h"

#define DMA2_STREAM_NUM    8

/* 1. 상태 관리 배열 (volatile 필수) */
static volatile unsigned int DMA_STATUS[DMA2_STREAM_NUM] = {
    DMA_STATUS_READY, DMA_STATUS_READY, DMA_STATUS_READY, DMA_STATUS_READY,
    DMA_STATUS_READY, DMA_STATUS_READY, DMA_STATUS_READY, DMA_STATUS_READY
};

/* 2. 레지스터 포인터 배열 */
static DMA_Stream_TypeDef * DMA2_STREAM[DMA2_STREAM_NUM] = {
    DMA2_Stream0, DMA2_Stream1, DMA2_Stream2, DMA2_Stream3,
    DMA2_Stream4, DMA2_Stream5, DMA2_Stream6, DMA2_Stream7
};

/* -------------------------------------------------------------------------- */
/* DMA2 Stream 초기화                                                          */
/* -------------------------------------------------------------------------- */
void DMA2_Stream_Init(uint8_t stream, void* src_addr, void* dst_addr, union _DMA_SCR_UNI scr1, uint16_t num_data)
{
    if (stream >= DMA2_STREAM_NUM) return;

    // 1. DMA2 클럭 활성화
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    // 2. 이미 실행 중인 경우 Stop 처리
    if (DMA_STATUS[stream] != DMA_STATUS_READY)
    {
        DMA2_Stop(stream);
    }

    // 3. 설정 전 Stream 비활성화 대기 (원본의 안전한 타임아웃 방식 유지)
    uint32_t timeout = 10000;
    DMA2_STREAM[stream]->CR &= ~DMA_SxCR_EN;
    while ((DMA2_STREAM[stream]->CR & DMA_SxCR_EN) && --timeout);

    // 4. 주소 및 데이터 개수 설정
    DMA2_STREAM[stream]->PAR  = (uint32_t)src_addr;
    DMA2_STREAM[stream]->M0AR = (uint32_t)dst_addr;
    DMA2_STREAM[stream]->NDTR = num_data;

    // 5. Control Register 설정
    DMA2_STREAM[stream]->CR = scr1.ui_data;

    // 6. M2M 필수: FIFO 활성화 (DMDIS = 1, FTH = 11)
    DMA2_STREAM[stream]->FCR |= (1UL << 2) | (3UL << 0); 
}

/* -------------------------------------------------------------------------- */
/* DMA2 전송 시작                                                              */
/* -------------------------------------------------------------------------- */
void DMA2_Start(uint8_t stream)
{
    if (stream >= DMA2_STREAM_NUM) return;

    // 상태를 RUN으로 업데이트
    DMA_STATUS[stream] = DMA_STATUS_RUN;

    // 1. 기존 발생한 인터럽트 플래그 클리어 (원본 로직 100% 동일)
    static const uint8_t shift_offset[4] = {0, 6, 16, 22};
    uint32_t shift = shift_offset[stream % 4];

    if (stream < 4)
    {
        DMA2->LIFCR = (0x3DUL << shift);
    }
    else
    {
        DMA2->HIFCR = (0x3DUL << shift);
    }

    // 2. Stream Enable (불필요한 Clear 동작 제거하고 원본처럼 깔끔하게 ON)
    DMA2_STREAM[stream]->CR |= DMA_SxCR_EN;
}

/* -------------------------------------------------------------------------- */
/* DMA2 전송 중지                                                              */
/* -------------------------------------------------------------------------- */
void DMA2_Stop(int stream)
{
    if (stream >= DMA2_STREAM_NUM) return;

    DMA_STATUS[stream] = DMA_STATUS_READY;

    // CR의 EN 비트 클리어 후 완전히 멈출 때까지 대기
    DMA2_STREAM[stream]->CR &= ~DMA_SxCR_EN;
    while (DMA2_STREAM[stream]->CR & DMA_SxCR_EN);
}

/* -------------------------------------------------------------------------- */
/* DMA2 인터럽트 활성화                                                        */
/* -------------------------------------------------------------------------- */
void DMA2_ISR_Enable(uint8_t stream, uint8_t te_ie, uint8_t ht_ie, uint8_t tc_ie)
{
    // 0. 예외 처리 (M4 DMA2 Stream은 0 ~ 7)
    if (stream >= DMA2_STREAM_NUM) return;

    // 1. Stream CR 레지스터의 인터럽트 비트 설정 (Stream이 정지 상태거나 설정 시 안전하게 임시 변수 사용)
    uint32_t cr_temp = DMA2_STREAM[stream]->CR;

    // Transfer Complete Interrupt Enable (TCIE: Bit 4)
    if (tc_ie) cr_temp |= (1UL << 4);
    else       cr_temp &= ~(1UL << 4);

    // Half Transfer Interrupt Enable (HTIE: Bit 3)
    if (ht_ie) cr_temp |= (1UL << 3);
    else       cr_temp &= ~(1UL << 3);

    // Transfer Error Interrupt Enable (TEIE: Bit 2)
    if (te_ie) cr_temp |= (1UL << 2);
    else       cr_temp &= ~(1UL << 2);

    DMA2_STREAM[stream]->CR = cr_temp;

    // 2. Stream 번호에 따른 NVIC IRQ 번호 자동 산출
    // STM32F4: Stream 0~4 (IRQ 56~60), Stream 5~7 (IRQ 68~70)
    IRQn_Type irq_num = (stream < 5) ? (IRQn_Type)(DMA2_Stream0_IRQn + stream)
                                     : (IRQn_Type)(DMA2_Stream5_IRQn + (stream - 5));

    // 3. 하나라도 인터럽트가 활성화되었으면 NVIC Enable, 모두 꺼졌으면 NVIC Disable
    if (tc_ie || ht_ie || te_ie)
    {
        NVIC_ClearPendingIRQ(irq_num); // 기존에 대기 중이던 펜딩 플래그 제거
        NVIC_EnableIRQ(irq_num);
    }
    else
    {
        NVIC_DisableIRQ(irq_num);
    }
}

/* -------------------------------------------------------------------------- */
/* 상태 제어 API                                                              */
/* -------------------------------------------------------------------------- */
void DMA2_Set_Status(int stream, unsigned int status)
{
    if (stream >= DMA2_STREAM_NUM) return;
    DMA_STATUS[stream] = status;
}

unsigned int DMA2_Get_Status(int stream)
{
    if (stream >= DMA2_STREAM_NUM) return DMA_STATUS_READY;

    unsigned int tmp = DMA_STATUS[stream];
    if (tmp == DMA_STATUS_COMPLETE)
    {
        DMA_STATUS[stream] = DMA_STATUS_READY;
    }
    return tmp;
}

// DMA1 Stream6 (USART2 TX) 초기화
void DMA1_Stream6_USART2_TX_Init(void)
{
    // 1. DMA1 클럭 활성화 (USART2 / DMA1은 AHB1 / APB1 버스 소속)
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    // 2. 설정 변경을 위해 Stream 비활성화
    DMA1_Stream6->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream6->CR & DMA_SxCR_EN); // 비활성화 대기

    // 3. 레지스터 초기화
    DMA1_Stream6->CR = 0;

    // 4. DMA 상세 설정
    // - CHSEL: Channel 4 선택 (0x04 << 25) -> USART2_TX 매핑
    // - PL: Priority Level Very High (11b << 16)
    // - MINC: Memory Increment Mode 활성화 (1b << 10)
    // - DIR: Memory-to-Peripheral (01b << 6)
    // - TCIE: Transfer Complete Interrupt Enable (1b << 4)
    DMA1_Stream6->CR |= (4 << DMA_SxCR_CHSEL_Pos)  |
                        (3 << DMA_SxCR_PL_Pos)     |
                        DMA_SxCR_MINC              |
                        (1 << DMA_SxCR_DIR_Pos)    |
                        DMA_SxCR_TCIE;

    // 5. DMA NVIC 인터럽트 활성화
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}

// DMA 전송 시작 함수
void DMA1_Stream6_Start(uint32_t src_addr, uint32_t length)
{
    // 1. Stream 비활성화 확인
    DMA1_Stream6->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream6->CR & DMA_SxCR_EN);

    // 2. Peripheral 주소 (USART2->DR), Memory 주소, 전송 길이 설정
    DMA1_Stream6->PAR = (uint32_t)&(USART2->DR);  // 목적지: USART2 Data Register
    DMA1_Stream6->M0AR = src_addr;               // 출발지: 메모리 주소
    DMA1_Stream6->NDTR = length;                 // 전송 바이트 수

    // 3. 전송 완료 인터럽트 플래그 클리어
    DMA1->HIFCR = DMA_HIFCR_CTCIF6;

    // 4. DMA Stream 활성화 (전송 개시 대기)
    DMA1_Stream6->CR |= DMA_SxCR_EN;
}
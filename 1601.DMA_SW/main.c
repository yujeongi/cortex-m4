#include "device_driver.h"
#include <stdio.h>
#include <string.h>
static void Sys_Init(int baud) 
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
}

volatile int Key_Pressed = 0;
volatile int Uart_Data_In = 0;
volatile unsigned char Uart_Data = 0;
volatile int TIM4_Expired = 0;
# if 0
#define NUM_OF_DATA 40

unsigned int src1_dat[NUM_OF_DATA];
unsigned int dst1_dat[NUM_OF_DATA];

void Main(void)
{
	unsigned int i=0;

	Sys_Init(115200);
	printf("Memory Copy Test\n");

	TIM4_Repeat_Interrupt_Enable(1, 1000);

	for(i=0;i<NUM_OF_DATA;i++)
	{
		src1_dat[i] = i + 8;
		dst1_dat[i] = 0;
	}

	/* Copy from SRC to DST */
	for(i=0;i<NUM_OF_DATA;i++)
	{
		dst1_dat[i] = src1_dat[i];
	}

	/* Verify */
	for(i=0;i<NUM_OF_DATA;i++)
	{
		printf("dst1_dat[%d] = %d\n",i,dst1_dat[i]);
		dst1_dat[i] = 0;
	}

	for(;;)
	{
		if(TIM4_Expired)
		{
			static unsigned int led = 0;
			(led ^= 0x1) ? LED_On() : LED_Off();
			TIM4_Expired = 0;
		}
	}
}
#endif

#if 1

extern volatile int TIM4_Expired;
extern volatile int DMA2_STREAM_DONE[]; // M4의 M2M 전송은 DMA2에서만 가능

#define STREAM          0 // M4는 Channel 대신 Stream을 제어 (예: Stream 0)
#define CHANNEL         0 // M2M 전송 시 Channel 선택 (DMA2에서는 보통 Ch0 사용)
#define NUM_OF_DATA     40

unsigned int src1_dat[NUM_OF_DATA];
unsigned int dst1_dat[NUM_OF_DATA];

void Main(void)
{
    unsigned int i = 0;
    union _DMA_SCR_UNI scr1; // M3의 CCR(Control Register) -> M4의 SCR(Stream Configuration Register)

    Sys_Init(115200);
    printf("2 DMA Software Trigger Test (Cortex-M4)\n");

    TIM4_Repeat_Interrupt_Enable(1, 1000);

    for(i = 0; i < NUM_OF_DATA; i++)
    {
        src1_dat[i] = i + 8;
        dst1_dat[i] = 0;
    }

    scr1.ui_data = 0;

    scr1.st_data.dir = DMA_DIR_M2M;           // Memory-to-Memory
    scr1.st_data.chnsel = CHANNEL;            // Channel 0
    scr1.st_data.circ = DMA_CIRCULAR_DIS;

    // M2M 전송 시 pinc는 Source 주소 증가, minc는 Destination 주소 증가로 매핑됩니다.
    scr1.st_data.minc = DMA_ADDR_INC;         // Destination 주소 증가
    scr1.st_data.pinc = DMA_ADDR_INC;         // Source 주소 증가

    scr1.st_data.msize = DMA_SIZE_32BIT;
    scr1.st_data.psize = DMA_SIZE_32BIT;
    scr1.st_data.pl = DMA_PL_LOW;

    // ★ 중요: Source(src1_dat)가 PAR로, Destination(dst1_dat)이 M0AR로 전달되는지 확인
    DMA2_Stream_Init(STREAM, src1_dat, dst1_dat, scr1, NUM_OF_DATA);
    DMA2_ISR_Enable(STREAM, 0, 0, 1); 
    DMA2_Start(STREAM);

    for(;;)
    {
        if(DMA2_STREAM_DONE[STREAM])
        {
            printf("\nDMA2-Stream%d Complete!\n", STREAM);

            for(i = 0; i < NUM_OF_DATA; i++)
            {
                printf("dst1_dat[%d] = %d\n", i, dst1_dat[i]);
                dst1_dat[i] = 0;
            }

            DMA2_STREAM_DONE[STREAM] = 0;
        }

        if(TIM4_Expired)
        {
            static unsigned int led = 0;
            (led ^= 0x1) ? LED_On() : LED_Off();
            TIM4_Expired = 0;
        }
    }
}
#endif

#if 0
#include <stdio.h>

#define STREAM1         0   /* Stream 0 ~ 7 중 선택 (DMA2 사용) */
#define STREAM2         1   
#define CHANNEL         0   /* M2M 전송 시 Channel은 보통 0 선택 */
#define NUM_OF_DATA     40

extern volatile int TIM4_Expired;
extern volatile int DMA2_STREAM_DONE[]; // M4 M2M 전송은 DMA2 사용

unsigned int src1_dat[NUM_OF_DATA];
unsigned int dst1_dat[NUM_OF_DATA];
unsigned int src2_dat[NUM_OF_DATA];
unsigned int dst2_dat[NUM_OF_DATA];

void Main(void)
{
    unsigned int i = 0;

    union _DMA_SCR_UNI scr1; // M4는 CCR 대신 SCR 사용
    union _DMA_SCR_UNI scr2;

    Sys_Init(115200);
    printf("Multiple DMA Software Trigger Test (Cortex-M4)\n\n");

    
    TIM4_Repeat_Interrupt_Enable(1, 1000);

    for(i = 0; i < NUM_OF_DATA; i++)
    {
        src1_dat[i] = i + 8;
        src2_dat[i] = 99 - i;
        dst1_dat[i] = 0;
        dst2_dat[i] = 0;
    }

    /* ------------------- DMA Stream 1 (scr1) 설정 ------------------- */
    scr1.ui_data = 0;
    scr1.st_data.dir = DMA_DIR_M2M;           // M4 M2M 전송 설정
    scr1.st_data.chnsel = CHANNEL;            // Channel 0
    scr1.st_data.circ = DMA_CIRCULAR_DIS;      // M2M은 Circular 미지원

    // Source(pinc), Destination(minc) 주소 증가 설정
    scr1.st_data.minc = DMA_ADDR_INC;         
    scr1.st_data.pinc = DMA_ADDR_INC;         

    scr1.st_data.msize = DMA_SIZE_32BIT;
    scr1.st_data.psize = DMA_SIZE_32BIT;
    scr1.st_data.pl = DMA_PL_LOW;

    /* ------------------- DMA Stream 2 (scr2) 설정 ------------------- */
    scr2.ui_data = 0;
    scr2.st_data.dir = DMA_DIR_M2M;           // M4 M2M 전송 설정
    scr2.st_data.chnsel = CHANNEL;            // Channel 0
    scr2.st_data.circ = DMA_CIRCULAR_DIS;      // M2M은 Circular 미지원 (DIS로 수정)

    scr2.st_data.minc = DMA_ADDR_INC;         
    scr2.st_data.pinc = DMA_ADDR_INC;         

    scr2.st_data.msize = DMA_SIZE_32BIT;
    scr2.st_data.psize = DMA_SIZE_32BIT;
    scr2.st_data.pl = DMA_PL_VERY_HIGH;

    /* DMA 초기화 (DMA2 사용, 파라미터 순서: Stream, Source, Destination, Config, Count) */
    DMA2_Stream_Init(STREAM1, src1_dat, dst1_dat, scr1, NUM_OF_DATA);
    DMA2_Stream_Init(STREAM2, src2_dat, dst2_dat, scr2, NUM_OF_DATA);

    /* 인터럽트 활성화 (TCIE = 1) */
    DMA2_ISR_Enable(STREAM1, 0, 0, 1);
    DMA2_ISR_Enable(STREAM2, 0, 0, 1);

    /* DMA 전송 시작 */
    DMA2_Start(STREAM1);
    DMA2_Start(STREAM2);

    for(;;)
    {
        /* STREAM1 전송 완료 처리 */
        if(DMA2_STREAM_DONE[STREAM1])
        {
            printf("\nDMA2-Stream%d Complete!\n", STREAM1);

            for(i = 0; i < NUM_OF_DATA; i++)
            {
                printf("dst1_dat[%d] = %d\n", i, dst1_dat[i]);
                dst1_dat[i] = 0;
            }

            DMA2_STREAM_DONE[STREAM1] = 0;
            printf("Restart Stream-%d\n", STREAM1);
            // DMA2_Start(STREAM1); // 재시작
        }

        /* STREAM2 전송 완료 처리 */
        if(DMA2_STREAM_DONE[STREAM2])
        {
            static int cnt = 0;

            printf("\nDMA2-Stream%d Complete! [%d]\n", STREAM2, cnt++);

            for(i = 0; i < NUM_OF_DATA; i++)
            {
                printf("dst2_dat[%d] = %d\n", i, dst2_dat[i]);
                dst2_dat[i] = 0;
            }

            DMA2_STREAM_DONE[STREAM2] = 0;
            printf("Finish Stream-%d\n", STREAM2);
            // DMA2_Stop(STREAM2); // 중지
        }

        /* 타이머 4 주기적 LED 토글 */
        if(TIM4_Expired)
        {
            static unsigned int led = 0;
            (led ^= 0x1) ? LED_On() : LED_Off();
            TIM4_Expired = 0;
        }
    }
}
#endif

#if 0
#include <stdio.h>
const char * str[] = {"[1] The DMA controller performs direct memory transfer by sharing the system bus with the Cortex™-M3 core.\n"
	"[2] The DMA request may stop the CPU access to the system bus for some bus cycles,\n"
	"[3] when the CPU and DMA are targeting the same destination (memory or peripheral).\n"
	"[4] The bus matrix implements round-robin scheduling, thus ensuring at least half of the system bus bandwidth (both to memory and peripheral) for the CPU.\n"
	"[5] After an event, the peripheral sends a request signal to the DMA Controller.\n"
	"[6] The DMA controller serves the request depending on the channel priorities.\n"
	"[7] As soon as the DMA Controller accesses the peripheral, an Acknowledge is sent to the peripheral by the DMA Controller.\n"
	"[8] The peripheral releases its request as soon as it gets the Acknowledge from the DMA Controller.\n"
	"[9] Once the request is deasserted by the peripheral, the DMA Controller release the Acknowledge.\n"};

extern volatile int DMA2_STREAM_DONE[]; // M4 M2M 전송은 DMA2 사용
volatile uint8_t g_dma_tx_done = 0;
void Uart2_Wait_for_TX_Complete(void)
{
    // USART2->SR 레지스터의 TC (Bit 6) 비트가 1이 될 때까지 대기
    while (!(USART2->SR & USART_SR_TC));
}
void Main(void)
{
    unsigned int str_num = 0;
    unsigned int total_strings = sizeof(str) / sizeof(str[0]);

    // 클럭 및 Peripherals 기본 초기화
    Sys_Init(115200);
    printf("DMA(M2P) Test - H/W (USART) Trigger\n\n");
    TIM4_Repeat_Interrupt_Enable(1, 1000);
    Uart2_Wait_for_TX_Complete();
    
   // DMA1 Stream6 초기화
    DMA1_Stream6_USART2_TX_Init();

    // USART2 TX DMA Request 활성화 (CR3 레지스터 Bit 7: DMAT)
    USART2->CR3 |= USART_CR3_DMAT;

    for (;;)
    {
        // 첫 시작이거나, 이전 DMA 전송이 완료된 경우
        if (g_dma_tx_done || (str_num == 0))
        {
            if (str_num < total_strings)
            {
                g_dma_tx_done = 0;

                // DMA1 Stream6을 통한 USART2 TX 전송 시작
                DMA1_Stream6_Start((uint32_t)str[str_num], strlen(str[str_num]));

                str_num++;
            }
            else
            {
                // 모든 전송 완료 시 DMA 비활성화
                DMA1_Stream6->CR &= ~DMA_SxCR_EN;
            }
        }
    }
}
#endif
#include "stm32f4xx.h"
#include "option.h"
#include "macro.h"
#include "malloc.h"

// Uart.c

extern void Uart2_Init(int baud);
extern void Uart2_Send_Byte(char data);
extern void Uart2_RX_Interrupt_Enable(int en);

extern void Uart1_Init(int baud);
extern void Uart1_Send_Byte(char data);
extern void Uart1_Send_String(char *pt);
extern void Uart1_Printf(char *fmt,...);
extern char Uart1_Get_Char(void);
extern char Uart1_Get_Pressed(void);

// SysTick.c

extern void SysTick_Run(unsigned int msec);
extern int SysTick_Check_Timeout(void);
extern unsigned int SysTick_Get_Time(void);
extern unsigned int SysTick_Get_Load_Time(void);
extern void SysTick_Stop(void);

// Led.c

extern void LED_Init(void);
extern void LED_On(void);
extern void LED_Off(void);

// Clock.c

extern void Clock_Init(void);

// Key.c

extern void Key_Poll_Init(void);
extern int Key_Get_Pressed(void);
extern void Key_Wait_Key_Released(void);
extern void Key_Wait_Key_Pressed(void);
extern void Key_ISR_Enable(int en);

// Timer.c

extern void TIM2_Delay(int time);
extern void TIM2_Stopwatch_Start(void);
extern unsigned int TIM2_Stopwatch_Stop(void);
extern void TIM4_Repeat(int time);
extern int TIM4_Check_Timeout(void);
extern void TIM4_Stop(void);
extern void TIM4_Change_Value(int time);
extern void TIM4_Repeat_Interrupt_Enable(int en, int time);
extern void TIM3_Out_Init(void);
extern void TIM3_Out_Freq_Generation(unsigned short freq);
extern void TIM3_Out_Stop(void);

// DMA

// ==========================================
// Cortex-M4 DMA Register & Config Defines
// ==========================================

/* DMA Transfer Direction (DIR) */
#define DMA_DIR_P2M          0x0  // Peripheral-to-Memory
#define DMA_DIR_M2P          0x1  // Memory-to-Peripheral
#define DMA_DIR_M2M          0x2  // Memory-to-Memory (M4 전용)

/* DMA Circular Mode */
#define DMA_CIRCULAR_DIS     0x0
#define DMA_CIRCULAR_EN      0x1

/* DMA Increment Mode */
#define DMA_ADDR_FIXED       0x0
#define DMA_ADDR_INC         0x1

/* DMA Data Size */
#define DMA_SIZE_8BIT        0x0
#define DMA_SIZE_16BIT       0x1
#define DMA_SIZE_32BIT       0x2

/* DMA Priority Level */
#define DMA_PL_LOW           0x0
#define DMA_PL_MEDIUM        0x1
#define DMA_PL_HIGH          0x2
#define DMA_PL_VERY_HIGH     0x3


// ==========================================
// Cortex-M4 DMA Stream Configuration Register (SCR) Union
// ==========================================
union _DMA_SCR_UNI {
    unsigned int ui_data;
    struct {
        unsigned int en       : 1;  // Bit 0: Stream enable
        unsigned int dmeie    : 1;  // Bit 1: Direct mode error interrupt enable
        unsigned int teie     : 1;  // Bit 2: Transfer error interrupt enable
        unsigned int htie     : 1;  // Bit 3: Half transfer interrupt enable
        unsigned int tcie     : 1;  // Bit 4: Transfer complete interrupt enable
        unsigned int pfctrl   : 1;  // Bit 5: Peripheral flow controller
        unsigned int dir      : 2;  // Bit 6-7: Data transfer direction (00:P2M, 01:M2P, 10:M2M)
        unsigned int circ     : 1;  // Bit 8: Circular mode
        unsigned int pinc     : 1;  // Bit 9: Peripheral increment mode
        unsigned int minc     : 1;  // Bit 10: Memory increment mode
        unsigned int psize    : 2;  // Bit 11-12: Peripheral data size
        unsigned int msize    : 2;  // Bit 13-14: Memory data size
        unsigned int pincos   : 1;  // Bit 15: Peripheral increment offset size
        unsigned int pl       : 2;  // Bit 16-17: Priority level
        unsigned int dbm      : 1;  // Bit 18: Double buffer mode
        unsigned int ct       : 1;  // Bit 19: Current target (in double buffer mode)
        unsigned int ack      : 1;  // Bit 20: Reserved
        unsigned int pburst   : 2;  // Bit 21-22: Peripheral burst transfer configuration
        unsigned int mburst   : 2;  // Bit 23-24: Memory burst transfer configuration
        unsigned int chnsel   : 3;  // Bit 25-27: Channel selection
        unsigned int reserved : 4;  // Bit 28-31: Reserved
    } st_data;
};


#define DMA_STATUS_READY			(0)
#define DMA_STATUS_RUN				(1)
#define DMA_STATUS_COMPLETE			(2)
#define DMA_STATUS_HALFTRANSFERED	        (3)
#define DMA_STATUS_ERROR			(4)

extern void DMA2_Stream_Init(uint8_t stream, void* src_addr, void* dst_addr, union _DMA_SCR_UNI scr1, uint16_t num_data);
extern void DMA2_ISR_Enable(uint8_t stream, uint8_t te_ie, uint8_t ht_ie, uint8_t tc_ie);
extern void DMA2_Start(uint8_t stream);
extern DMA_Stream_TypeDef* Get_DMA2_Stream(uint8_t stream);
extern unsigned int DMA2_Get_Status(int stream);
extern void DMA2_Set_Status(int stream, unsigned int status);
extern void DMA2_Stop(int stream);
extern void DMA1_Stream6_Start(uint32_t src_addr, uint32_t length);
extern void DMA1_Stream6_USART2_TX_Init(void);
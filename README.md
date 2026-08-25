# STM32F411 Cortex-M4 Projects

STM32F411RE(Nucleo-64) 기반 Cortex-M4 실습 및 미니 프로젝트 산출물입니다. HAL 대신 CMSIS와 주변장치 레지스터를 직접 제어하는 Bare-metal C 방식으로 작성했습니다.

## 프로젝트 구성

### `1601.DMA_SW`

- STM32F411용 기본 드라이버 및 DMA 실습 소스
- GPIO, UART, Timer, SysTick, Key, LED, Exception 처리 코드
- ARM GNU Toolchain용 `Makefile`, 시작 코드, 링커 스크립트 포함
- 빌드에 필요한 시작 코드와 링커 스크립트 포함

### `김유정_260723-260727_Cortex-M4`

4x4 Key Matrix, I2C LCD, Servo Motor, Buzzer를 사용한 도어락 미니 프로젝트입니다.

- 비밀번호 입력 및 확인
- I2C LCD 상태 표시
- TIM4 PWM 기반 SG90 Servo 제어
- TIM3 PWM 기반 성공/실패 부저 멜로디
- UART2 디버그 로그

상세 설계, 핀맵, 주요 코드와 테스트 결과는 [미니 프로젝트 보고서](./김유정_260723-260727_Cortex-M4/README.md)에서 확인할 수 있습니다.

실제 구동 모습은 [동작 영상](./김유정_260723-260727_Cortex-M4/김유정_260723-260727_Cortex-M4_동작영상.mp4)에서 확인할 수 있습니다.

## 개발 환경

| 항목 | 내용 |
| --- | --- |
| MCU | STM32F411RE (Nucleo-64) |
| 언어 | C, ARM Assembly |
| 개발 방식 | Bare-metal, CMSIS 레지스터 직접 제어 |
| Toolchain | Arm GNU Toolchain 15.2.1 (`arm-none-eabi`) |
| 주요 주변장치 | GPIO, UART, I2C, DMA, Timer/PWM, SysTick |

## 빌드 및 실행

`1601.DMA_SW/Makefile`의 `TOOL_DIR`을 로컬 Arm GNU Toolchain 설치 경로에 맞게 수정한 뒤 빌드합니다.

```sh
cd 1601.DMA_SW
make
```

STM32CubeProgrammer CLI가 설치되어 있다면 다음 명령으로 보드에 기록할 수 있습니다.

```sh
make run
```

> 저장소에는 소스 코드, 문서와 프로젝트 동작 영상을 보관하며, 다시 생성할 수 있는 빌드 결과물은 제외했습니다.

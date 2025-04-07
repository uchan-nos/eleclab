#include <stddef.h>
#include <stdint.h>

/********
 各種設定
 ********/
// MSMP のボーレート
#define MSMP_BAUDRATE 9600
// 信号記録のサンプリングレート
#define SIG_RECORD_RATE_SCALE 10
#define SIG_RECORD_RATE (MSMP_BAUDRATE * SIG_RECORD_RATE_SCALE) // 96KHz
// メッセージ受信バッファサイズ（バイト）
#define MSG_BUF_SIZE 1024 // 1KB
// 信号記録バッファサイズ（バイト）
#define SIG_BUF_SIZE (17*1024)
// MSMP 通信に用いる USART コンポーネントの番号（USARTn）
#define MSMP_USART_NUM 1
// MSMP 送信メッセージバッファサイズ（バイト）
#define TX_BUF_LEN 65
// デバッガ操作に用いる USART コンポーネントの番号
#define CMD_USART_NUM 2
// USART のピン
#define USART1_TX_PIN PA9
#define USART1_RX_PIN PA10
#define USART2_TX_PIN PA2
#define USART2_RX_PIN PA3

/************************
 設定値から導出される定数
 ************************/
// 信号記録の周期
#define SIG_RECORD_TIM_PERIOD (FUNCONF_SYSTEM_CORE_CLOCK / SIG_RECORD_RATE)
// メッセージ受信バッファの要素数
#define MSG_BUF_LEN (MSG_BUF_SIZE / sizeof(struct Message))
// 信号記録バッファの要素数
#define SIG_BUF_LEN (SIG_BUF_SIZE / sizeof(tick_t))

// USART 関連マクロの定義補助
#define CONCAT_USART_AGAIN(num, postfix) USART ## num ## postfix
// 2 段階のマクロ展開にすることで MSMP_USART_NUM の展開後の値をトークン結合する
// see: https://www.jpcert.or.jp/sc-rules/c-pre05-c.html
#define CONCAT_USART(num, postfix) CONCAT_USART_AGAIN(num, postfix)

// MSMP 通信に用いる USART コンポーネント
#define MSMP_USART_ID(postfix) CONCAT_USART(MSMP_USART_NUM, postfix)
#define MSMP_USART MSMP_USART_ID()
#define MSMP_USART_IRQn MSMP_USART_ID(_IRQn)
#define MSMP_USART_IRQHandler MSMP_USART_ID(_IRQHandler)
#define MSMP_TX_PIN MSMP_USART_ID(_TX_PIN)
#define MSMP_RX_PIN MSMP_USART_ID(_RX_PIN)
// デバッガ操作に用いる USART コンポーネント
#define CMD_USART_ID(postfix) CONCAT_USART(CMD_USART_NUM, postfix)
#define CMD_USART CMD_USART_ID()
#define CMD_USART_IRQn CMD_USART_ID(_IRQn)
#define CMD_USART_IRQHandler CMD_USART_ID(_IRQHandler)
#define CMD_TX_PIN CMD_USART_ID(_TX_PIN)
#define CMD_RX_PIN CMD_USART_ID(_RX_PIN)

typedef uint32_t tick_t;

struct Message {
  tick_t start_tick; // スタートビットを受信したときの時刻
  union {
    struct {
      uint8_t addr;
      uint8_t len;
      uint8_t body[63];
    };
    uint8_t raw_msg[65];
  } __attribute__((packed));
};

enum MSMPState {
  MSTATE_IDLE, // 先頭バイトを待っている状態
  MSTATE_ADDR, // 先頭バイトを受信中
  MSTATE_LEN,  // メッセージ長を受信中
  MSTATE_BODY, // メッセージボディを受信中
};

/*************
 * msmpdbg.c *
 *************/
// 送受信同時デバッグモード
extern volatile tick_t tick;
extern volatile bool transmit_on_receive_mode;

/*******************
 * msmp_recorder.c *
 *******************/
volatile enum MSMPState msmp_state;
extern volatile struct Message msg_buf[MSG_BUF_LEN];
extern volatile size_t msg_wpos; // msg の書き込み位置
extern volatile size_t raw_msg_wpos; // msg.raw_msg の書き込み位置
// 受信信号の 0/1 が切り替わった時刻のリスト
// sig[0] はスタートビットの受信時刻、sig[1] はその次に信号が 1 になった時刻
extern volatile tick_t sig_buf[SIG_BUF_LEN];
extern volatile size_t sig_wpos;
extern volatile bool sig_record_mode; // 真なら信号を記録する
extern volatile uint32_t sig_record_period_ticks; // 信号を記録する期間


// MSMP 送信データバッファ
extern uint8_t msmp_transmit_queue[TX_BUF_LEN];
extern size_t msmp_transmit_queue_len;
extern volatile size_t msmp_transmit_rpos;

/* RX ピンの状態を入力
 *
 * @param tick  現在時刻
 * @param sig   RX ピンの状態
 * @return MSMP メッセージの先頭バイトのスタートビットを受信したら真
 */
bool SenseSignal(tick_t tick, bool sig);

/* 記録された信号をグラフ化して表示 */
void PlotSignal();

/* 受信された 1 バイトを処理 */
void ProcByte(uint8_t c);

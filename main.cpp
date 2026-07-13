#define F_CPU 16000000UL

#include "funsape/globalDefines.hpp"
#include "funsape/peripheral/gpioPin.hpp"
#include "funsape/peripheral/gpioBus.hpp"

#include "funsape/peripheral/timer1.hpp"
#include "funsape/peripheral/timer0.hpp"
#include "funsape/peripheral/pcint1.hpp"

#include "funsape/peripheral/usart0.hpp"

//Macro definitions ------------------------------------------------------------

#define defineBit(var, pos, value) ((uint8_t)((var & (1 << pos)) ? (var & (~((!((bool_t)value)) << pos)) ) : (var | (bool_t)value << pos)))
#define MSB(x) ((uint8_t)((x >> 8) & 0xFF))
#define LSB(x) ((uint8_t)(x & 0xFF))
#define ABS(x) ((x>0) ? (x) : (-x))
#define ENABLE_BUTTON 1
#define LEFT_BUTTON 4
#define RIGHT_BUTTON 2

#define PACKET_SIZE 10

//Data structure declarations --------------------------------------------------

enum OP_MODE{
  IDLE_MODE,
  CURSOR_MODE,
  SCROLL_MODE
};

enum BT_STATE{
    BT_CONNECTED,
    BT_DISCONNECTED
};


//Function prototype definition ------------------------------------------------

uint32_t get_millis();
uint8_t spi_writeRead(uint8_t data);
void transcieve_packets(uint8_t *packet, uint8_t *recv_packet, size_t len);
void format_packet(uint16_t x, uint16_t y, uint8_t buttons, int16_t scroll, OP_MODE mode, uint8_t *packet);
void convert_recieved(int16_t *roll, int16_t *pitch, \
     int16_t *yaw, BT_STATE *state, uint8_t *packet);

// GLOBAL VARIABLE DECLARATIONS ================================================

//GPIO Declarations ------------------------------------------------------------
GpioPin led;
GpioBus buttons;
GpioPin slave_select_esp;

//Timer counter ----------------------------------------------------------------
volatile uint32_t timer_millis = 0;

//Button and debounce variables ------------------------------------------------
volatile bool_t button_pending_change = false;
volatile uint32_t button_last_changed[3];
volatile uint8_t debounced_buttons = 0;

//SPI variables ----------------------------------------------------------------
volatile bool spi_flag = false;

//STATE
OP_MODE current_state = IDLE_MODE;

int main(){
    
    // Config do Pcint1 --- Botões do mouse ------------------------------------
    {
    pcint1.setPinMode(Pcint1::Pin::PIN_PCINT8 | Pcint1::Pin::PIN_PCINT9 | Pcint1::Pin::PIN_PCINT10, PinMode::INPUT_PULLED_UP); 
    pcint1.init(Pcint1::Pin::PIN_PCINT8 | Pcint1::Pin::PIN_PCINT9 | Pcint1::Pin::PIN_PCINT10);
    pcint1.clearInterruptRequest();
    pcint1.activateInterrupt();
    }

    // Configure GPIO --- Led / Botoẽs do mouse / SS esp -----------------------
    {
    // led.init(&PORTB, PinIndex::P5);
    // led.setMode(PinMode::OUTPUT_PUSH_PULL);
    
    buttons.init(&PORTC, PinIndex::P0, 3);
    buttons.setMode(PinMode::INPUT_PULLED_UP);

    slave_select_esp.init(&PORTB, PinIndex::P1);
    slave_select_esp.setMode(PinMode::OUTPUT_PUSH_PULL);
    }

    // Config SPI --- Comm com ESP ---------------------------------------------
    {
    //SS, MISO, MOSI, SCK
    clrBit(PRR, PRSPI);

    setBit(DDRB, (uint8_t)PinIndex::P2); // SS
    setBit(DDRB, (uint8_t)PinIndex::P3); // MOSI
    clrBit(DDRB, (uint8_t)PinIndex::P4); // MISO
    setBit(DDRB, (uint8_t)PinIndex::P5); // SCK


    // setBit(SPCR, 6); //BIT 6 SPE - SPI Enable
    // clrBit(SPCR, 5); //BIT 5 DORD - Data order, 0 - MSB
    // setBit(SPCR, 4); //BIT 4 MSTR - Set master
    // clrBit(SPCR, 3); //BIT 3 CPOL - Clock Polarity, mode 0 - 0
    // clrBit(SPCR, 2); //BIT 2 CPHA - Clock Phase, mode 0 - 0

    // clrBit(SPCR, SPR1); //BIT 1 SPR1 - Clock prescaler - Prescaler 16
    // setBit(SPCR, SPR0); //BIT 0 SPR2 - Clock prescaler - Prescaler 16
    SPCR = 0b1010001;
    }

    // Config Timer 0 --- 1ms --------------------------------------------------
    {
    timer0.init(Timer0::Mode::CTC_OCRA, //Colocamos quantos ciclos pra chegar na interrupção
            Timer0::ClockSource::PRESCALER_64);//Velocidade que e dividida a cpu
    timer0.setCompareAValue(249); //Fala quantos ticks precisam para chegar a 10ms
    timer0.clearCompareAInterruptRequest(); //Limpar o pre interrupt
    timer0.activateCompareAInterrupt();
    }

    // Config Timer 1 --- 5ms -------------------------------------------------
    {
    timer1.init(Timer1::Mode::CTC_OCRA, //Colocamos quantos ciclos pra chegar na interrupção
            Timer1::ClockSource::PRESCALER_64);//Velocidade que e dividida a cpu
    timer1.setCompareAValue(1250*4); // 50ms
    timer1.clearCompareAInterruptRequest(); //Limpar o pre interrupt
    timer1.activateCompareAInterrupt();
    }
    
    // Config Uart --- Debug ---------------------------------------------------
    {
    usart0.init();
    usart0.setBaudRate(Usart0::BaudRate::BAUD_RATE_9600);
    usart0.setFrameFormat(Usart0::FrameFormat::FRAME_FORMAT_8_E_1);
    usart0.enableTransmitter();
    usart0.enableReceiver();
    usart0.stdio();
    }
    
    // Set enable interrupts ---------------------------------------------------
    sei();

    // Local variable declarations ---------------------------------------------
    uint16_t x {0}, y {32000};
    uint8_t packet[10];
    uint8_t recv_packet[10];
    int16_t pitch {0}, roll {0}, yaw {0};
    BT_STATE state = BT_DISCONNECTED;
    uint8_t last_debounced_buttons = 0;

    uint8_t scroll_count = 0;

    printf("Inicializando Atmega\n");

    while (true){
        uint8_t button_data = buttons.read();
        if(button_pending_change){
            button_pending_change = false;
            
            uint32_t current_milis = get_millis();
            for(uint8_t i = 0; i < 3; i++)
                if((current_milis - button_last_changed[i]) > 25){
                    debounced_buttons = defineBit(debounced_buttons, i, !(bool)(button_data & (1 << i)));
                }else{
                    button_pending_change = true;
                }
        }

        if(((debounced_buttons ^ last_debounced_buttons) & ENABLE_BUTTON) && (debounced_buttons & ENABLE_BUTTON)){
            current_state = (current_state == IDLE_MODE) ? SCROLL_MODE : IDLE_MODE;
        }

        switch (current_state){
            case IDLE_MODE:

                break;
            case CURSOR_MODE:
                x = (yaw+450)*36;
                y = (-roll-1700)*82;

                if(spi_flag){
                        format_packet(x, y, debounced_buttons, 0, current_state, packet);
                        transcieve_packets(packet, recv_packet, PACKET_SIZE);
                        convert_recieved(&roll, &pitch, &yaw, &state, recv_packet);
                        spi_flag = false;
            
                        // printf("State: %u  Pitch: %i.%u  Roll: %i.%u  Yaw: %i.%u SENDING: X: %u  Y: %u\r\n", state, pitch/10, ABS(pitch%10), roll/10, ABS(roll%10), yaw/10, ABS(yaw%10), x, y);
                        // printf("Buttons: %u %u %u\r\n", (bool_t)(debounced_buttons&(1<<0)), (bool_t)(debounced_buttons&(1<<1)), (bool_t)(debounced_buttons&(1<<2)));
                }
                    
                break;
            case SCROLL_MODE:
                if(spi_flag){
                    scroll_count++;
                    // printf("Scroll count: %u\r\n", scroll_count);
                    if(scroll_count >= 10){
                        scroll_count = 0;
                        int16_t scroll = (-roll-1700)/20;

                        format_packet(0, 0, debounced_buttons, scroll, current_state, packet);
                        transcieve_packets(packet, recv_packet, PACKET_SIZE);
                        convert_recieved(&roll, &pitch, &yaw, &state, recv_packet);
                        spi_flag = false;
            
                        printf("State: %u  Pitch: %i.%u  Roll: %i.%u  Yaw: %i.%u SENDING: SCROLL: %i\r\n", state, pitch/10, ABS(pitch%10), roll/10, ABS(roll%10), yaw/10, ABS(yaw%10), scroll);
                        // printf("Buttons: %u %u %u\r\n", (bool_t)(debounced_buttons&(1<<0)), (bool_t)(debounced_buttons&(1<<1)), (bool_t)(debounced_buttons&(1<<2)));
                    }
                }

                break;
            default:
                ;
            }
            
    
            last_debounced_buttons = debounced_buttons;
    }
}

uint8_t spi_writeRead(uint8_t data){
    SPDR = data;
    while(!(SPSR & (1<<SPIF)));

    return SPDR;
}

void transcieve_packets(uint8_t *packet, uint8_t *recv_packet, size_t len) {
  // take the SS pin low to select the chip:
  slave_select_esp.low();
//   delayMs(1);
  // send in the address and value via SPI:
  
  for(uint8_t i = 0; i < len; i++){
    recv_packet[i] = spi_writeRead(packet[i]);
  }

//   delayMs(1);
  // take the SS pin high to de-select the chip:
  slave_select_esp.high();
}

void format_packet(uint16_t x, uint16_t y, uint8_t buttons, int16_t scroll, OP_MODE mode, uint8_t *packet){
  packet[0] = 0x67;
  packet[1] = (uint8_t)mode;
  packet[2] = MSB(x);
  packet[3] = LSB(x);
  packet[4] = MSB(y);
  packet[5] = LSB(y);
  packet[6] = ((bool_t)(buttons & LEFT_BUTTON)) + (((bool_t)(buttons & RIGHT_BUTTON)) << 1);
  packet[7] = (scroll >> 8) & 0xFF;
  packet[8] = scroll & 0xFF;
  packet[9] = 0;
}

void convert_recieved(int16_t *roll, int16_t *pitch, int16_t *yaw, BT_STATE *state, uint8_t *packet){
  if(packet[3] != 0x68) return;

  *state = (BT_STATE)packet[0];
  *pitch = (packet[1] << 8) + packet[2];
  *roll = (packet[4] << 8) + packet[5];
  *yaw = (packet[6] << 8) + packet[7];
}

uint32_t get_millis() {
    uint32_t m;
    cli(); // Desativa interrupções temporariamente para leitura atômica
    m = timer_millis;
    sei(); // Reativa interrupções
    return m;
}

void pcint1InterruptCallback(void){
    uint8_t button_data = buttons.read();
    for(uint8_t i = 0; i < 3; i++){
        if((button_data & (1 << i)) != (debounced_buttons & (1 << i))){
            button_last_changed[i] = get_millis();
            button_pending_change = true;
        }else
            button_last_changed[i] = 0;
    }

}

void timer1CompareACallback(void){
    //A cada 50ms
    spi_flag = true;    
    
}

void timer0CompareACallback(void){
    //A cada 1 ms
    timer_millis++;
}
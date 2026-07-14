#define F_CPU 16000000UL

#include "funsape/globalDefines.hpp"
#include "funsape/peripheral/gpioPin.hpp"
#include "funsape/peripheral/gpioBus.hpp"

#include "funsape/peripheral/timer1.hpp"
#include "funsape/peripheral/timer0.hpp"
#include "funsape/peripheral/pcint1.hpp"

#include "funsape/peripheral/usart0.hpp"

#include "funsape/peripheral/twi.hpp"
#include "funsape/device/ssd1306.hpp"
#include "funsape/util/intTrig.hpp"
//Macro definitions ------------------------------------------------------------

#define defineBit(var, pos, value) ((uint8_t)((var & (1 << pos)) ? (var & (~((!((bool_t)value)) << pos)) ) : (var | (bool_t)value << pos)))
#define MSB(x) ((uint8_t)((x >> 8) & 0xFF))
#define LSB(x) ((uint8_t)(x & 0xFF))
#define ABS(x) ((x>0) ? (x) : (-x))
#define MAX(a,b) ((a>b) ? (a) : (b))
#define MIN(a,b) ((b>a) ? (a) : (b))
#define SIGN(a) ((a>0) ? (1) : ((a<0) ? (-1) : (0)))

#define ENABLE_BUTTON 1
#define LEFT_BUTTON 4
#define RIGHT_BUTTON 2

#define PACKET_SIZE 16

//Data structure declarations --------------------------------------------------

enum OP_MODE{
  IDLE_MODE,
  CURSOR_MODE,
  SCROLL_MODE,
  CALIBRATION_MODE
};

enum BT_STATE{
    BT_CONNECTED,
    BT_DISCONNECTED
};


//Function prototype definition ------------------------------------------------

uint32_t get_millis();
uint32_t micros();
uint8_t spi_writeRead(uint8_t data);
void transcieve_packets(uint8_t *packet, uint8_t *recv_packet, size_t len);

void format_packet(uint16_t x, uint16_t y, uint8_t buttons, \
    int16_t scroll, OP_MODE mode, uint8_t *packet);

void convert_recieved(int32_t *roll, int32_t *pitch, \
    int32_t *yaw, int16_t *accel_x, BT_STATE *state, uint8_t *packet);

uint16_t projectTan(int32_t angle_mdeg, int32_t center_mdeg,
                    int32_t halfRange_mdeg, uint16_t sens_q8);

// GLOBAL VARIABLE DECLARATIONS ================================================

//GPIO Declarations ------------------------------------------------------------
GpioPin led;
GpioBus buttons;
GpioPin slave_select_esp;

//OLED Display -----------------------------------------------------------------
Ssd1306 display;
uint8_t oledBuffer[1024];
volatile bool update_display_flag = false;
volatile bool clear_display_flag = false;

//Timer counter ----------------------------------------------------------------
volatile uint32_t timer1_overflows = 0;

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
    timer0.init(Timer0::Mode::NORMAL, //Colocamos quantos ciclos pra chegar na interrupção
            Timer0::ClockSource::PRESCALER_1024);//Velocidade que e dividida a cpu
    timer0.setCompareAValue(155); //Fala quantos ticks precisam para chegar a 10ms
    timer0.clearCompareAInterruptRequest(); //Limpar o pre interrupt
    timer0.activateCompareAInterrupt();
    }

    // Config Timer 1 --- 5ms -------------------------------------------------
    {
    timer1.init(Timer1::Mode::NORMAL, //Colocamos quantos ciclos pra chegar na interrupção
            Timer1::ClockSource::PRESCALER_8);//Velocidade que e dividida a cpu
    // timer1.setCompareAValue(1250*4); // 50ms
    timer1.clearOverflowInterruptRequest(); //Limpar o pre interrupt
    timer1.activateOverflowInterrupt();
    }
    
    // Config Uart --- Debug ---------------------------------------------------
    {
    usart0.init();
    usart0.setBaudRate(Usart0::BaudRate::BAUD_RATE_115200);
    usart0.setFrameFormat(Usart0::FrameFormat::FRAME_FORMAT_8_E_1);
    usart0.enableTransmitter();
    usart0.enableReceiver();
    usart0.stdio();
    }

    // Set enable interrupts ---------------------------------------------------
    sei();

    // Config OLED SSD1306 --- Display I2C -------------------------------------
    // (TWI a 400kHz, buffer de 64 bytes; interrupcoes ja habilitadas com sei())
    {
    twi.init(400000, 64);

    display.init(&twi, Ssd1306::Size::SSD1306_128X64, 0x3C, oledBuffer);

    display.setUpsideDown(true);
    display.clearScreen();
    
    // display.drawRect(0, 5, 128, 3);     // contorno cobrindo 3 paginas
    // display.fillRect(4, 6, 40, 1, true);// barra solida
    }



    // Local variable declarations ---------------------------------------------
    uint16_t x {0}, y {0};
    uint8_t packet[PACKET_SIZE], recv_packet[PACKET_SIZE];
    int32_t pitch {0}, roll {0}, yaw {0};
    int16_t accel_x {0};
    uint8_t sense_x {20}, sense_y{20}; 

    BT_STATE ble_state = BT_DISCONNECTED;
    uint8_t last_debounced_buttons = 0;

    uint8_t idle_count = 0;

    uint32_t last_mouse_button_pressed {0};

    printf("Inicializando Atmega\n");

    while (true){
        uint8_t button_data = buttons.read();
        if(button_pending_change){
            button_pending_change = false;
            
            uint32_t current_milis = get_millis();
            for(uint8_t i = 0; i < 3; i++)
                if((current_milis - button_last_changed[i]) > 15){
                    debounced_buttons = defineBit(debounced_buttons, i, !(bool)(button_data & (1 << i)));
                }else{
                    button_pending_change = true;
                }
        }

        uint8_t pressed_buttons = (debounced_buttons ^ last_debounced_buttons) & debounced_buttons;

        if((debounced_buttons & ENABLE_BUTTON) && (get_millis()-button_last_changed[ENABLE_BUTTON] > 2000 )){
            current_state = CALIBRATION_MODE;
        }

        if((pressed_buttons & ENABLE_BUTTON) && (debounced_buttons & ENABLE_BUTTON)){
            current_state = (current_state == IDLE_MODE) ? CURSOR_MODE : IDLE_MODE;
        }
        if((pressed_buttons & (RIGHT_BUTTON | LEFT_BUTTON)) && (debounced_buttons & (RIGHT_BUTTON | LEFT_BUTTON))){
            last_mouse_button_pressed = get_millis();
        }

        // printf("State: %s", (current_state == IDLE_MODE) ? "IDLE" : ((current_state == CURSOR_MODE) ? "CURSOR" : "SCROLL"));
        if(update_display_flag){
            display.clearScreen();
            display.setCursor(0, 7);
            display.setTextSize(1);
            char formatted_state[22];
            sprintf_P(formatted_state, PSTR("State: %S|BLE: %S"), 
                (current_state == IDLE_MODE) ? PSTR("IDLE") : 
                (current_state == CURSOR_MODE) ? PSTR("CURSOR") : 
                (current_state == SCROLL_MODE) ? PSTR("SCROLL") : PSTR("ADJUST"),
                (ble_state == BT_CONNECTED) ? PSTR("OK") : PSTR("NO"));
            display.print(formatted_state);
            
            int8_t bar_roll = MIN(MAX((roll/1410), -63), 63);
            display.fillRect(63+MIN(0, bar_roll), 0, (cuint8_t)ABS(bar_roll), 1, true);                    
            
            int8_t bar_pitch = MIN(MAX((pitch/1410), -63), 63);
            display.fillRect(63+MIN(0, bar_pitch), 1, (cuint8_t)ABS(bar_pitch), 1, true);                    
            
            int8_t bar_yaw = MIN(MAX((yaw/1410), -63), 63);
            display.fillRect(63+MIN(0, bar_yaw), 2, (cuint8_t)ABS(bar_yaw), 1, true);                    

            display.drawPixel(x>>8, y>>9);
            
            display.setCursor(10,5);

            char formatted_sense[21];
            sprintf_P(formatted_sense, PSTR("SENSE X: %u.%ux Y: %u.%ux"), sense_x/10, sense_x%10, sense_y/10, sense_y%10);
            display.setCursor(0,5);            
            display.print(formatted_sense);

            display.display();
            update_display_flag = false;
        }

        switch (current_state){
            case IDLE_MODE:
                if(spi_flag){
                    spi_flag=false;
                    idle_count++;
                    if(idle_count >= 5){
                        
                        idle_count = 0;
                        format_packet(x, y, 0, 0, current_state, packet);
                        transcieve_packets(packet, recv_packet, PACKET_SIZE);
                        convert_recieved(&roll, &pitch, &yaw, &accel_x, &ble_state, recv_packet);
                        spi_flag = false;
            
                        // printf("State: %u  Pitch: %i.%u  Roll: %i.%u  Yaw: %i.%u SENDING: SCROLL: %i\r\n", state, pitch/10, ABS(pitch%10), roll/10, ABS(roll%10), yaw/10, ABS(yaw%10), scroll);
                        // printf("Buttons: %u %u %u\r\n", (bool_t)(debounced_buttons&(1<<0)), (bool_t)(debounced_buttons&(1<<1)), (bool_t)(debounced_buttons&(1<<2)));
                    }
                }
                break;
            case CURSOR_MODE:
                if (pitch > 60000){current_state = SCROLL_MODE; break;}

                {

                    if(get_millis() - last_mouse_button_pressed > 100){
                        // int32_t raw_x = ((int32_t)(yaw+45000)*410*sense_x)/10000;
                        // int32_t raw_y = ((int32_t)pitch*655*sense_y)/10000;

                        // yaw no eixo X: 0 -> centro, -45 -> esquerda, +45 -> direita
                        int32_t raw_x = projectTan(yaw,   0,     45000, sense_x);
                        // pitch no eixo Y: 0..50 -> tela toda (centro em 25)
                        int32_t raw_y = projectTan(pitch, 25000, 25000, sense_y);
                        
                        x = (int16_t)truncateBetween(raw_x, 0, 32767);
                        y = 32767 - (int16_t)truncateBetween(raw_y, 0, 32767);

                    }

                    if(spi_flag){
                            format_packet(x, y, debounced_buttons, 0, current_state, packet);
                            transcieve_packets(packet, recv_packet, PACKET_SIZE);
                            convert_recieved(&roll, &pitch, &yaw, &accel_x, &ble_state, recv_packet);
                            spi_flag = false;
                
                            // printf("State: %u  Pitch: %li  Roll: %li  Yaw: %li SENDING: X: %u  Y: %u\r\n", ble_state, pitch, roll, yaw, x, y);
                            // printf("Buttons: %u %u %u\r\n", (bool_t)(debounced_buttons&(1<<0)), (bool_t)(debounced_buttons&(1<<1)), (bool_t)(debounced_buttons&(1<<2)));
                    }
                }        
                break;
            case SCROLL_MODE:
                if (pitch < 50000){current_state = CURSOR_MODE; break;}

                if(spi_flag){
                    int8_t scroll_packet = (int8_t)(ABS(yaw) > 15000) ? (int8_t)(yaw/15000) : 0;

                    format_packet(x, y, debounced_buttons, scroll_packet, current_state, packet);
                    transcieve_packets(packet, recv_packet, PACKET_SIZE);
                    convert_recieved(&roll, &pitch, &yaw, &accel_x, &ble_state, recv_packet);
                    
                    printf("State: %u  Pitch:  %li  Roll: %li  Yaw: %li Accel_x: %d.%u SENDING: SCROLL: %d\r\n", ble_state, pitch, roll, yaw, accel_x/10, ABS(accel_x%10), scroll_packet);

                    spi_flag=false;
                }

                break;
            case CALIBRATION_MODE:
                if(pressed_buttons & LEFT_BUTTON){
                    sense_x = (sense_x+5)%45;
                }
                if(pressed_buttons & RIGHT_BUTTON){
                    sense_y = (sense_y+5)%45;
                }

                if(spi_flag){
                        format_packet(x, y, 0, 0, current_state, packet);
                        transcieve_packets(packet, recv_packet, PACKET_SIZE);
                        convert_recieved(&roll, &pitch, &yaw, &accel_x, &ble_state, recv_packet);
                        
                        spi_flag=false;
                    }
                break;
            default:
                ;
            }
            
    
            last_debounced_buttons = debounced_buttons;
    }
}

//! Projeta um angulo (em milesimos de grau) numa coordenada de tela 0..32767,
//! por TANGENTE (projecao "tela plana"). 'center' cai no meio (16384); em
//! (center +- halfRange) chega nas bordas (0 e 32767). 'sens' e' ganho Q8
//! (256 = 1.0x; 512 = 2.0x; 128 = 0.5x). Resultado sempre limitado a [0,32767].
uint16_t projectTan(int32_t angle_mdeg, int32_t center_mdeg,
                    int32_t halfRange_mdeg, uint16_t sens_q8)
{

    int32_t rel = angle_mdeg - center_mdeg;
    rel = truncateBetween(rel, -60000L, 60000L);              // evita div0/estouro do tan
    int32_t tanRel  = ((int32_t)sini(rel) << 12) / cosi(rel);          // tan em Q12
    int32_t tanHalf = ((int32_t)sini(halfRange_mdeg) << 12) / cosi(halfRange_mdeg);
    int32_t offset  = ((int32_t)tanRel * sens_q8 * 1536) / tanHalf;      // 16384 na borda (sens=256)
    int32_t coord   = 16384L + offset;
    return (uint16_t)truncateBetween(coord, 0L, 32767L);
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
  packet[10] = 0;
  packet[11] = 0;
  packet[12] = 0;
  packet[13] = 0;
  packet[14] = 0;
  packet[15] = 0;
}

void convert_recieved(int32_t *roll, int32_t *pitch, int32_t *yaw, int16_t *accel_x, BT_STATE *state, uint8_t *packet){
  if(packet[0] != 0x68) return;

  *state = (BT_STATE)packet[1];
  *pitch = ((uint32_t)packet[2] << 24) + ((uint32_t)packet[3] << 16) + ((uint32_t)packet[4] << 8) + (uint32_t)packet[5];
  *roll = ((uint32_t)packet[6] << 24) + ((uint32_t)packet[7] << 16) + ((uint32_t)packet[8] << 8) + (uint32_t)packet[9];
  *yaw = ((uint32_t)packet[10] << 24) + ((uint32_t)packet[11] << 16) + ((uint32_t)packet[12] << 8) + (uint32_t)packet[13];
  *accel_x = (packet[14] << 8) + packet[15];
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

void timer1OverflowCallback(void){
    //A cada 50ms
    timer1_overflows++;
}

uint32_t micros() {
    uint32_t overflows;
    uint16_t ticks;

    // Disable interrupts briefly to ensure atomic read of 16-bit register and 32-bit counter
    cli();
    overflows = timer1_overflows;
    ticks = timer1.getCounterValue();;
    
    // Check if an overflow just occurred but hasn't been handled yet
    if ((TIFR1 & (1 << TOV1)) && (ticks < 65535)) {
        overflows++;
        ticks = timer1.getCounterValue(); // Re-read to guarantee accurate timing
    }
    sei();

    // Each overflow is 65536 ticks. Each tick is 0.5 microseconds.
    // Total microseconds = (overflows * 65536 * 0.5) + (ticks * 0.5)
    return (overflows * 32768) + (ticks >> 1);
}

uint32_t get_millis() {
    uint32_t overflows;
    uint16_t ticks;

    cli();
    overflows = timer1_overflows;
    ticks = timer1.getCounterValue();
    if ((TIFR1 & (1 << TOV1)) && (ticks < 65535)) {
        overflows++;
        ticks = timer1.getCounterValue();
    }
    sei();

    // Em vez de calcular microssegundos, calculamos direto o milissegundo aproximado.
    // 32768 / 1024 = 32 (Exato!) -> overflows * 32
    // ticks / 2048 -> equivale a ticks >> 11
    return (overflows * 32) + (ticks >> 11);
}

void timer0CompareACallback(void){
    //A cada 10 ms
    static uint8_t counter_spi = 0;
    static uint8_t counter_display = 0;
    static uint8_t counter_clear_display = 0;

    if(counter_spi++ == 1){
        spi_flag = true;    
        counter_spi = 0;
    }

    if(counter_display++ == 10){
        update_display_flag = true;
        counter_display = 0;
    }

    if(counter_clear_display++ == 50){
        clear_display_flag = true;
        counter_clear_display = 0;
    }
}
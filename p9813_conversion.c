/*

    Copyright (C) 2024 Mauricio Bustos (m@bustos.org)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "clocked_input.pio.h"
#include "p9813.pio.h"


#define PIN_CLK 26
#define PIN_DIN 27

#define N_LEDS 50
#define SERIAL_FREQ (4000 * 1000)

uint8_t shift_left = 0, shift_right = 0, mask = 0xFF;
int frames = 0;
int zero_count = 0;
uint32_t next_word;

void put_start_frame(PIO pio, uint sm) {
    pio_sm_put(pio, sm, 0u);
}

void put_end_frame(PIO pio, uint sm) {
    pio_sm_put(pio, sm, 0u);
}

void put_rgb888(PIO pio, uint sm, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t flag;
    flag = (red&0xC0)>>6 | ((green&0xC0)>>4) | ((blue&0xC0)>>2);

    pio_sm_put_blocking(pio, sm,
                        ~flag << 24 |
                                (uint32_t) blue << 16 |
                                (uint32_t) green << 8 |
                                (uint32_t) red << 0
    );
}

void p9813_tx() {

    int p9813_zero_count = 0;
    int p9813_end_count = 0;
    uint32_t color_int = 0;

    puts("Start p9813");

    PIO p9813 = pio1;
    uint p9813_offset = pio_add_program(p9813, &p9813_mini_program);
    uint p9813_sm = pio_claim_unused_sm(p9813, true);

    p9813_mini_program_init(p9813, p9813_sm, p9813_offset, SERIAL_FREQ, PIN_CLK, PIN_DIN);

    puts("Output p9813 startup sequence");
    puts("\tRed");
    put_start_frame(p9813, p9813_sm);
    for (int i = 0; i < N_LEDS + 1; i++) {
        put_rgb888(p9813, p9813_sm, 0xFF, 0x00, 0x00);
    }
    put_end_frame(p9813, p9813_sm);
    sleep_ms(500);
    puts("\tGreen");
    put_start_frame(p9813, p9813_sm);
    for (int i = 0; i < N_LEDS + 1; i++) {
        put_rgb888(p9813, p9813_sm, 0x00, 0xFF, 0x00);
    }
    put_end_frame(p9813, p9813_sm);
    sleep_ms(500);
    puts("\tBlue");
    put_start_frame(p9813, p9813_sm);
    for (int i = 0; i < N_LEDS + 1; i++) {
        put_rgb888(p9813, p9813_sm, 0x00, 0x00, 0xFF);
    }
    put_end_frame(p9813, p9813_sm);
    sleep_ms(500);

    while (1) {
        uint8_t byte_value = multicore_fifo_pop_blocking();
        if (byte_value == 0) {
            p9813_zero_count++;
            if (p9813_zero_count == 4) {
                frames++;
                put_start_frame(p9813, p9813_sm);
                for (int i = 0; i < N_LEDS; i++) {
                    byte_value = multicore_fifo_pop_blocking();
                    color_int = 0xC0000000;
                    byte_value = multicore_fifo_pop_blocking();
                    color_int = color_int | ((uint32_t)byte_value << 8) | ((~(uint32_t)byte_value & 0xC0) << 20);
                    byte_value = multicore_fifo_pop_blocking();
                    color_int = color_int | (uint32_t)byte_value | ((~(uint32_t)byte_value & 0xC0) << 18);
                    byte_value = multicore_fifo_pop_blocking();
                    color_int = color_int | ((uint32_t)byte_value << 16) | ((~(uint32_t)byte_value & 0xC0) << 22);
                    pio_sm_put_blocking(p9813, p9813_sm, color_int);
                }
                for (int i = 0; i < 4 * N_LEDS; i++) {
                }
                put_end_frame(p9813, p9813_sm);
            }
        } else {
            p9813_zero_count = 0;
            if (byte_value == 0xFF) {
                p9813_end_count++;
                if (p9813_end_count == 4) {
                    p9813_end_count = 0;
                    continue;
                }
            } else {
                p9813_end_count = 0;
            }
        }
    }

}

void process_byte(uint8_t current_byte, uint8_t previous_byte) {
    if (current_byte == 0) {
        zero_count++;
        if (zero_count == 4) {frames++; mask = 0x00; shift_left = 0; shift_right = 0;}
        else if (zero_count == 3) {
            frames++;
            switch (mask) {
                case 0x00:
                    break;
                case 0x80:
                    shift_left = 1;
                    shift_right = 7;
                    break;
                case 0xC0:
                    shift_left = 2;
                    shift_right = 6;
                    break;
                case 0xE0:
                    mask = 0x00;
                    shift_left = 0;
                    shift_right = 0;
                    break;
                case 0xF0:
                    shift_left = 4;
                    shift_right = 4;
                    break;
                case 0xF8:
                    shift_left = 5;
                    shift_right = 3;
                    break;
                case 0xFC:
                    shift_left = 6;
                    shift_right = 2;
                    break;
                case 0xFE:
                    shift_left = 7;
                    shift_right = 1;
                    break;
                case 0xFF:
                    shift_left = 0;
                    shift_right = 0;
                    break;
                default:
                    shift_left = 0;
                    shift_right = 0;
            }
        }
    } else {
        mask = current_byte;
        zero_count = 0;
    }
    if (shift_left) multicore_fifo_push_blocking((previous_byte << shift_left) | ((current_byte & mask) >> shift_right));
    else multicore_fifo_push_blocking(previous_byte);
}

int main() {
    stdio_init_all();
    puts("Start");

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &clocked_input_program);
    uint sm = pio_claim_unused_sm(pio, true);

    clocked_input_program_init(pio, sm, offset, PICO_DEFAULT_SPI_SCK_PIN);

    puts("Start pixelblaze read");

    multicore_launch_core1(p9813_tx);

    sleep_ms(1800);

    uint8_t previous_byte = 0, current_byte = 0;

    while (1) {
        next_word = pio_sm_get_blocking(pio, sm);
        current_byte = (uint8_t) (next_word >> 24);
        process_byte(current_byte, previous_byte);
        previous_byte = current_byte;
        current_byte = (uint8_t) (next_word >> 16);
        process_byte(current_byte, previous_byte);
        previous_byte = current_byte;
        current_byte = (uint8_t) (next_word >> 8);
        process_byte(current_byte, previous_byte);
        previous_byte = current_byte;
        current_byte = (uint8_t) (next_word);
        process_byte(current_byte, previous_byte);
        previous_byte = current_byte;
        if (frames > 0 && frames % 500 == 0) {
            //printf("%d %d %d\n", frames, shift_left, shift_right);
        }
    }
}

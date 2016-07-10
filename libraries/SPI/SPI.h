/*
 * Copyright (c) 2016 Thomas Roell.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimers.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimers in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Thomas Roell, nor the names of its contributors
 *     may be used to endorse or promote products derived from this Software
 *     without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

#ifndef _SPI_H_INCLUDED
#define _SPI_H_INCLUDED

#include <Arduino.h>

// SPI_HAS_TRANSACTION means SPI has beginTransaction(), endTransaction(),
// usingInterrupt(), and SPISetting(clock, bitOrder, dataMode)
#define SPI_HAS_TRANSACTION 1

// SPI_HAS_NOTUSINGINTERRUPT means that SPI has notUsingInterrupt() method
#define SPI_HAS_NOTUSINGINTERRUPT 1

#define SPI_MODE0 0x00
#define SPI_MODE1 0x01
#define SPI_MODE2 0x02
#define SPI_MODE3 0x03

class SPISettings {
  public:
    SPISettings(uint32_t clock, BitOrder bitOrder, uint8_t dataMode) {
	if (__builtin_constant_p(clock)) {
	    init_AlwaysInline(clock, bitOrder, dataMode);
	} else {
	    init_MightInline(clock, bitOrder, dataMode);
	}
    }
  
    // Default speed set to 4MHz, SPI mode set to MODE 0 and Bit order set to MSB first.
    SPISettings() { init_AlwaysInline(4000000, MSBFIRST, SPI_MODE0); }
  
  private:
    void init_MightInline(uint32_t clock, BitOrder bitOrder, uint8_t dataMode) {
	init_AlwaysInline(clock, bitOrder, dataMode);
    }
  
    void init_AlwaysInline(uint32_t clock, BitOrder bitOrder, uint8_t dataMode) __attribute__((__always_inline__)) {
	uint32_t control = dataMode;

	if (bitOrder != MSBFIRST) { control |= SPI_CR1_LSBFIRST; }

#if (F_CPU <= 32000000)
	if      (clock >= (F_CPU / 2))   { control |= 0;                                            }
	else if (clock >= (F_CPU / 4))   { control |= (SPI_CR1_BR_0);                               }
	else if (clock >= (F_CPU / 8))   { control |= (SPI_CR1_BR_1);                               }
	else if (clock >= (F_CPU / 16))  { control |= (SPI_CR1_BR_0 | SPI_CR1_BR_1);                }
	else if (clock >= (F_CPU / 32))  { control |= (SPI_CR1_BR_2);                               }
	else if (clock >= (F_CPU / 64))  { control |= (SPI_CR1_BR_0 | SPI_CR1_BR_2);                }
	else if (clock >= (F_CPU / 128)) { control |= (SPI_CR1_BR_1 | SPI_CR1_BR_2);                }
	else                             { control |= (SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2); }
#else
	if      (clock >= (F_CPU / 4))   { control |= 0;                                            }
	else if (clock >= (F_CPU / 8))   { control |= (SPI_CR1_BR_0);                               }
	else if (clock >= (F_CPU / 16))  { control |= (SPI_CR1_BR_1);                               }
	else if (clock >= (F_CPU / 32))  { control |= (SPI_CR1_BR_0 | SPI_CR1_BR_1);                }
	else if (clock >= (F_CPU / 64))  { control |= (SPI_CR1_BR_2);                               }
	else if (clock >= (F_CPU / 128)) { control |= (SPI_CR1_BR_0 | SPI_CR1_BR_2);                }
	else if (clock >= (F_CPU / 256)) { control |= (SPI_CR1_BR_1 | SPI_CR1_BR_2);                }
	else                             { control |= (SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2); }
#endif

	this->_control = control;
    }

    uint32_t _control;

    friend class SPIClass;
};

class SPIClass {
  public:
    SPIClass(struct _stm32l4_spi_t *spi, unsigned int instance, const struct _stm32l4_spi_pins_t *pins, unsigned int priority, unsigned int mode);

    inline uint8_t transfer(uint8_t data) { return exchange8(data); }
    inline uint16_t transfer16(uint16_t data) { return exchange16(data); }
    inline void transfer(void *buffer, size_t count) { return exchange(static_cast<const uint8_t*>(buffer), static_cast<uint8_t*>(buffer), count); }

    // Transaction Functions
    void usingInterrupt(uint32_t pin);
    void notUsingInterrupt(uint32_t pin);
    void beginTransaction(SPISettings settings);
    void endTransaction(void);

    // SPI Configuration methods
    void attachInterrupt();
    void detachInterrupt();

    void begin();
    void end();

    void setBitOrder(BitOrder bitOrder);
    void setDataMode(uint8_t dataMode);
    void setClockDivider(uint8_t divider);

    // STM32L4 EXTENSTION: synchronous inline read/write
    inline uint8_t read(void) { return exchange8(0xff); }
    inline void read(void *buffer, size_t count) { return exchange(NULL, static_cast<uint8_t*>(buffer), count); }
    inline void write(uint8_t data) { return (void)exchange8(data); }
    inline void write(const void *buffer, size_t count) { return exchange(static_cast<const uint8_t*>(buffer), NULL, count); }

    // STM32L4 EXTENSTION: synchronous composite transaction
    void transfer(SPISettings settings, const void *txBuffer, void *rxBuffer, size_t count, bool halfDuplex = false);

    // STM32L4 EXTENSTION: asynchronous composite transaction
    bool transfer(SPISettings settings, const void *txBuffer, void *rxBuffer, size_t count, void(*callback)(void), bool halfDuplex = false);
    void flush(void);
    bool done(void);

  private:
    struct _stm32l4_spi_t *_spi;
    bool _selected;
    uint32_t _clock;
    BitOrder _bitOrder;
    uint8_t _dataMode;
    uint32_t _interruptMask;

    void (*_exchangeRoutine)(struct _stm32l4_spi_t*, const uint8_t*, uint8_t*, size_t);
    uint8_t (*_exchange8Routine)(struct _stm32l4_spi_t*, uint8_t);
    uint16_t (*_exchange16Routine)(struct _stm32l4_spi_t*, uint16_t);
    
    void exchange(const uint8_t *txData, uint8_t *rxData, size_t count)  __attribute__((__always_inline__)) {
	return (*_exchangeRoutine)(_spi, txData, rxData, count);
    }

    uint8_t exchange8(uint8_t data)  __attribute__((__always_inline__)) {
	return (*_exchange8Routine)(_spi, data);
    }

    uint16_t exchange16(uint16_t data) __attribute__((__always_inline__)) {
	return (*_exchange16Routine)(_spi, data);
    }

    static void _exchangeSelect(struct _stm32l4_spi_t *spi, const uint8_t *txData, uint8_t *rxData, size_t count);
    static uint8_t _exchange8Select(struct _stm32l4_spi_t *spi, uint8_t data);
    static uint16_t _exchange16Select(struct _stm32l4_spi_t *spi, uint16_t data);

    void (*_completionCallback)(void);

    static void _eventCallback(void *context, uint32_t events);
    void EventCallback(uint32_t events);
};

#if SPI_INTERFACES_COUNT > 0
  extern SPIClass SPI;
#endif
#if SPI_INTERFACES_COUNT > 1
  extern SPIClass SPI1;
#endif
#if SPI_INTERFACES_COUNT > 2
  extern SPIClass SPI2;
#endif

// For compatibility with sketches designed for AVR @ 16 MHz
// New programs should use SPI.beginTransaction to set the SPI clock
#define SPI_CLOCK_DIV2   (F_CPU / 8000000)
#define SPI_CLOCK_DIV4   (F_CPU / 4000000)
#define SPI_CLOCK_DIV8   (F_CPU / 2000000)
#define SPI_CLOCK_DIV16  (F_CPU / 1000000)
#define SPI_CLOCK_DIV32  (F_CPU / 500000)
#define SPI_CLOCK_DIV64  (F_CPU / 250000)
#define SPI_CLOCK_DIV128 (F_CPU / 125000)

#endif
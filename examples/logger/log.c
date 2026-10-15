/*
 * Copyright (c) 2020 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "log.h"

#include <string.h>

#include "bdk_gpio.h"
#include "bdk_status.h"
#include "bdk_uart.h"
#include "stm32f4xx.h"

#define MAX_CALLBACKS 32

static uint32_t uart_log_irq_primask;

static void uart_log_irq_lock(bool lock, void *udata)
{
  (void)udata;
  if (lock) {
    uart_log_irq_primask = __get_PRIMASK();
    __disable_irq();
  } else {
    __set_PRIMASK(uart_log_irq_primask);
  }
}

typedef struct {
  log_LogFn fn;
  void *udata;
  int level;
} Callback;

static struct {
  void *udata;
  log_LockFn lock;
  int level;
  bool quiet;
  Callback callbacks[MAX_CALLBACKS];
} L;

static bdk_uart_id_t log_uart_id = BDK_UART_2;
static bool log_uart_sink;


static const char *level_strings[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif


static void stdout_callback(log_Event *ev) {
  char buf[16];
  buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
#ifdef LOG_USE_COLOR
  fprintf(
    ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
    buf, level_colors[ev->level], level_strings[ev->level],
    ev->file, ev->line);
#else
  fprintf(
    ev->udata, "%s %-5s %s:%d: ",
    buf, level_strings[ev->level], ev->file, ev->line);
#endif
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}


static void uart_sink_callback(log_Event *ev)
{
  char tbuf[16];
  char hdr[160];
  char body[256];
  char line[420];
  int hlen;
  int blen;

  tbuf[0] = '\0';
  if (ev->time) {
    tbuf[strftime(tbuf, sizeof(tbuf), "%H:%M:%S", ev->time)] = '\0';
  }

  hlen = snprintf(
      hdr, sizeof(hdr), "%s %-5s ",
      tbuf, level_strings[ev->level]);
  if (hlen < 0) {
    return;
  }

  blen = vsnprintf(body, sizeof(body), ev->fmt, ev->ap);
  if (blen < 0) {
    return;
  }

  hlen = snprintf(line, sizeof(line), "%s%s\r\n", hdr, body);
  if (hlen <= 0) {
    return;
  }

  (void)bdk_uart_write(log_uart_id, (const uint8_t *)line, (size_t)hlen);
}


static void file_callback(log_Event *ev) {
  char buf[64];
  buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
  fprintf(
    ev->udata, "%s %-5s %s:%d: ",
    buf, level_strings[ev->level], ev->file, ev->line);
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}


static void lock(void)   {
  if (L.lock) { L.lock(true, L.udata); }
}


static void unlock(void) {
  if (L.lock) { L.lock(false, L.udata); }
}


const char* log_level_string(int level) {
  return level_strings[level];
}


void log_set_lock(log_LockFn fn, void *udata) {
  L.lock = fn;
  L.udata = udata;
}


void log_set_level(int level) {
  L.level = level;
}


void log_set_quiet(bool enable) {
  L.quiet = enable;
}


int log_add_callback(log_LogFn fn, void *udata, int level) {
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (!L.callbacks[i].fn) {
      L.callbacks[i] = (Callback) { fn, udata, level };
      return 0;
    }
  }
  return -1;
}


int log_add_fp(FILE *fp, int level) {
  return log_add_callback(file_callback, fp, level);
}


static void init_event(log_Event *ev, void *udata) {
  if (!ev->time) {
    time_t t = time(NULL);
    ev->time = localtime(&t);
  }
  ev->udata = udata;
}


void log_redirect_uart2(uint32_t baud)
{
  bdk_gpio_config_t tx = {
      .port  = BDK_GPIO_PORT_A,
      .pin   = 2,
      .mode  = BDK_GPIO_MODE_AF,
      .af    = BDK_GPIO_AF_USART2,
      .otype = BDK_GPIO_OTYPE_PP,
      .pull  = BDK_GPIO_PULL_NONE,
      .speed = BDK_GPIO_SPEED_HIGH,
  };
  bdk_gpio_config_t rx = {
      .port  = BDK_GPIO_PORT_A,
      .pin   = 3,
      .mode  = BDK_GPIO_MODE_AF,
      .af    = BDK_GPIO_AF_USART2,
      .otype = BDK_GPIO_OTYPE_PP,
      .pull  = BDK_GPIO_PULL_NONE,
      .speed = BDK_GPIO_SPEED_HIGH,
  };
  bdk_uart_config_t uart_cfg = {
      .id     = BDK_UART_2,
      .baud   = baud,
      .parity = BDK_UART_PARITY_NONE,
      .stop   = BDK_UART_STOP_1,
      .word   = BDK_UART_WORD_8,
  };

  if (log_uart_sink) {
    return;
  }

  if (bdk_gpio_init(&tx) != BDK_OK || bdk_gpio_init(&rx) != BDK_OK ||
      bdk_uart_init(&uart_cfg) != BDK_OK) {
    return;
  }

  log_uart_id = BDK_UART_2;
  log_set_quiet(true);
  log_set_lock(uart_log_irq_lock, NULL);
  if (log_add_callback(uart_sink_callback, NULL, LOG_TRACE) != 0) {
    return;
  }
  log_uart_sink = true;
}


void log_log(int level, const char *file, int line, const char *fmt, ...) {
  log_Event ev = {
    .fmt   = fmt,
    .file  = file,
    .line  = line,
    .level = level,
  };

  lock();

  if (!L.quiet && level >= L.level) {
    init_event(&ev, stderr);
    va_start(ev.ap, fmt);
    stdout_callback(&ev);
    va_end(ev.ap);
  }

  for (int i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
    Callback *cb = &L.callbacks[i];
    if (level >= cb->level) {
      init_event(&ev, cb->udata);
      va_start(ev.ap, fmt);
      cb->fn(&ev);
      va_end(ev.ap);
    }
  }

  unlock();
}

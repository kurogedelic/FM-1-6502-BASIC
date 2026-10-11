/* SPDX-License-Identifier: MPL-2.0
 * FM-1 6502 BASIC firmware for the M-VAVE FM-1.
 *
 * Structure (start-up order, boot guard, LCD init sequence, timer/key scan
 * wiring) follows felucca-bsp's examples/hwtest/hwtest.c (MPL-2.0, Leo
 * Kuroshita / Hugelton Instruments). It does NOT include chip/usb.c (GPL-3.0),
 * so this image contains no GPL-licensed code from felucca-bsp.
 *
 * What it does: runs the host-tested 6502 core (src/cpu6502.c) with the
 * Microsoft BASIC ROM (build/basic.bin, embedded) in a 64 KiB virtual
 * machine. Characters written to $D000 go to the 40x24 Terminal model, which
 * is drawn on the 240x240 LCD. FM-1 keys are converted to characters with
 * src/keyboard.c and queued on $D000/$D001.
 *
 * STATUS: compiles and links with the JieLi toolchain. NOT yet run on a
 * device. Read docs/FM1_PORT.md before flashing anything.
 *
 * Recovery: two boots in a row that do not reach the healthy point (2 s)
 * enter UBOOT, and holding OCT- + OCT+ for 2 s enters UBOOT from the running
 * firmware. Both use felucca-bsp's fm1_enter_uboot(); neither touches the boot
 * or recovery flash regions. */
#include <stdint.h>
#include <stddef.h>
#include "fm1_time.h"
#include "fm1_sys.h"
#include "fm1_irq.h"
#include "fm1_guard.h"
#include "fm1_timer.h"
#include "fm1_input.h"
#include "fm1_lcd_hw.h"
#include "libc.c"

#include "../src/cpu6502.h"
#include "../src/keyboard.h"
#include "../src/lcdtext.h"
#include "../src/terminal.h"

/* felucca-bsp's libc.c has no memmove; terminal.c scrolls with it. */
void *memmove(void *d, const void *s, unsigned n)
{
    unsigned char *p = d;
    const unsigned char *q = s;
    if (p < q)
        while (n--)
            *p++ = *q++;
    else {
        p += n;
        q += n;
        while (n--)
            *--p = *--q;
    }
    return d;
}

extern const unsigned char basic_rom[0x8000];   /* build/basic.bin: $8000..$FFFF */
extern uint32_t _data_start[], _data_end[], _data_load[], _bss_start[], _bss_end[];

static volatile uint32_t ms;
static struct { uint32_t magic, pending, failed; } guard __attribute__((section(".noinit")));
void fm1_alnk0_irq(void) {}                     /* isr_alnk0 is linked but no audio runs */

void fm1_timer5_irq(void)                       /* 10 kHz: key scan and the ms clock */
{
    static uint32_t last, acc;
    uint32_t t = fm1_ticks();
    fm1_timer5_ack();
    fm1_input_tick();
    acc += t - last;
    last = t;
    while (acc >= 1000u * FM1_TICKS_PER_US) {
        acc -= 1000u * FM1_TICKS_PER_US;
        ms++;
    }
}
extern void isr_timer5(void);

/* ---- LCD: ST7789 240x240, RGB565 sent high byte first (colours pre-swapped) ---- */
#define C(x) ((uint16_t)((((x) >> 8) | ((x) << 8)) & 0xFFFFu))
#define FG C(0x07E0)                            /* terminal text: green */
#define BG C(0x0000)
#define DIMFG C(0x4208)                         /* status line */
#define RED_ C(0xF800)
static uint16_t rowbuf[LCD_CELL_H * LCD_W];     /* one text row of pixels */

static void lcd_cmd(uint8_t c, const uint8_t *d, uint32_t n)
{
    static uint8_t buf[4];
    uint32_t i;
    fm1_lcd_send_cmd(c);
    fm1_lcd_wait();
    fm1_lcd_deselect();
    if (!n)
        return;
    for (i = 0; i < n; i++)
        buf[i] = d[i];
    fm1_lcd_send_data(buf, n);
    fm1_lcd_wait();
    fm1_lcd_deselect();
}

static void lcd_init(void)
{
    static const uint8_t colmod = 0x55, madctl = 0x00;
    fm1_lcd_hw_init();
    fm1_delay_ms(50);
    lcd_cmd(0x01, 0, 0);                        /* SWRESET */
    fm1_delay_ms(150);
    lcd_cmd(0x11, 0, 0);                        /* SLPOUT */
    fm1_delay_ms(120);
    lcd_cmd(0x3A, &colmod, 1);                  /* RGB565 */
    lcd_cmd(0x36, &madctl, 1);                  /* top-left origin, RGB */
    lcd_cmd(0x21, 0, 0);                        /* INVON: this panel is IPS */
    lcd_cmd(0x13, 0, 0);                        /* NORON */
    fm1_lcd_baud(4);
}

static void lcd_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    uint8_t a[4];
    a[0] = 0; a[1] = (uint8_t)x; a[2] = 0; a[3] = (uint8_t)(x + w - 1u);
    lcd_cmd(0x2A, a, 4);
    a[1] = (uint8_t)y; a[3] = (uint8_t)(y + h - 1u);
    lcd_cmd(0x2B, a, 4);
    lcd_cmd(0x2C, 0, 0);
}

static void lcd_fill(uint16_t px)
{
    uint32_t i;
    for (i = 0; i < LCD_W; i++)
        rowbuf[i] = px;
    lcd_window(0, 0, 240, 240);
    for (i = 0; i < 240; i++) {
        fm1_lcd_send_data(rowbuf, LCD_W * 2u);
        fm1_lcd_wait();
        fm1_lcd_deselect();
    }
}

/* draw one 40-character text row at text row `r` (0..29) */
static void lcd_text_row(uint32_t r, const char *cells, int cursor_col, uint16_t fg)
{
    uint32_t i;
    lcdtext_row(cells, cursor_col, fg, BG, rowbuf);
    lcd_window(0, r * LCD_CELL_H, LCD_W, LCD_CELL_H);
    for (i = 0; i < LCD_CELL_H; i++) {
        fm1_lcd_send_data(rowbuf + i * LCD_W, LCD_W * 2u);
        fm1_lcd_wait();
        fm1_lcd_deselect();
    }
}

/* a fault: red screen, then reset; the record in fm1_crash survives to the next boot */
static void fm1_fault(const fm1_crash_t *c)
{
    (void)c;
    lcd_fill(RED_);
    fm1_delay_ms(1500);
    fm1_reboot();
}

/* ---- virtual machine ---- */
static BasicVM vm;
static Cpu6502 cpu;
static Terminal term;
static Keyboard kb;

static void vm_out(void *ctx, uint8_t ch) { term_put((Terminal *)ctx, (char)ch); }

static void machine_init(void)
{
    uint32_t i;
    term_init(&term);
    vm_init(&vm, vm_out, &term);
    for (i = 0; i < sizeof basic_rom; i++)
        vm.mem[0x8000u + i] = basic_rom[i];
    keyboard_init(&kb);
    cpu_reset(&cpu, &vm);
}

/* ---- keys ---- */
/* matrix button ids (felucca-bsp: 0 = OCT-, 1 = OCT+; 2..13 = FX SCL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC,
 * the printed-label order is a best guess for some buttons) -> keyboard_panel() index */
static int panel_index(uint32_t id)
{
    if (id == 0u) return 12;
    if (id == 1u) return 13;
    return (int)id - 2;
}

static void type_char(int c)
{
    if (c < 0 || (c >= 0x11 && c <= 0x14))      /* modifiers; cursor keys have no meaning in BASIC's line input */
        return;
    (void)vm_input(&vm, (uint8_t)c);            /* a full queue drops the key */
}

static void keys_poll(void)
{
    uint32_t n = fm1_input_note_edges(), b = fm1_input_edges(0), i;
    for (i = 0; i < 27u; i++)
        if ((n >> i) & 1u)
            type_char(keyboard_note(&kb, i));
    for (i = 0; i < 14u; i++)
        if ((b >> i) & 1u)
            type_char(keyboard_panel(&kb, (unsigned)panel_index(i)));
    /* LEDs: pressed keys, and the SHIFT / CTRL / symbol-layer state on SCL, GLO, SAVE */
    for (i = 0; i < 14u; i++)
        fm1_led_key(i, (fm1_in.buttons >> i) & 1u);
    for (i = 0; i < 27u; i++)
        fm1_led_key(14u + i, (fm1_in.notes >> i) & 1u);
    fm1_led_key(3u, kb.shift != 0u);            /* SCL */
    fm1_led_key(7u, kb.ctrl != 0u);             /* GLO */
    fm1_led_key(9u, kb.layer == KEY_SYMBOLS);   /* SAVE */
}

/* ---- screen update: only rows whose text or cursor changed ---- */
static char shadow[TERM_ROWS][TERM_COLS];
static int8_t shadow_cursor[TERM_ROWS];
static char status_shadow[TERM_COLS];
static int status_ok;

static void status_text(char *s)
{
    static const char t[] = "FM-1 BASIC   ABC  SHIFT  CTRL   OCT-:BREAK";
    uint32_t i;
    for (i = 0; i < TERM_COLS; i++)
        s[i] = i < sizeof t - 1u ? t[i] : ' ';
    if (kb.layer == KEY_SYMBOLS) {
        s[13] = '1'; s[14] = '2'; s[15] = '3';
    }
    if (!kb.shift) for (i = 18; i < 23; i++) s[i] = ' ';
    if (!kb.ctrl) for (i = 25; i < 29; i++) s[i] = ' ';
}

static void screen_update(uint32_t now)
{
    uint32_t r, budget = 4;
    int blink = (now >> 9) & 1u;
    uint32_t crow = term.y;
    int ccol = term.x < TERM_COLS ? term.x : TERM_COLS - 1;
    char st[TERM_COLS];
    for (r = 0; r < TERM_ROWS && budget; r++) {
        int want = (blink && r == crow) ? ccol : -1;
        if (memcmp(shadow[r], term.cells[r], TERM_COLS) || shadow_cursor[r] != want) {
            lcd_text_row(r, term.cells[r], want, FG);
            memcpy(shadow[r], term.cells[r], TERM_COLS);
            shadow_cursor[r] = (int8_t)want;
            budget--;
        }
    }
    status_text(st);
    if (budget && (!status_ok || memcmp(st, status_shadow, TERM_COLS))) {
        lcd_text_row(LCD_TEXT_ROWS - 1, st, -1, DIMFG);
        memcpy(status_shadow, st, TERM_COLS);
        status_ok = 1;
    }
}

static void app_main(void)
{
    uint32_t i, boots_ok = 0, chord = 0;
    lcd_init();
    lcd_fill(BG);
    machine_init();
    for (i = 0; i < TERM_ROWS; i++)
        shadow_cursor[i] = -2;                  /* force the first full draw */
    fm1_input_init();
    fm1_timer5_start(isr_timer5, 4);
    fm1_guard_lock_top();
    fm1_irq_enable_all();
    fm1_lcd_backlight(1);
    for (;;) {
        uint32_t now = ms, n;
        fm1_wdt_feed();
        if (!boots_ok && now > 2000u) {         /* it runs: the next boot is not a loop */
            boots_ok = 1;
            guard.pending = 0;
            guard.failed = 0;
        }
        if ((fm1_in.buttons & 3u) == 3u) {      /* OCT- + OCT+ held 2 s: UBOOT, so a new image can be installed */
            if (!chord)
                chord = now ? now : 1u;
            else if (now - chord > 2000u)
                fm1_enter_uboot();
        } else
            chord = 0;
        keys_poll();
        for (n = 0; n < 4000u; n++) {           /* about a millisecond of 6502 time per slice, then service the UI */
            if (cpu_step(&cpu) != 0)
                break;
        }
        if (cpu.stopped) {                      /* unsupported opcode: restart BASIC */
            machine_init();
            for (i = 0; i < TERM_ROWS; i++)
                shadow_cursor[i] = -2;
        }
        screen_update(now);
    }
}

void fm1_cstart(void)
{
    uint32_t *s, *d;
    fm1_time_init();
    fm1_wdt_arm(0x0D);
    if (guard.magic != 0x42415331u) {           /* "BAS1" */
        guard.magic = 0x42415331u;
        guard.failed = 0;
        guard.pending = 0;
    }
    if (guard.pending)
        guard.failed++;
    guard.pending = 1;
    if (guard.failed >= 2u) {
        guard.failed = 0;
        guard.pending = 0;
        fm1_enter_uboot();
    }
    fm1_irq_init();
    for (d = _bss_start; d < _bss_end; d++)
        *d = 0;
    for (s = _data_load, d = _data_start; d < _data_end; s++, d++)
        *d = *s;
    fm1_guard_enable(FM1_GUARD_STACK | FM1_GUARD_WRITE | FM1_GUARD_BUS | FM1_GUARD_PC);
    app_main();
    for (;;)
        ;
}

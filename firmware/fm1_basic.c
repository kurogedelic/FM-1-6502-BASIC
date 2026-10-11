/* SPDX-License-Identifier: MPL-2.0
 * FM-1 6502 BASIC firmware for the M-VAVE FM-1.
 *
 * Structure (start-up order, boot guard, LCD init sequence, timer/key scan
 * wiring) follows felucca-bsp's examples/hwtest/hwtest.c (MPL-2.0, Leo
 * Kuroshita / Hugelton Instruments). The default image does not include
 * chip/usb.c; FM1_USB=1 builds include that GPL-3.0-only code.
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
#ifndef FM1_USB
#define FM1_USB 0     /* 1: development image with felucca-bsp's chip/usb.c (GPL-3.0-only): USB serial + SysEx "UBOOT" request */
#endif
#if FM1_USB
#define FELUCCA_CDC 1
#define FELUCCA_OTA 0
#define FELUCCA_ID "fm1-6502-basic"
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#endif
#include "fm1_time.h"
#include "fm1_sys.h"
#include "fm1_irq.h"
#include "fm1_guard.h"
#include "fm1_timer.h"
#include "fm1_input.h"
#include "fm1_lcd_hw.h"
#include "libc.c"
#if FM1_USB
#include "usb.c"
#endif

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
static volatile uint32_t timer5_calls;
static struct { uint32_t magic, pending, failed; } guard __attribute__((section(".noinit")));
#if FM1_DIAG
/* crt0 saves its boot probe at 0x01C07F20. The adjacent debug word survives
 * the ROM UBOOT fallback, unlike .noinit near the top of RAM. */
#define BOOT_STAGE(n) (*(volatile uint32_t *)0x01C07F40u = 0xB6500000u | (n))
#else
#define BOOT_STAGE(n) ((void)0)
#endif
void fm1_alnk0_irq(void) {}                     /* isr_alnk0 is linked but no audio runs */

void fm1_timer5_irq(void)                       /* 10 kHz: key scan and the ms clock */
{
    static uint32_t last, acc;
    static uint8_t primed;
#if FM1_USB
    static uint32_t sub;
#endif
    uint32_t t = fm1_ticks();
    fm1_timer5_ack();
    timer5_calls++;
    fm1_input_tick();
    if (!primed) {
        /* TIMER4 may already be running after a soft power cycle. Its first
         * count is not elapsed application time. */
        primed = 1;
        last = t;
    } else {
        acc += t - last;
        last = t;
        while (acc >= 1000u * FM1_TICKS_PER_US) {
            acc -= 1000u * FM1_TICKS_PER_US;
            ms++;
        }
    }
#if FM1_USB
    if (++sub == 5u) {                          /* USB at 2 kHz, as in hwtest */
        sub = 0;
        usb_poll();
    }
#endif
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
    lcd_cmd(0x29, 0, 0);                        /* DISPON: leave the panel's display-off state */
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

#if FM1_DIAG
static volatile uint32_t out_count_fwd;
#endif
static void vm_out(void *ctx, uint8_t ch)
{
#if FM1_DIAG
    out_count_fwd++;
#endif
    term_put((Terminal *)ctx, (char)ch);
}

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


#ifndef FM1_DIAG
#define FM1_DIAG 0
#endif
#if FM1_DIAG
/* Diagnostic build (firmware/build.sh DIAG=1): shows how far start-up got and the 6502 state as text. */
static void dline(uint32_t row, const char *label, uint32_t v1, int d1, const char *l2, uint32_t v2, int d2)
{
    char t[TERM_COLS];
    uint32_t i = 0, k;
    for (k = 0; k < TERM_COLS; k++) t[k] = ' ';
    while (*label && i < TERM_COLS) t[i++] = *label++;
    for (k = d1; k > 0 && i < TERM_COLS; k--) t[i++] = "0123456789ABCDEF"[(v1 >> (4u * (k - 1u))) & 15u];
    if (l2) {
        i++;
        while (*l2 && i < TERM_COLS) t[i++] = *l2++;
        for (k = d2; k > 0 && i < TERM_COLS; k--) t[i++] = "0123456789ABCDEF"[(v2 >> (4u * (k - 1u))) & 15u];
    }
    lcd_text_row(row, t, -1, C(0xFFFF));
}
static void dtext(uint32_t row, const char *s)
{
    char t[TERM_COLS];
    uint32_t k;
    for (k = 0; k < TERM_COLS; k++) t[k] = *s ? *s++ : ' ';
    lcd_text_row(row, t, -1, C(0xFFE0));
}
#endif

#if FM1_USB
#define USB_SERVICE(now) do { usb_retry(now); if (usb.uboot_req) { fm1_delay_ms(30); usb_detach(); fm1_enter_uboot(); } } while (0)
#else
#define USB_SERVICE(now) ((void)(now))
#endif

static void app_main(void)
{
    uint32_t i, boots_ok = 0, chord = 0, loop_count = 0;
    BOOT_STAGE(8);
    lcd_init();
    BOOT_STAGE(9);
#if FM1_DIAG
    fm1_lcd_backlight(1);                      /* make the first LCD checkpoint visible before VM and IRQ setup */
    lcd_fill(C(0x001F));                        /* blue: proves the LCD path works even if nothing else does */
    fm1_delay_ms(300);
#endif
    lcd_fill(BG);
#if FM1_DIAG
    dtext(0, "FM-1 BASIC DIAG BUILD");
    dtext(1, "1 LCD INIT OK");
#endif
    machine_init();
    BOOT_STAGE(10);
#if FM1_DIAG
    dtext(2, "2 VM INIT OK");
    dline(3, "  RESET PC=", cpu.pc, 4, "SP=", cpu.sp, 2);
    dline(4, "  ROM $8000=", vm.mem[0x8000], 2, "$FFFC=", (uint32_t)vm.mem[0xFFFC] | ((uint32_t)vm.mem[0xFFFD] << 8), 4);
#endif
    for (i = 0; i < TERM_ROWS; i++)
        shadow_cursor[i] = -2;                  /* force the first full draw */
    fm1_input_init();
#if FM1_USB
    usb_start();
#endif
    fm1_timer5_start(isr_timer5, 4);
    fm1_guard_lock_top();
    fm1_irq_enable_all();
    fm1_lcd_backlight(1);
    BOOT_STAGE(11);
#if FM1_DIAG
    dtext(5, "3 IRQ/TIMER OK, ENTERING LOOP");
    for (;;) {
        static uint32_t last_diag, steps, restarts;
        uint32_t now = ms, n;
        fm1_wdt_feed();
        USB_SERVICE(now);
        if (!boots_ok && (++loop_count >= 100u || now > 2000u)) {
            boots_ok = 1;
            guard.pending = 0;
            guard.failed = 0;
            BOOT_STAGE(12);
        }
        if ((fm1_in.buttons & 3u) == 3u) {
            if (!chord)
                chord = now ? now : 1u;
            else if (now - chord > 2000u)
                fm1_enter_uboot();
        } else
            chord = 0;
        keys_poll();
        for (n = 0; n < 4000u; n++) {
            if (cpu_step(&cpu) != 0)
                break;
            steps++;
        }
        if (cpu.stopped) {
            restarts++;
            machine_init();
        }
        if (now - last_diag >= 250u) {
            uint32_t r;
            last_diag = now;
            dline(6, "PC=", cpu.pc, 4, "A=", cpu.a, 2);
            dline(7, "X=", cpu.x, 2, "Y=", cpu.y, 2);
            dline(8, "STEPS=", steps, 8, "OUT=", out_count_fwd, 4);
            dline(9, "RESTARTS=", restarts, 4, "MS=", now, 8);
            dline(10, "BTN=", fm1_in.buttons, 4, "KEYS=", fm1_in.notes, 8);
            for (r = 0; r < 12u; r++)
                lcd_text_row(12u + r, term.cells[r], -1, FG);
        }
    }
#endif
    for (;;) {
        uint32_t now = ms, n;
        fm1_wdt_feed();
        USB_SERVICE(now);
        if (!boots_ok && (++loop_count >= 100u || now > 2000u)) { /* main loop reached: clear stale boot guard */
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
    BOOT_STAGE(1);
    fm1_time_init();
    BOOT_STAGE(2);
    fm1_wdt_arm(0x0D);
    BOOT_STAGE(3);
    if (guard.magic != 0x42415332u) {           /* "BAS2": reset old failed-boot counts after the timer fix */
        guard.magic = 0x42415332u;
        guard.failed = 0;
        guard.pending = 0;
    }
    if (guard.pending)
        guard.failed++;
    guard.pending = 1;
    BOOT_STAGE(4);
    /* Do not force UBOOT from a retained .noinit failure count. A power-cycle
     * can preserve this state even when the app is perfectly bootable. Keep
     * manual OCT-/OCT+ UBOOT recovery and the external Transporter instead. */
    if (guard.failed >= 2u) {
        BOOT_STAGE(0xE);
        guard.failed = 0;
        guard.pending = 0;
    }
    fm1_irq_init();
    BOOT_STAGE(5);
    for (d = _bss_start; d < _bss_end; d++)
        *d = 0;
    for (s = _data_load, d = _data_start; d < _data_end; s++, d++)
        *d = *s;
    BOOT_STAGE(6);
    fm1_guard_enable(FM1_GUARD_STACK | FM1_GUARD_WRITE | FM1_GUARD_BUS | FM1_GUARD_PC);
    BOOT_STAGE(7);
    app_main();
    for (;;)
        ;
}

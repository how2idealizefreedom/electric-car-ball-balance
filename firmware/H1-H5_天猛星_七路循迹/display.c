#include "display.h"
#include "car_config.h"
#include "ti_msp_dl_config.h"

#include <stddef.h>

#define OLED_WIDTH                 (128U)
#define OLED_PAGES                 (8U)
#define OLED_ADDRESS_0             (0x3CU)
#define OLED_ADDRESS_1             (0x3DU)
#define OLED_TIMEOUT_LOOPS         (200000U)
#define OLED_CONTROL_COMMAND       (0x00U)
#define OLED_CONTROL_DATA          (0x40U)

static uint8_t g_framebuffer[OLED_WIDTH * OLED_PAGES];
static uint8_t g_txBuffer[OLED_WIDTH + 1U];
static uint8_t g_dirtyPages;
static uint8_t g_oledAddress;
static bool g_ready;
static uint32_t g_lastTenths = UINT32_MAX;
static char g_lastStatus[17];

static bool oled_wait_idle(void)
{
    uint32_t timeout = OLED_TIMEOUT_LOOPS;

    while ((DL_I2C_getControllerStatus(OLED_I2C_INST) &
            DL_I2C_CONTROLLER_STATUS_IDLE) == 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }
    return true;
}

static void oled_recover_transfer(void)
{
    DL_I2C_resetControllerTransfer(OLED_I2C_INST);
    DL_I2C_flushControllerTXFIFO(OLED_I2C_INST);
    DL_I2C_clearInterruptStatus(OLED_I2C_INST, UINT32_MAX);
}

static bool oled_i2c_write(const uint8_t *data, uint16_t length)
{
    uint16_t written;
    uint32_t timeout = OLED_TIMEOUT_LOOPS;

    if ((data == NULL) || (length == 0U) || !oled_wait_idle()) {
        return false;
    }
    DL_I2C_clearInterruptStatus(OLED_I2C_INST, UINT32_MAX);
    DL_I2C_flushControllerTXFIFO(OLED_I2C_INST);
    written = DL_I2C_fillControllerTXFIFO(
        OLED_I2C_INST, data, length);
    DL_I2C_startControllerTransfer(OLED_I2C_INST, g_oledAddress,
        DL_I2C_CONTROLLER_DIRECTION_TX, length);

    /* I2C_ERR_13 workaround: allow at least three functional clock cycles. */
    delay_cycles(32U);
    while (written < length) {
        uint32_t status = DL_I2C_getControllerStatus(OLED_I2C_INST);
        if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
            oled_recover_transfer();
            return false;
        }
        if (!DL_I2C_isControllerTXFIFOFull(OLED_I2C_INST)) {
            DL_I2C_transmitControllerData(OLED_I2C_INST, data[written++]);
        }
        if (--timeout == 0U) {
            oled_recover_transfer();
            return false;
        }
    }
    timeout = OLED_TIMEOUT_LOOPS;
    while ((DL_I2C_getControllerStatus(OLED_I2C_INST) &
            DL_I2C_CONTROLLER_STATUS_BUSY) != 0U) {
        if (--timeout == 0U) {
            oled_recover_transfer();
            return false;
        }
    }
    if ((DL_I2C_getControllerStatus(OLED_I2C_INST) &
            DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
        oled_recover_transfer();
        return false;
    }
    return oled_wait_idle();
}

static bool oled_write_commands(const uint8_t *commands, uint8_t count)
{
    uint8_t index;

    g_txBuffer[0] = OLED_CONTROL_COMMAND;
    for (index = 0U; index < count; ++index) {
        g_txBuffer[index + 1U] = commands[index];
    }
    return oled_i2c_write(g_txBuffer, (uint16_t) count + 1U);
}

static bool oled_write_page(uint8_t page)
{
    uint8_t commands[3];
    uint16_t index;

    commands[0] = (uint8_t) (0xB0U | page);
    commands[1] = 0x00U;
    commands[2] = 0x10U;
    if (!oled_write_commands(commands, 3U)) {
        return false;
    }
    g_txBuffer[0] = OLED_CONTROL_DATA;
    for (index = 0U; index < OLED_WIDTH; ++index) {
        g_txBuffer[index + 1U] =
            g_framebuffer[(uint16_t) page * OLED_WIDTH + index];
    }
    return oled_i2c_write(g_txBuffer, OLED_WIDTH + 1U);
}

static void glyph(char character, uint8_t output[5])
{
    static const uint8_t digits[10][5] = {
        {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
        {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
        {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}
    };
    static const uint8_t letters[26][5] = {
        {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
        {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
        {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
        {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
        {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
        {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
        {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
        {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
        {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
        {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
        {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
        {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
        {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}
    };
    uint8_t index;

    for (index = 0U; index < 5U; ++index) {
        output[index] = 0U;
    }
    if ((character >= '0') && (character <= '9')) {
        for (index = 0U; index < 5U; ++index) {
            output[index] = digits[(uint8_t) character - (uint8_t) '0'][index];
        }
    } else if ((character >= 'A') && (character <= 'Z')) {
        for (index = 0U; index < 5U; ++index) {
            output[index] = letters[(uint8_t) character - (uint8_t) 'A'][index];
        }
    } else if (character == '-') {
        output[0] = output[1] = output[2] = output[3] = output[4] = 0x08U;
    } else if (character == '_') {
        output[0] = output[1] = output[2] = output[3] = output[4] = 0x40U;
    } else if (character == ':') {
        output[2] = 0x36U;
    } else if (character == '.') {
        output[2] = 0x60U;
    } else if (character == '=') {
        output[0] = output[1] = output[2] = output[3] = output[4] = 0x14U;
    } else if (character == '/') {
        output[0] = 0x20U;
        output[1] = 0x10U;
        output[2] = 0x08U;
        output[3] = 0x04U;
        output[4] = 0x02U;
    }
}

static void clear_page(uint8_t page)
{
    uint16_t index;

    for (index = 0U; index < OLED_WIDTH; ++index) {
        g_framebuffer[(uint16_t) page * OLED_WIDTH + index] = 0U;
    }
    g_dirtyPages |= (uint8_t) (1U << page);
}

static void draw_text(uint8_t page, uint8_t x, const char *text)
{
    uint8_t bitmap[5];
    uint8_t column;

    while ((*text != '\0') && ((uint16_t) x + 5U < OLED_WIDTH)) {
        glyph(*text++, bitmap);
        for (column = 0U; column < 5U; ++column) {
            g_framebuffer[(uint16_t) page * OLED_WIDTH + x++] = bitmap[column];
        }
        g_framebuffer[(uint16_t) page * OLED_WIDTH + x++] = 0U;
    }
    g_dirtyPages |= (uint8_t) (1U << page);
}

static bool same_status(const char *status)
{
    uint8_t index = 0U;

    while ((index < (sizeof(g_lastStatus) - 1U)) &&
        (status[index] != '\0') && (g_lastStatus[index] == status[index])) {
        ++index;
    }
    return (status[index] == '\0') && (g_lastStatus[index] == '\0');
}

static void cache_status(const char *status)
{
    uint8_t index = 0U;

    while ((index < (sizeof(g_lastStatus) - 1U)) &&
        (status[index] != '\0')) {
        g_lastStatus[index] = status[index];
        ++index;
    }
    g_lastStatus[index] = '\0';
}

void display_init(void)
{
    static const uint8_t initCommands[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6,
        0xAF
    };
    uint16_t index;

    g_ready = false;
    g_dirtyPages = 0U;
    g_lastStatus[0] = '\0';
    for (index = 0U; index < sizeof(g_framebuffer); ++index) {
        g_framebuffer[index] = 0U;
    }
    delay_cycles(320000U);
    g_oledAddress = OLED_ADDRESS_0;
    if (!oled_write_commands(initCommands, sizeof(initCommands))) {
        oled_recover_transfer();
        g_oledAddress = OLED_ADDRESS_1;
        if (!oled_write_commands(initCommands, sizeof(initCommands))) {
            oled_recover_transfer();
            return;
        }
    }
    g_ready = true;
    g_dirtyPages = 0xFFU;
}

bool display_ready(void)
{
    return g_ready;
}

void display_show_mode(uint8_t question, const char *name)
{
    char modeText[7] = {'M','O','D','E',' ','Q','\0'};
    char numberText[2];

    if (!g_ready) {
        return;
    }
    numberText[0] = (char) ('0' + question);
    numberText[1] = '\0';
    clear_page(0U);
    draw_text(0U, 0U, modeText);
    draw_text(0U, 42U, numberText);
    clear_page(2U);
    draw_text(2U, 0U, name);
}

void display_show_status(const char *status, uint32_t elapsedMs)
{
    char timeText[13] = {'T','I','M','E',' ','0','0','0','.', '0','S','\0','\0'};
    uint32_t tenths;
    uint32_t seconds;

    if (!g_ready) {
        return;
    }
    if (!same_status(status)) {
        cache_status(status);
        clear_page(4U);
        draw_text(4U, 0U, status);
    }
    tenths = elapsedMs / 100U;
    if (tenths == g_lastTenths) {
        return;
    }
    g_lastTenths = tenths;
    seconds = elapsedMs / 1000U;
    if (seconds > 999U) {
        seconds = 999U;
    }
    timeText[5] = (char) ('0' + ((seconds / 100U) % 10U));
    timeText[6] = (char) ('0' + ((seconds / 10U) % 10U));
    timeText[7] = (char) ('0' + (seconds % 10U));
    timeText[9] = (char) ('0' + ((elapsedMs / 100U) % 10U));
    clear_page(6U);
    draw_text(6U, 0U, timeText);
}

void display_service(void)
{
    uint8_t page;

    if (!g_ready || (g_dirtyPages == 0U)) {
        return;
    }
    for (page = 0U; page < OLED_PAGES; ++page) {
        if ((g_dirtyPages & (uint8_t) (1U << page)) != 0U) {
            if (oled_write_page(page)) {
                g_dirtyPages &= (uint8_t) ~(1U << page);
            } else {
                g_ready = false;
            }
            return;
        }
    }
}

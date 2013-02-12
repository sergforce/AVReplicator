#include "VirtualSerialHost.h"
#include "spiio.h"
#include "ctcontrol.h"

#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#define MAX_STRING_LEN   64


char g_ctrecvbuffer[MAX_STRING_LEN+1];
uint8_t g_len; //Message length
uint8_t g_ct_mode; //Current clocktamer mode

const char g_comma[] PROGMEM = ",";
const char g_ctc_hwi[] PROGMEM = "HWI";
const char g_ctc_ver[] PROGMEM = "VER";
const char g_ctc_setfreq[] PROGMEM = "SET,,OUT";
const char g_ctc_setosc[] PROGMEM = "SET,,OSC";
const char g_ctc_setlmkprt[] PROGMEM = "SET,LMK,PRT";
const char g_ctc_ste[] PROGMEM = "STE";
const char g_ctc_lde[] PROGMEM = "LDE";
const char g_ctc_autotest[] PROGMEM = "SET,STS,AUT";

const char g_ctc_pinled[] PROGMEM = "PIN,LED,";

const char g_ctc_inf_osc[] PROGMEM = "INF,,OSC";

const char g_rep_ok[] PROGMEM = "OK";
const char g_rep_syntax[] PROGMEM = "SYNTAX";
const char g_rep_cmd[] PROGMEM = "CMD";
const char g_rep_bad[] PROGMEM = "BAD";
const char g_rep_failed[] PROGMEM = "FAILED";


const char g_selftest[] PROGMEM = "SEL,F_T,EST,";
const char g_selftest_lck[] PROGMEM = "lock pin,";
const char g_selftest_setf[] PROGMEM = "set freq,";
const char g_selftest_counted[] PROGMEM = "counted,";


#define strncmp_PF     strncmp_P
//#define strcmp_PF     strcmp_P
#define strcat_PF     strcat_P
#define strcpy_PF     strcpy_P
#define strlen_PF     strlen_P

uint8_t ct_send_simple_cmd(void)
{
    uint8_t mode = (USB_HostState == HOST_STATE_Configured) ? CTM_USB : CTM_SPI;

    if (g_ct_mode == CTM_SPI || mode == CTM_SPI) {
        g_len = clocktamer_sendcmd((char*)g_ctrecvbuffer, MAX_STRING_LEN);
    } else if (g_ct_mode == CTM_USB || mode == CTM_USB) {
        int16_t res = USB_ClockTamer_Send((char*)g_ctrecvbuffer, MAX_STRING_LEN);
        if (res < 0) {
            //Command failed
            return 1;
        }
        g_len = res;
    } else {
        g_len = 0;
    }

    return (g_len > 0) ? 0 : 1;
}


static uint8_t ct_recv_reply(void)
{
    uint8_t mode = (USB_HostState == HOST_STATE_Configured) ? CTM_USB : CTM_SPI;
    if (g_ct_mode == CTM_SPI || mode == CTM_SPI) {
        g_len = clocktamer_get_replyln((char*)g_ctrecvbuffer, MAX_STRING_LEN);
    } else if (g_ct_mode == CTM_USB || mode == CTM_USB) {
        int16_t res = USB_ClockTamer_RecvLine((char*)g_ctrecvbuffer, MAX_STRING_LEN);
        if (res < 0) {
            //Command failed
            return 1;
        }
        g_len = res;
    } else {
        g_len = 0;
    }
    return (g_len > 0) ? 0 : 1;
}

// Internal helper functions
static void fillcmd_withvalue(const char* cmd, uint32_t value)
{
    strcpy_PF((char*)g_ctrecvbuffer, cmd);
    strcat_PF((char*)g_ctrecvbuffer, g_comma);
    uint8_t len = strlen((char*)g_ctrecvbuffer);
    ultoa(value, (char*)g_ctrecvbuffer + len, 10);
    //g_len = strlen(g_ctrecvbuffer + len);
}

static uint8_t parse_simple_reply(void)
{
    if (strncmp_PF(g_ctrecvbuffer, g_rep_ok, sizeof(g_rep_ok)-1) == 0)
        return CTR_OK;
    else if (strncmp_PF(g_ctrecvbuffer, g_rep_syntax, sizeof(g_rep_syntax)-1) == 0)
        return CTR_SYNTAX_ERROR;
    else if (strncmp_PF(g_ctrecvbuffer, g_rep_cmd, sizeof(g_rep_cmd)-1) == 0)
        return CTR_CMD_ERROR;
    else if (strncmp_PF(g_ctrecvbuffer, g_rep_bad, sizeof(g_rep_bad)-1) == 0)
        return CTR_BAD_TUNING_RANGE;
    else
        return CTR_INCORRECT_REPLY;
}

static uint8_t parse_info_reply(const char* cmd, uint32_t *pvalue)
{
    uint8_t len = strlen_PF(cmd);
    if (strncmp_PF(g_ctrecvbuffer, cmd, len) != 0)
        return CTR_INCORRECT_REPLY;

    if (g_ctrecvbuffer[len] != ',')
        return CTR_INCORRECT_REPLY;

    *pvalue = atol(&g_ctrecvbuffer[len+1]);
    return CTR_OK;
}

// API functions
uint8_t CTInit(uint8_t mode)
{
    g_ct_mode = mode;
    return 0;
}


uint8_t CTSetLed(uint8_t led)
{
    fillcmd_withvalue(g_ctc_pinled, led);
    if (ct_send_simple_cmd() == 0) {
        return parse_simple_reply();
    }
    return CTR_IO_ERROR;
}


uint8_t CTSetOutput(uint32_t freq)
{
    fillcmd_withvalue(g_ctc_setfreq, freq);
    if (ct_send_simple_cmd() == 0) {
        return parse_simple_reply();
    }
    return CTR_IO_ERROR;
}

uint8_t CTSetOsc(uint32_t osc)
{
    fillcmd_withvalue(g_ctc_setosc, osc);
    if (ct_send_simple_cmd() == 0) {
        return parse_simple_reply();
    }
    return CTR_IO_ERROR;
}

uint8_t CTEnableOutputs(uint8_t outputs)
{
    fillcmd_withvalue(g_ctc_setlmkprt, outputs);
    if (ct_send_simple_cmd() == 0) {
        return parse_simple_reply();
    }
    return CTR_IO_ERROR;
}

uint8_t CTStoreToEEPROM(void)
{
    strcpy_PF(g_ctrecvbuffer, g_ctc_ste);
    if (ct_send_simple_cmd() == 0) {
        return parse_simple_reply();
    }
    return CTR_IO_ERROR;
}

uint8_t CTLoadFromEEPROM(void)
{
    strcpy_PF(g_ctrecvbuffer, g_ctc_lde);
    if (ct_send_simple_cmd() == 0) {
        return parse_simple_reply();
    }
    return CTR_IO_ERROR;
}

uint8_t CTGetOsc(uint32_t *posc)
{
    strcpy_PF(g_ctrecvbuffer, g_ctc_inf_osc);
    if (ct_send_simple_cmd() == 0) {
        return parse_info_reply(g_ctc_inf_osc, posc);
    }
    return CTR_IO_ERROR;
}

char* CTVer(void)
{
    strcpy_PF(g_ctrecvbuffer, g_ctc_ver);
    if (ct_send_simple_cmd() == 0) {
        return g_ctrecvbuffer;
    }
    return 0;
}

char* CTHwi(void)
{
    strcpy_PF(g_ctrecvbuffer, g_ctc_hwi);
    if (ct_send_simple_cmd() == 0) {
        return g_ctrecvbuffer;
    }
    return 0;
}



static uint8_t ct_selftest_get_type(void)
{
    // Reply should be with SEL,F_T,EST,
    if (strncmp_PF(g_ctrecvbuffer, g_selftest, sizeof(g_selftest)-1)) {
        return SFT_ERROR;
    }
    const char *ptr = g_ctrecvbuffer + sizeof(g_selftest) - 1;
    if (*ptr >= '0' && *ptr <= '9') {
        return SFT_END;
    } else if (!strncmp_PF(ptr, g_selftest_lck, sizeof(g_selftest_lck)-1)) {
        return SFT_PIN_LOCK;
    } else if (!strncmp_PF(ptr, g_selftest_setf, sizeof(g_selftest_setf)-1)) {
        return SFT_SET_FREQ;
    } else if (!strncmp_PF(ptr, g_selftest_counted, sizeof(g_selftest_counted)-1)) {
        return SFT_COUNTED;
    }
    return SFT_UNKNOWN;
}

static uint8_t ct_selftest_get_status(uint8_t off)
{
    const char *ptr = g_ctrecvbuffer + off;
    if (!strncmp_PF(ptr, g_rep_ok, sizeof(g_rep_ok)-1)) {
        return CTR_OK;
    } else if (!strncmp_PF(ptr, g_rep_failed, sizeof(g_rep_failed)-1)) {
        return CTR_FAILED;
    }
    return CTR_INCORRECT_REPLY;
}

// MAX x`xxx`xxx`xxx
#define UINT32_STR_SIZE  10

uint8_t CTSelfTest(CTOnSelfTestEvent event)
{
    strcpy_PF(g_ctrecvbuffer, g_ctc_autotest);
    if (ct_send_simple_cmd() != 0) {
        return CTR_IO_ERROR;
    }

    uint8_t i;
    for (i = 0; i < 255; i++) {
        //Check result
        uint8_t type = ct_selftest_get_type();
        uint32_t value;
        uint8_t res;

        switch (type) {
        case SFT_END:
            value = atol(g_ctrecvbuffer + sizeof(g_selftest) - 1);
            if (value > 0) {
                return CTR_FAILED;
            } else {
                return CTR_OK;
            }
            break;
        case SFT_PIN_LOCK:
            res = ct_selftest_get_status(sizeof(g_selftest) - 1 +
                                         sizeof(g_selftest_lck) - 1);
            event(SFT_PIN_LOCK, 0, res);
            break;
        case SFT_SET_FREQ:
            value = atol(g_ctrecvbuffer + sizeof(g_selftest) - 1 +
                                          sizeof(g_selftest_setf) - 1);
            res = ct_selftest_get_status(sizeof(g_selftest) - 1 +
                                         sizeof(g_selftest_setf) - 1 +
                                         UINT32_STR_SIZE + 1);
            event(SFT_SET_FREQ, value, res);
            break;
        case SFT_COUNTED:
            //FORMAT g_selftest|g_selftest_setf|uint32|uint32|uint32|status
            value = atol(g_ctrecvbuffer + sizeof(g_selftest) - 1 +
                                          sizeof(g_selftest_counted) - 1 +
                                          2*(UINT32_STR_SIZE + 1));
            res = ct_selftest_get_status(sizeof(g_selftest) - 1 +
                                         sizeof(g_selftest_counted) - 1 +
                                         3*(UINT32_STR_SIZE + 1));
            event(SFT_COUNTED, value, res);
            break;
        case SFT_ERROR:
            //return CTR_IO_ERROR;
            return CTR_INCORRECT_REPLY + 1;
        }

        if (ct_recv_reply()) {
            //Wait more
            if (ct_recv_reply()) {
                //return CTR_IO_ERROR;
                return CTR_INCORRECT_REPLY + 2;
            }
        }
    }
    // Shold not be here
    return CTR_INCORRECT_REPLY;
}

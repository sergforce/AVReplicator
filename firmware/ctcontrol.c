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


const char g_ctc_inf_osc[] PROGMEM = "INF,,OSC";

const char g_rep_ok[] PROGMEM = "OK";
const char g_rep_syntax[] PROGMEM = "SYNTAX";
const char g_rep_cmd[] PROGMEM = "CMD";
const char g_rep_bad[] PROGMEM = "BAD";


#define strcmp_PF     strcmp_P
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
            //Comman failed
            return 1;
        }
        g_len = res;
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
    if (strcmp_PF(g_ctrecvbuffer, g_rep_ok) == 0)
        return CTR_OK;
    else if (strcmp_PF(g_ctrecvbuffer, g_rep_syntax) == 0)
        return CTR_SYNTAX_ERROR;
    else if (strcmp_PF(g_ctrecvbuffer, g_rep_cmd) == 0)
        return CTR_CMD_ERROR;
    else if (strcmp_PF(g_ctrecvbuffer, g_rep_bad) == 0)
        return CTR_BAD_TUNING_RANGE;
    else
        return CTR_LAST_MSG_NO;
}

static uint8_t parse_info_reply(const char* cmd, uint32_t *pvalue)
{
    if (strcmp_PF(g_ctrecvbuffer, cmd) != 0)
        return CTR_INCORRECT_REPLY;
    uint8_t len = strlen_PF(cmd);

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




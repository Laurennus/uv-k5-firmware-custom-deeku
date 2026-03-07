/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <string.h>
#include <stdlib.h>

#include "../app/dtmf.h"
#include "../app/menu.h"
#include "../bitmaps.h"
#include "../board.h"
#include "../dcs.h"
#include "../driver/backlight.h"
#include "../driver/bk4819.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../external/printf/printf.h"
#include "../frequencies.h"
#include "../helper/battery.h"
#include "../misc.h"
#include "../settings.h"
#include "radio.h"
#ifdef ENABLE_FEAT_F4HWN
    #include "../version.h"
#endif
#include "helper.h"
#include "inputbox.h"
#include "menu.h"
#include "ui.h"

const int mainMenuListCount = 6;
const t_menu_item MainMenuList[] = {
    {"Squelch", MENU_SQL},
    {"Channel", SUBMENU_CHANNEL},
    {"Display", SUBMENU_DISPLAY},
    {"System", SUBMENU_SYSTEM},
    {"Scanning", SUBMENU_SCANNING},
    {"Info", MENU_VOL},
};

const int channelMenuListCount = 20;
const t_menu_item ChannelMenuList[] = {
    {"Step", MENU_STEP},
    {"Power", MENU_TXP}, // was "TXP"
    {"Rx DCS", MENU_R_DCS}, // was "R_DCS"
    {"Rx CTCS", MENU_R_CTCS}, // was "R_CTCS"
    {"Tx DCS", MENU_T_DCS}, // was "T_DCS"
    {"Tx CTCS", MENU_T_CTCS}, // was "T_CTCS"
    {"Duplex", MENU_SFT_D}, // was "SFT_D"
    {"Offset", MENU_OFFSET}, // was "OFFSET"
    {"Wide/Nar", MENU_W_N},
    {"AM/FM/SB", MENU_AM}, // was "AM"
    {"Compandr", MENU_COMPAND},
    {"TX Lock", MENU_TX_LOCK},
    {"Busy Lck", MENU_BCL}, // was "BCL"
    {"PTT ID", MENU_PTT_ID},
    {"SList 1", MENU_S_ADD1},
    {"SList 2", MENU_S_ADD2},
    {"SList 3", MENU_S_ADD3},
    {"Ch Save", MENU_MEM_CH}, // was "MEM-CH"
    {"Ch Name", MENU_MEM_NAME},
    {"Ch Del", MENU_DEL_CH}, // was "DEL-CH"
};

const int displayMenuListCount = 13;
const t_menu_item DisplayMenuList[] = {
    {"Bl Time", MENU_ABR}, // was "ABR"
    {"Bl Min", MENU_ABR_MIN},
    {"Bl Max", MENU_ABR_MAX},
    {"Bl TxRx", MENU_ABR_ON_TX_RX},
    {"Set Ctr", MENU_SET_CTR},
    {"Inv Disp", MENU_SET_INV},
    {"Bat Text", MENU_BAT_TXT},
    {"S-Meter", MENU_SET_MET},
    {"GUI Size", MENU_SET_GUI},
    {"RxTxTime", MENU_SET_TMR},
    {"Ch Dspl", MENU_MDF}, // was "MDF"
    {"Mic Bar", MENU_MIC_BAR},
    {"DTMFLive", MENU_D_LIVE_DEC}, // live DTMF decoder
};

const int systemMenuListCount = 28;
const t_menu_item SystemMenuList[] = {
    {"Pwr Save", MENU_SAVE}, // was "SAVE"
    {"Sleep", MENU_SET_OFF},
    {"Keylock", MENU_AUTOLK}, // was "AUTOLk"
    {"Lock PTT", MENU_SET_LCK},
    {"Mic Gain", MENU_MIC},
    {"Spk Gain", MENU_SET_VOL},
    {"User Pwr", MENU_SET_PWR},
    {"Stail El", MENU_STE},
    {"StailRPT", MENU_RP_STE},
    {"Roger", MENU_ROGER},
    {"Tx TOT", MENU_TOT}, // was "TOT"
    {"TOT Alrt", MENU_SET_TOT},
    {"EOT Ind", MENU_SET_EOT},
    {"PON Msg", MENU_PONMSG},
    {"F1 Short", MENU_F1SHRT},
    {"F1 Long", MENU_F1LONG},
    {"F2 Short", MENU_F2SHRT},
    {"F2 Long", MENU_F2LONG},
    {"M Long", MENU_MLONG},
    {"ROps Key", MENU_SET_KEY},
    {"1 Call", MENU_1_CALL},
    {"PTT Type", MENU_SET_PTT},
    {"Key Beep", MENU_BEEP},
    {"DTMF ST", MENU_D_ST},
    {"DTMFPrel", MENU_D_PRE},
    {"UP Code", MENU_UPCODE},
    {"DW Code", MENU_DWCODE},
    {"Dual RX", MENU_TDR},
};

const int scanningMenuListCount = 5;
const t_menu_item ScanningMenuList[] = {
    {"Slist", MENU_S_LIST},
    {"Slist 1", MENU_SLIST1},
    {"Slist 2", MENU_SLIST2},
    {"Slist 3", MENU_SLIST3},
    {"Scan Res", MENU_SC_REV},
};

const int hiddenMenuListCount = 6;
const t_menu_item HiddenMenuList[] = {
    {"F Lock", MENU_F_LOCK},
    {"350 En", MENU_350EN}, // was "350EN"
    {"Xtal Cal", MENU_F_CALI}, // reference xtal calibration
    {"Bat Cal", MENU_BATCAL}, // battery voltage calibration
    {"Bat Type", MENU_BATTYP}, // battery type 1600/2200mAh
    {"Reset", MENU_RESET}, // might be better to move this to the hidden menu items ?
};

const uint8_t FIRST_HIDDEN_MENU_ITEM = MENU_F_LOCK;
const t_menu_item MenuList[] =
{
    {"",                              0xff               }  // end of list - DO NOT delete or move this this
};

const char gSubMenu_TXP[][6] =
{
    "USER",
    "LOW 1",
    "LOW 2",
    "LOW 3",
    "LOW 4",
    "LOW 5",
    "MID",
    "HIGH"
};

const char gSubMenu_SFT_D[][4] =
{
    "OFF",
    "+",
    "-"
};

const char gSubMenu_W_N[][7] =
{
    "WIDE",
    "NARROW"
};

const char gSubMenu_OFF_ON[][4] =
{
    "OFF",
    "ON"
};

const char gSubMenu_NA[4] =
{
    "N/A"
};

const char* const gSubMenu_RXMode[] =
{
    "MAIN\nONLY",       // TX and RX on main only
    "DUAL RX\nRESPOND", // Watch both and respond
    "CROSS\nBAND",      // TX on main, RX on secondary
    "MAIN TX\nDUAL RX"  // always TX on main, but RX on both
};

#ifdef ENABLE_VOICE
    const char gSubMenu_VOICE[][4] =
    {
        "OFF",
        "Chinese",
        "English"
    };
#endif

const char* const gSubMenu_MDF[] =
{
    "FREQ",
    "CHANNEL NUMBER",
    "NAME",
    "NAME\nFREQ"
};

#ifdef ENABLE_ALARM
    const char gSubMenu_AL_MOD[][5] =
    {
        "SITE",
        "TONE"
    };
#endif

#ifdef ENABLE_DTMF_CALLING
const char gSubMenu_D_RSP[][11] =
{
    "DO\nNOTHING",
    "RING",
    "REPLY",
    "BOTH"
};
#endif

const char* const gSubMenu_PTT_ID[] =
{
    "OFF",
    "UP CODE",
    "DOWN CODE",
    "UP+DOWN\nCODE",
    "APOLLO\nQUINDAR"
};

const char gSubMenu_PONMSG[][8] =
{
#ifdef ENABLE_FEAT_F4HWN
    "ALL",
    "SOUND",
#else
    "FULL",
#endif
    "MESSAGE",
    "VOLTAGE",
    "NONE"
};

const char gSubMenu_ROGER[][6] =
{
    "OFF",
    "ROGER",
    "MDC"
};

const char gSubMenu_RESET[][4] =
{
    "VFO",
    "ALL"
};

const char * const gSubMenu_F_LOCK[] =
{
    "DEFAULT+\n137-174\n400-470",
    "FCC HAM\n144-148\n420-450",
#ifdef ENABLE_FEAT_F4HWN_CA
    "CA HAM\n144-148\n430-450",
#endif
    "CE HAM\n144-146\n430-440",
    "GB HAM\n144-148\n430-440",
    "137-174\n400-430",
    "137-174\n400-438",
#ifdef ENABLE_FEAT_F4HWN_PMR
    "PMR 446",
#endif
#ifdef ENABLE_FEAT_F4HWN_GMRS_FRS_MURS
    "GMRS\nFRS\nMURS",
#endif
    "DISABLE\nALL",
    "UNLOCK\nALL",
};

const char gSubMenu_RX_TX[][6] =
{
    "OFF",
    "TX",
    "RX",
    "TX/RX"
};

const char gSubMenu_BAT_TXT[][8] =
{
    "NONE",
    "VOLTAGE",
    "PERCENT"
};

const char gSubMenu_BATTYP[][9] =
{
    "1600mAh",
    "2200mAh",
    "3500mAh"
};

#ifndef ENABLE_FEAT_F4HWN
const char gSubMenu_SCRAMBLER[][7] =
{
    "OFF",
    "2600Hz",
    "2700Hz",
    "2800Hz",
    "2900Hz",
    "3000Hz",
    "3100Hz",
    "3200Hz",
    "3300Hz",
    "3400Hz",
    "3500Hz"
};
#endif

#ifdef ENABLE_FEAT_F4HWN
    const char gSubMenu_SET_PWR[][6] =
    {
        "< 20m",
        "125m",
        "250m",
        "500m",
        "1",
        "2",
        "5"
    };

    const char gSubMenu_SET_PTT[][8] =
    {
        "CLASSIC",
        "ONEPUSH"
    };

    const char gSubMenu_SET_TOT[][7] =  // Use by SET_EOT too
    {
        "OFF",
        "SOUND",
        "VISUAL",
        "ALL"
    };

    const char gSubMenu_SET_LCK[][9] =
    {
        "KEYS",
        "KEYS+PTT"
    };

    const char gSubMenu_SET_MET[][8] =
    {
        "TINY",
        "CLASSIC"
    };

    #ifdef ENABLE_FEAT_F4HWN_NARROWER
        const char gSubMenu_SET_NFM[][9] =
        {
            "NARROW",
            "NARROWER"
        };
    #endif

    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
        const char gSubMenu_SET_KEY[][9] =
        {
            "KEY_MENU",
            "KEY_UP",
            "KEY_DOWN",
            "KEY_EXIT",
            "KEY_STAR"
        };
    #endif
#endif

const t_sidefunction gSubMenu_SIDEFUNCTIONS[] =
{
    {"NONE",            ACTION_OPT_NONE},
#ifdef ENABLE_FLASHLIGHT
    {"FLASH\nLIGHT",    ACTION_OPT_FLASHLIGHT},
#endif
    {"POWER",           ACTION_OPT_POWER},
    {"MONITOR",         ACTION_OPT_MONITOR},
    {"SCAN",            ACTION_OPT_SCAN},
#ifdef ENABLE_VOX
    {"VOX",             ACTION_OPT_VOX},
#endif
#ifdef ENABLE_ALARM
    {"ALARM",           ACTION_OPT_ALARM},
#endif
#ifdef ENABLE_FMRADIO
    {"FM RADIO",        ACTION_OPT_FM},
#endif
#ifdef ENABLE_TX1750
    {"1750Hz",          ACTION_OPT_1750},
#endif
#ifdef ENABLE_REGA
    {"REGA\nALARM",     ACTION_OPT_REGA_ALARM},
    {"REGA\nTEST",      ACTION_OPT_REGA_TEST},
#endif
    {"LOCK\nKEYPAD",    ACTION_OPT_KEYLOCK},
    {"VFO A\nVFO B",    ACTION_OPT_A_B},
    {"VFO\nMEM",        ACTION_OPT_VFO_MR},
    {"MODE",            ACTION_OPT_SWITCH_DEMODUL},
#ifdef ENABLE_BLMIN_TMP_OFF
    {"BLMIN\nTMP OFF",  ACTION_OPT_BLMIN_TMP_OFF},      //BackLight Minimum Temporay OFF
#endif
#ifdef ENABLE_FEAT_F4HWN
    {"RX MODE",         ACTION_OPT_RXMODE},
    {"MAIN ONLY",       ACTION_OPT_MAINONLY},
    {"PTT",             ACTION_OPT_PTT},
    {"WIDE\nNARROW",    ACTION_OPT_WN},
    #if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
    {"MUTE",            ACTION_OPT_MUTE},
    #endif
    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
        {"POWER\nHIGH",    ACTION_OPT_POWER_HIGH},
        {"REMOVE\nOFFSET",  ACTION_OPT_REMOVE_OFFSET},
    #endif
#endif
};

const uint8_t gSubMenu_SIDEFUNCTIONS_size = ARRAY_SIZE(gSubMenu_SIDEFUNCTIONS);

bool    gIsInSubMenu;
uint8_t gMenuCursor;
int UI_MENU_GetCurrentMenuId() {
    if(gMenuCursor < ARRAY_SIZE(MenuList))
        return MenuList[gMenuCursor].menu_id;

    return MenuList[ARRAY_SIZE(MenuList)-1].menu_id;
}

uint8_t UI_MENU_GetMenuIdx(uint8_t id)
{
    for(uint8_t i = 0; i < ARRAY_SIZE(MenuList); i++)
        if(MenuList[i].menu_id == id)
            return i;
    return 0;
}

int32_t gSubMenuSelection;

// edit box
char    edit_original[17]; // a copy of the text before editing so that we can easily test for changes/difference
char    edit[17];
int     edit_index;

int inSubMenu;
int mainMenuIndex;
int mainMenuIndexOnScreen;
int subMenuIndex;
int subMenuIndexOnScreen;
int *currentMenuIndex = &mainMenuIndex;
int *currentMenuIndexOnScreen = &mainMenuIndexOnScreen;
bool menuEditing = false;
int menuSelection;
const int *currentMenuListCount = &mainMenuListCount;
const t_menu_item (*currentMenuList)[];

void UI_DisplayMenu(void) {
    char String[32];
    char offon[][4] = {"off", "on"};
    switch (inSubMenu) {
        case SUBMENU_CHANNEL:
            strcpy(String, "Channel");
            currentMenuIndex = &subMenuIndex;
            currentMenuIndexOnScreen = &subMenuIndexOnScreen;
            currentMenuList = &ChannelMenuList;
            currentMenuListCount = &channelMenuListCount;
            break;
        case SUBMENU_DISPLAY:
            strcpy(String, "Display");
            currentMenuIndex = &subMenuIndex;
            currentMenuIndexOnScreen = &subMenuIndexOnScreen;
            currentMenuList = &DisplayMenuList;
            currentMenuListCount = &displayMenuListCount;
            break;
        case SUBMENU_HIDDEN:
            strcpy(String, "Service");
            currentMenuIndex = &subMenuIndex;
            currentMenuIndexOnScreen = &subMenuIndexOnScreen;
            currentMenuList = &HiddenMenuList;
            currentMenuListCount = &hiddenMenuListCount;
            break;
        case SUBMENU_SYSTEM:
            strcpy(String, "System");
            currentMenuIndex = &subMenuIndex;
            currentMenuIndexOnScreen = &subMenuIndexOnScreen;
            currentMenuList = &SystemMenuList;
            currentMenuListCount = &systemMenuListCount;
            break;
        case SUBMENU_SCANNING:
            strcpy(String, "Scanning");
            currentMenuIndex = &subMenuIndex;
            currentMenuIndexOnScreen = &subMenuIndexOnScreen;
            currentMenuList = &ScanningMenuList;
            currentMenuListCount = &scanningMenuListCount;
            break;
        case 0:
        default:
            strcpy(String, "Menu");
            inSubMenu = 0;
            currentMenuIndex = &mainMenuIndex;
            currentMenuIndexOnScreen = &mainMenuIndexOnScreen;
            currentMenuList = &MainMenuList;
            currentMenuListCount = &mainMenuListCount;
            break;
    }

    UI_DisplayClear();

    // toprow
    UI_PrintStringSmallNormal(String, 0, LCD_WIDTH-1, 0);
    for (int i = 0; i < LCD_WIDTH; i++)
    {
        gFrameBuffer[0][i] ^= 0xfe;
    }

    for (int i=0; i<MIN(6, *currentMenuListCount); i++) {
        // item num
        sprintf(String, "% 2u", *currentMenuIndex + i - *currentMenuIndexOnScreen);
        GUI_DisplaySmallest(String, 4, (i+1)*8 + 2, false, true);
        // item name
        UI_PrintStringSmallNormal((*currentMenuList)[*currentMenuIndex + i - *currentMenuIndexOnScreen].name, 12, 0, i+1);
    }

    // border
    for(int y=1; y<7; y++) {
        gFrameBuffer[y][0] = 0xff;
        gFrameBuffer[y][LCD_WIDTH-1] = 0xff;
    }

    // populate current values
    for (int i=0; i<MIN(6, *currentMenuListCount); i++) {
        int menuSelectionTmp = menuSelection;
        switch ((*currentMenuList)[*currentMenuIndex + i - *currentMenuIndexOnScreen].menu_id) {
            case SUBMENU_CHANNEL:
            case SUBMENU_DISPLAY:
            case SUBMENU_SYSTEM:
            case SUBMENU_SCANNING:
            case SUBMENU_HIDDEN:
                strcpy(String, "...");
                break;
            case MENU_SQL:
                if (i != *currentMenuIndexOnScreen || !menuEditing)
                    menuSelectionTmp=gEeprom.SQUELCH_LEVEL;
                sprintf(String, "%d", menuSelectionTmp);
                break;
            case MENU_VOL:
                strcpy(String, EDITION_STRING);
                break;
            // case MENU_STEP:
            //     sprintf(String, "%d.%02u kHz", gTxVfo->StepFrequency / 100, gTxVfo->StepFrequency % 100);
            //     break;
            case MENU_TXP:
                if (i != *currentMenuIndexOnScreen || !menuEditing)
                    menuSelectionTmp = gTxVfo->OUTPUT_POWER;
                const char pwr_longer[][8] = {"< 20 mW", "125 mW", "250 mW", "500 mW", "1 W", "2 W", "5 W"};
                if (menuSelectionTmp == OUTPUT_POWER_USER)
                    sprintf(String, "User: %s", pwr_longer[gSetting_set_pwr]);
                else
                    strcpy(String, pwr_longer[menuSelectionTmp-1]);
                break;

            // TODO: DCS/CTCSS tone selection
            case MENU_R_DCS:
                if (gTxVfo->freq_config_RX.CodeType == CODE_TYPE_DIGITAL)
                    sprintf(String, "%03o Normal", DCS_Options[menuSelectionTmp]);
                else if (gTxVfo->freq_config_RX.CodeType == CODE_TYPE_REVERSE_DIGITAL)
                    sprintf(String, "%03o Inverted", DCS_Options[menuSelectionTmp]);
                else
                    strcpy(String, "-");
                break;
            case MENU_T_DCS:
                if (gTxVfo->freq_config_TX.CodeType == CODE_TYPE_DIGITAL)
                    sprintf(String, "%03o Normal", DCS_Options[gTxVfo->freq_config_TX.Code]);
                else if (gTxVfo->freq_config_TX.CodeType == CODE_TYPE_REVERSE_DIGITAL)
                    sprintf(String, "%03o Inverted", DCS_Options[gTxVfo->freq_config_TX.Code]);
                else
                    strcpy(String, "-");
                break;
            case MENU_R_CTCS:
                if (gTxVfo->freq_config_RX.CodeType == CODE_TYPE_CONTINUOUS_TONE)
                    sprintf(String, "%u.%u Hz", CTCSS_Options[gTxVfo->freq_config_RX.Code] / 10, CTCSS_Options[gTxVfo->freq_config_RX.Code] % 10);
                else
                    strcpy(String, "-");
                break;
            case MENU_T_CTCS:
                if (gTxVfo->freq_config_TX.CodeType == CODE_TYPE_CONTINUOUS_TONE)
                    sprintf(String, "%u.%u Hz", CTCSS_Options[gTxVfo->freq_config_TX.Code] / 10, CTCSS_Options[gTxVfo->freq_config_TX.Code] % 10);
                else
                    strcpy(String, "-");
                break;

            case MENU_SFT_D:
                switch (gTxVfo->TX_OFFSET_FREQUENCY_DIRECTION) {
                    case TX_OFFSET_FREQUENCY_DIRECTION_OFF:
                        strcpy(String, "off");
                        break;
                    case TX_OFFSET_FREQUENCY_DIRECTION_ADD:
                        strcpy(String, "+");
                        break;
                    case TX_OFFSET_FREQUENCY_DIRECTION_SUB:
                        strcpy(String, "-");
                        break;
                    default:
                        strcpy(String, "?");
                }
                break;
            case MENU_OFFSET:
                sprintf(String, "%u.%u MHz", gTxVfo->TX_OFFSET_FREQUENCY / 100000, gTxVfo->TX_OFFSET_FREQUENCY % 100000);
                break;

            case MENU_W_N:
                if (gTxVfo->CHANNEL_BANDWIDTH == BANDWIDTH_WIDE)
                    strcpy(String, "Wide");
                else if (gTxVfo->CHANNEL_BANDWIDTH == BANDWIDTH_NARROW)
                    strcpy(String, "Narrow");
                break;

            case MENU_AM:
                switch (gTxVfo->Modulation) {
                    case MODULATION_AM:
                        strcpy(String, "AM");
                        break;
                    case MODULATION_FM:
                        strcpy(String, "FM");
                        break;
                    case MODULATION_USB:
                        strcpy(String, "USB");
                        break;
                    default:
                        strcpy(String, "?");
                }
                break;

            case MENU_COMPAND:
                char companderString[][8] = { " - / - ", " - / Tx", "Rx / - ", "Rx / Tx" };
                strcpy(String, companderString[gTxVfo->Compander]);
                break;

            case MENU_TX_LOCK:
                sprintf(String, "%-3s%s", offon[gTxVfo->TX_LOCK], (TX_freq_check(gEeprom.VfoInfo[gEeprom.TX_VFO].pTX->Frequency) == 0)?"(no f-lock)":"");
                break;
            case MENU_BCL:
                strcpy(String, offon[gTxVfo->BUSY_CHANNEL_LOCK]);
                break;

            case MENU_S_ADD1:
                strcpy(String, offon[gTxVfo->SCANLIST1_PARTICIPATION]);
                break;
            case MENU_S_ADD2:
                strcpy(String, offon[gTxVfo->SCANLIST2_PARTICIPATION]);
                break;
            case MENU_S_ADD3:
                strcpy(String, offon[gTxVfo->SCANLIST3_PARTICIPATION]);
                break;
            case MENU_MEM_CH:
                sprintf(String, "%03u %s", gTxVfo->CHANNEL_SAVE+1, gTxVfo->CHANNEL_SAVE<200?gTxVfo->Name:"VFO Band");
                break;
            case MENU_ABR_ON_TX_RX:
                sprintf(String, "%s / %s",  gSetting_backlight_on_tx_rx&BACKLIGHT_ON_TR_RX?"Rx":" -", gSetting_backlight_on_tx_rx&BACKLIGHT_ON_TR_TX?"Tx":"- ");
                break;
            case MENU_SET_INV:
                strcpy(String, offon[gSetting_set_inv]);
                break;
            case MENU_BAT_TXT:
                char batSettingTexts[][8] = {"Off", "Voltage", "Percent"};
                strcpy(String, batSettingTexts[gSetting_battery_text]);
                break;
            case MENU_SET_MET:
                strcpy(String, gSetting_set_met?"Classic":"Tiny");
                break;
            case MENU_SET_GUI:
                strcpy(String, gSetting_set_gui?"Stock":"Better");
                break;
            case MENU_SET_TMR:
                strcpy(String, offon[gSetting_set_tmr]);
                break;
            case MENU_MIC_BAR:
                strcpy(String, offon[gSetting_mic_bar]);
                break;
            case MENU_D_LIVE_DEC:
                strcpy(String, offon[gSetting_live_DTMF_decoder]);
                break;
            default:
                strcpy(String, "?");
        }
        String[14] = 0; // ensure our string fits the display, just in case
        GUI_DisplaySmallest(String, 70, i*8+10, false, true);
    }


    // highlight current selection
    gFrameBuffer[1 + *currentMenuIndexOnScreen][2] |= 0b01111100;
    if (!menuEditing)
        for (int i = 12; i < 12+8*7; i++)
            gFrameBuffer[1 + *currentMenuIndexOnScreen][i] ^= 0b11111110;
    else
        for (int i = 12+8*7; i < LCD_WIDTH-3; i++)
            gFrameBuffer[1 + *currentMenuIndexOnScreen][i] ^= 0b11111110;

    for (int i=3; i<LCD_WIDTH-4; i++)
        gFrameBuffer[1 + *currentMenuIndexOnScreen][i] |= 0b10000010;
    gFrameBuffer[1 + *currentMenuIndexOnScreen][LCD_WIDTH-3] |= 0b01111100;

    ST7565_BlitFullScreen();
}

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

#ifdef ENABLE_FMRADIO

#include <string.h>

#include "app/fm.h"
#include "driver/bk1080.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "misc.h"
#include "settings.h"
#include "ui/fmradio.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "ui/ui.h"

static uint8_t lineOffset;
static void drawFM(void);
static void invertFMTop(void);
void UI_DisplayFM(void)
{
    char String[16] = {0};
    unsigned int activeTxVFO = gRxVfoIsActive ? gEeprom.RX_VFO : gEeprom.TX_VFO;
    if ((gEeprom.DUAL_WATCH == DUAL_WATCH_OFF) && (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF)) // main only
        lineOffset = 3;
    else
        lineOffset = 4 - 4*activeTxVFO;
    //UI_DisplayClear();
    for (int line = 0;line < 3; line++) {
        for (int i = 0; i < LCD_WIDTH; i++)
        {
            gFrameBuffer[line+lineOffset][i] = 0x00;
        }
    }


    UI_PrintString("FM", 3, 0, 0+lineOffset, 8);

    sprintf(String, "%d%s-%dM",
        BK1080_GetFreqLoLimit(gEeprom.FM_Band)/10,
        gEeprom.FM_Band == 0 ? ".5" : "",
        BK1080_GetFreqHiLimit(gEeprom.FM_Band)/10
        );

    UI_PrintStringSmallNormal(String, 1, 0, 2+lineOffset);

    //uint8_t spacings[] = {20,10,5};
    //sprintf(String, "%d0k", spacings[gEeprom.FM_Space % 3]);
    //UI_PrintStringSmallNormal(String, 127 - 4*7, 0, 6);

    if (gAskToSave) {
        UI_PrintString("SAV?", 86, 0, 0+lineOffset, 8);
    } else if (gAskToDelete) {
        UI_PrintString("DEL?", 86, 0, 0+lineOffset, 8);
    } else if (gFM_ScanState == FM_SCAN_OFF) {
        if (gEeprom.FM_IsMrMode) {

            UI_PrintStringSmallNormal("MR", 127-4*7, 0, 0+lineOffset);
            sprintf(String, "CH%02u", gEeprom.FM_SelectedChannel + 1);
            UI_PrintStringSmallNormal(String, 127-4*7, 0, 1+lineOffset);
        } else {
            UI_PrintStringSmallNormal("VFO", 127-4*7, 0, 0+lineOffset);
            for (unsigned int i = 0; i < 20; i++) {
                if (gEeprom.FM_FrequencyPlaying == gFM_Channels[i]) {
                    sprintf(String, "CH%02u", gEeprom.FM_SelectedChannel + 1);
                    UI_PrintStringSmallNormal(String, 127-4*7, 0, 1+lineOffset);
                    break;
                }
            }
        }
    } else if (gFM_AutoScan) {
        UI_PrintStringSmallNormal("Scan", 127-4*7, 0, 0+lineOffset);
        sprintf(String, "CH%02u", gFM_ChannelPosition + 1);
        UI_PrintStringSmallNormal(String, 127-4*7, 0, 1+lineOffset);
    } else {
        UI_PrintStringSmallNormal("Scan", 127-4*7, 0, 0+lineOffset);
    }

    memset(String, 0, sizeof(String));
    if (gAskToSave || (gEeprom.FM_IsMrMode && gInputBoxIndex > 0)) {
        UI_GenerateChannelString(String, gFM_ChannelPosition);
    } else if (gAskToDelete) {
        sprintf(String, "CH-%02u", gEeprom.FM_SelectedChannel + 1);
    } else {
        if (gInputBoxIndex == 0) {
            sprintf(String, "%4d.%d", gEeprom.FM_FrequencyPlaying / 10, gEeprom.FM_FrequencyPlaying % 10);
        } else {
            const char * ascii = INPUTBOX_GetAscii();
            sprintf(String, "%.3s.%.1s",ascii, ascii + 3);
        }

        UI_DisplayFrequency(String, 32, 0+lineOffset, gInputBoxIndex == 0);  // frequency
        //UI_DisplayFrequency(String, 32, lineOffset, false);

        drawFM();
        return;
    }

    UI_PrintString(String, 32, 0, 0+lineOffset, 8);

    drawFM();
}

static void drawFM(void) {
    invertFMTop();
    ST7565_BlitFullScreen();
}
static void invertFMTop(void) {
    //invert top
    for (int i = 0; i < LCD_WIDTH; i++)
    {
        gFrameBuffer[lineOffset][i] ^= 0xff;
        gFrameBuffer[lineOffset+1][i] ^= 0xff;
    }
}

#endif

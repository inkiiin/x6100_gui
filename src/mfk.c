/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  Xiegu X6100 LVGL GUI
 *
 *  Copyright (c) 2022-2023 Belousov Oleg aka R1CBU
 */

#include "lvgl/lvgl.h"
#include "mfk.h"
#include "params.h"
#include "spectrum.h"
#include "waterfall.h"
#include "msg.h"
#include "dsp.h"
#include "radio.h"
#include "cw.h"
#include "rtty.h"
#include "util.h"
#include "info.h"
#include "backlight.h"

mfk_state_t  mfk_state = MFK_STATE_EDIT;
mfk_mode_t   mfk_mode = MFK_MIN_LEVEL;

void mfk_update(int16_t diff) {
    int32_t     i;
    char        *str;
    bool        b;
    float       f;

    uint32_t    color = mfk_state == MFK_STATE_EDIT ? 0xFFFFFF : 0xBBBBBB;

    switch (mfk_mode) {
        case MFK_MIN_LEVEL:
            if (diff != 0) {
                waterfall_change_min(diff);
                spectrum_set_min(params_band.grid_min);
            }
            msg_set_text_fmt("#%3X Min level: %i dB", color, params_band.grid_min);
            break;
            
        case MFK_MAX_LEVEL:
            if (diff != 0) {
                waterfall_change_max(diff);
                spectrum_set_max(params_band.grid_max);
            }
            msg_set_text_fmt("#%3X Max level: %i dB", color, params_band.grid_max);
            break;

        case MFK_SPECTRUM_FACTOR:
            if (diff != 0) {
                params_lock();
                params_mode.spectrum_factor += diff;
                
                if (params_mode.spectrum_factor < 1) {
                    params_mode.spectrum_factor = 1;
                } else if (params_mode.spectrum_factor > 4) {
                    params_mode.spectrum_factor = 4;
                }
                params_unlock(&params_mode.durty.spectrum_factor);
            
                spectrum_mode_set();
            }
            msg_set_text_fmt("#%3X Spectrum zoom: x%i", color, params_mode.spectrum_factor);
            break;

        case MFK_SPECTRUM_BETA:
            if (diff != 0) {
                params_lock();
                params.spectrum_beta += (diff < 0) ? -5 : 5;
                
                if (params.spectrum_beta < 0) {
                    params.spectrum_beta = 0;
                } else if (params.spectrum_beta > 90) {
                    params.spectrum_beta = 90;
                }
                params_unlock(&params.durty.spectrum_beta);
            
                dsp_set_spectrum_beta(params.spectrum_beta / 100.0f);
            }
            msg_set_text_fmt("#%3X Spectrum beta: %i", color, params.spectrum_beta);
            break;

        case MFK_SPECTRUM_FILL:
            if (diff != 0) {
                params_lock();
                params.spectrum_filled = !params.spectrum_filled;
                params_unlock(&params.durty.spectrum_filled);
            }
            msg_set_text_fmt("#%3X Spectrum fill: %s", color, params.spectrum_filled ? "On" : "Off");
            break;
            
        case MFK_SPECTRUM_PEAK:
            if (diff != 0) {
                params_lock();
                params.spectrum_peak = !params.spectrum_peak;
                params_unlock(&params.durty.spectrum_peak);
            }
            msg_set_text_fmt("#%3X Spectrum peak: %s", color, params.spectrum_peak ? "On" : "Off");
            break;

        case MFK_PEAK_HOLD:
            if (diff != 0) {
                i = params.spectrum_peak_hold + diff * 1000;
                
                if (i < 1000) {
                    i = 1000;
                } else if (i > 10000) {
                    i = 10000;
                }
                
                params_lock();
                params.spectrum_peak_hold = i;
                params_unlock(&params.durty.spectrum_peak_hold);
            }
            msg_set_text_fmt("#%3X Peak hold: %i s", color, params.spectrum_peak_hold / 1000);
            break;
            
        case MFK_PEAK_SPEED:
            if (diff != 0) {
                f = params.spectrum_peak_speed + diff * 0.1f;
                
                if (f < 0.1f) {
                    f = 0.1f;
                } else if (f > 3.0f) {
                    f = 3.0f;
                }
                
                params_lock();
                params.spectrum_peak_speed = f;
                params_unlock(&params.durty.spectrum_peak_speed);
            }
            msg_set_text_fmt("#%3X Peak speed: %.1f dB", color, params.spectrum_peak_speed);
            break;
            
        case MFK_KEY_SPEED:
            i = radio_change_key_speed(diff);
            msg_set_text_fmt("#%3X Key speed: %i wpm", color, i);
            break;

        case MFK_KEY_MODE:
            i = radio_change_key_mode(diff);
            
            switch (i) {
                case x6100_key_manual:
                    str = "Manual";
                    break;

                case x6100_key_auto_left:
                    str = "Auto-L";
                    break;

                case x6100_key_auto_right:
                    str = "Auto-R";
                    break;
            }
            msg_set_text_fmt("#%3X Key mode: %s", color, str);
            break;
            
        case MFK_IAMBIC_MODE:
            i = radio_change_iambic_mode(diff);
            
            switch (i) {
                case x6100_iambic_a:
                    str = "A";
                    break;

                case x6100_iambic_b:
                    str = "B";
                    break;
            }
            msg_set_text_fmt("#%3X Iambic mode: %s", color, str);
            break;
            
        case MFK_KEY_TONE:
            i = radio_change_key_tone(diff);
            msg_set_text_fmt("#%3X Key tone: %i Hz", color, i);
            break;

        case MFK_KEY_VOL:
            i = radio_change_key_vol(diff);
            msg_set_text_fmt("#%3X Key volume: %i", color, i);
            break;

        case MFK_KEY_TRAIN:
            b = radio_change_key_train(diff);
            msg_set_text_fmt("#%3X Key train: %s", color, b ? "On" : "Off");
            break;

        case MFK_QSK_TIME:
            i = radio_change_qsk_time(diff);
            msg_set_text_fmt("#%3X QSK time: %i ms", color, i);
            break;

        case MFK_KEY_RATIO:
            i = radio_change_key_ratio(diff);
            msg_set_text_fmt("#%3X Key ratio: %.1f", color, i * 0.1f);
            break;

        case MFK_CHARGER:
            i = radio_change_charger(diff);
            
            switch (i) {
                case RADIO_CHARGER_OFF:
                    str = "Off";
                    break;
                    
                case RADIO_CHARGER_ON:
                    str = "On";
                    break;
                    
                case RADIO_CHARGER_SHADOW:
                    str = "Shadow";
                    break;
            }
            msg_set_text_fmt("#%3X Charger: %s", color, str);
            break;
            
        case MFK_ANT:
            if (diff != 0) {
                params_lock();
                params.ant = limit(params.ant + diff, 1, 5);
                params_unlock(&params.durty.ant);
                
                radio_load_atu();
                info_atu_update();
            }
            msg_set_text_fmt("#%3X Antenna : %i", color, params.ant);
            break;

        case MFK_RIT:
            i = radio_change_rit(diff);
            msg_set_text_fmt("#%3X RIT: %c%i", color, i < 0 ? '-' : '+', abs(i));
            break;

        case MFK_XIT:
            i = radio_change_xit(diff);
            msg_set_text_fmt("#%3X XIT: %c%i", color, i < 0 ? '-' : '+', abs(i));
            break;
            
        case MFK_DNF:
            b = radio_change_dnf(diff);
            msg_set_text_fmt("#%3X DNF: %s", color, b ? "On" : "Off");
            break;

        case MFK_DNF_CENTER:
            i = radio_change_dnf_center(diff);
            msg_set_text_fmt("#%3X DNF center: %i Hz", color, i);
            break;
            
        case MFK_DNF_WIDTH:
            i = radio_change_dnf_width(diff);
            msg_set_text_fmt("#%3X DNF width: %i Hz", color, i);
            break;

        case MFK_NB:
            b = radio_change_nb(diff);
            msg_set_text_fmt("#%3X NB: %s", color, b ? "On" : "Off");
            break;

        case MFK_NB_LEVEL:
            i = radio_change_nb_level(diff);
            msg_set_text_fmt("#%3X NB level: %i", color, i);
            break;

        case MFK_NB_WIDTH:
            i = radio_change_nb_width(diff);
            msg_set_text_fmt("#%3X NB width: %i Hz", color, i);
            break;

        case MFK_NR:
            b = radio_change_nr(diff);
            msg_set_text_fmt("#%3X NR: %s", color, b ? "On" : "Off");
            break;

        case MFK_NR_LEVEL:
            i = radio_change_nr_level(diff);
            msg_set_text_fmt("#%3X NR level: %i", color, i);
            break;

        case MFK_AGC_HANG:
            b = radio_change_agc_hang(diff);
            msg_set_text_fmt("#%3X AGC hang: %s", color, b ? "On" : "Off");
            break;

        case MFK_AGC_KNEE:
            i = radio_change_agc_knee(diff);
            msg_set_text_fmt("#%3X AGC knee: %i dB", color, i);
            break;

        case MFK_AGC_SLOPE:
            i = radio_change_agc_slope(diff);
            msg_set_text_fmt("#%3X AGC slope: %i dB", color, i);
            break;

        case MFK_CW_DECODER:
            b = cw_change_decoder(diff);
            msg_set_text_fmt("#%3X CW decoder: %s", color, b ? "On" : "Off");
            break;
            
        case MFK_CW_DECODER_SNR:
            f = cw_change_snr(diff);
            msg_set_text_fmt("#%3X CW decoder SNR: %.1f dB", color, f);
            break;
            
        case MFK_CW_DECODER_PEAK_BETA:
            f = cw_change_peak_beta(diff);
            msg_set_text_fmt("#%3X CW decoder peak beta: %.2f", color, f);
            break;
            
        case MFK_CW_DECODER_NOISE_BETA:
            f = cw_change_noise_beta(diff);
            msg_set_text_fmt("#%3X CW decoder noise beta: %.2f", color, f);
            break;

        case MFK_RTTY_RATE:
            f = rtty_change_rate(diff);
            msg_set_text_fmt("#%3X RTTY rate: %.2f", color, f);
            break;

        case MFK_RTTY_SHIFT:
            i = rtty_change_shift(diff);
            msg_set_text_fmt("#%3X RTTY shift: %i Hz", color, i);
            break;
        
        case MFK_RTTY_CENTER:
            i = rtty_change_center(diff);
            msg_set_text_fmt("#%3X RTTY center: %i Hz", color, i);
            break;
        
        case MFK_RTTY_REVERSE:
            b = rtty_change_reverse(diff);
            msg_set_text_fmt("#%3X RTTY reverse: %s", color, b ? "On" : "Off");
            break;

        default:
            break;
    }
}

void mfk_press(int16_t dir) {
    while (true) {
        if (dir > 0) {
            if (mfk_mode == MFK_LAST-1) {
                mfk_mode = 0;
            } else {
                mfk_mode++;
            }
        } else {
            if (mfk_mode == 0) {
                mfk_mode = MFK_LAST-1;
            } else {
                mfk_mode--;
            }
        }
        
        uint64_t mask = (uint64_t) 1L << mfk_mode;
        
        if (params.mfk_modes & mask) {
            break;
        }
    }
    
    mfk_update(0);
}

void mfk_set_mode(mfk_mode_t mode) {
    mfk_mode = mode;
}

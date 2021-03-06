/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#include "cmsis_os.h"
#include "hal_trace.h"
#include "resources.h"
#include "app_bt_stream.h"
#include "app_media_player.h"
#include "app_factory.h"
#include "string.h"

// for audio
#include "audioflinger.h"
#include "app_audio.h"
#include "app_utils.h"
#include "hal_timer.h"

#include "app_factory_audio.h"
#include "tgt_hardware.h"

#ifdef NOTCH_FILTER
#include "autowah.h"
#endif

#ifdef WL_NSX
#include "nsx_main.h"
#include "app_overlay.h"
#endif


#ifdef WEBRTC_AGC
#include "agc_main.h"
#endif

#ifdef WL_NSX
#define WEBRTC_NSX_BUFF_SIZE    	(14000)
#endif

#ifdef AUDIO_DEBUG
#include "audio_dump.h"
#endif

#ifdef GCC_PLAT
#include "gcc_plat.h"

#define GCC_PLAT_START_THD 200
#endif

#ifdef AUDIO_DEBUG
#include "audio_dump.h"
#endif

#ifdef WL_VAD
#include "vad_user.h"
#endif


#ifdef WL_GPIO_SWITCH
#include "tgt_hardware.h"
#endif

#ifdef WL_LED_ZG_SWITCH
#include "tgt_hardware.h" 
#endif

#ifdef WL_DEBUG_MODE
#include "nvrecord_env.h"
#endif

#ifdef WL_FIR_FILTER
#include "wl_fir_filter.h"
#endif

#ifdef __FACTORY_MODE_SUPPORT__

#if SPEECH_CODEC_CAPTURE_SAMPLE == 48000    

#define BT_AUDIO_FACTORMODE_BUFF_SIZE    	(3*320*2)

#elif SPEECH_CODEC_CAPTURE_SAMPLE == 44100  

#define BT_AUDIO_FACTORMODE_BUFF_SIZE    	(440*4)

#elif SPEECH_CODEC_CAPTURE_SAMPLE == 32000  

#define BT_AUDIO_FACTORMODE_BUFF_SIZE    	(2*320*2)

#else

#ifdef WL_NSX_5MS
#define BT_AUDIO_FACTORMODE_BUFF_SIZE    	(160*2)
#else
#define BT_AUDIO_FACTORMODE_BUFF_SIZE    	(320*2)
#endif

#endif

static enum APP_AUDIO_CACHE_T a2dp_cache_status = APP_AUDIO_CACHE_QTY;
static int16_t *app_audioloop_play_cache = NULL;


short out_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
short revert_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
short audio_uart_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];


#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2    

short one_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
short two_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];


static short one_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
static short two_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];

static void aaudio_div_stero_to_rmono(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (uint32_t i = 0; i < src_len>>1; i++)
    {
        dst_buf[i] = src_buf[i*2 + 1];
    }
}

static void aaudio_div_stero_to_lmono(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (uint32_t i = 0; i < src_len>>1; i++)
    {
        dst_buf[i] = src_buf[i*2 + 0];
    }
}

#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 3  

short one_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
short two_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
short three_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];


void aaudio_div_three_to_mono_one(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (uint32_t i = 0; i < src_len>>1; i++)
    {
        dst_buf[i] = src_buf[i*3 + 0];
    }
}

void aaudio_div_three_to_mono_two(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (uint32_t i = 0; i < src_len>>1; i++)
    {
        dst_buf[i] = src_buf[i*3 + 1];
    }
}

void aaudio_div_three_to_mono_three(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (uint32_t i = 0; i < src_len>>1; i++)
    {
        dst_buf[i] = src_buf[i*3 + 2];
    }
}


#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 4  

short one_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
short two_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
short three_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
short four_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];


void aaudio_div_four_to_mono_one(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (uint32_t i = 0; i < src_len>>1; i++)
    {
        dst_buf[i] = src_buf[i*4 + 0];
    }
}

void aaudio_div_four_to_mono_two(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (uint32_t i = 0; i < src_len>>1; i++)
    {
        dst_buf[i] = src_buf[i*4 + 1];
    }
}

void aaudio_div_four_to_mono_three(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (uint32_t i = 0; i < src_len>>1; i++)
    {
        dst_buf[i] = src_buf[i*4 + 2];
    }
}

void aaudio_div_four_to_mono_four(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (uint32_t i = 0; i < src_len>>1; i++)
    {
        dst_buf[i] = src_buf[i*4 + 3];
    }
}
#endif

#ifdef WL_LED_ZG_SWITCH
bool mute_switch=false;
uint8_t mute_key_state=0;

#define KEY_CNT_MIN      20
#define KEY_CNT_MAX     50

uint32_t vad_step_count_process(int vad_cnt)
{
    static uint32_t tmp_count = 0;
    static uint32_t last_count = 0;

    uint32_t ret = 0;

    if(vad_cnt)
    {
        tmp_count = 0;
    }
    else
    {
        tmp_count++;
    }
    
    if(tmp_count < last_count)
    {
        TRACE("tmp_count:%d last_count:%d \n\t",tmp_count,last_count);
        if((last_count > KEY_CNT_MIN) && (last_count < KEY_CNT_MAX))
        {
            ret = 1;
        }
    }

    last_count = tmp_count;

    return ret;
}

static void app_zg_mute_mode_switch(void)
{
    if(mute_switch)
    {
        mute_switch=false;
    }
    else
    {
        mute_switch=true;
    }
    
}

static bool app_zg_mute_mode_get(void)
{
    return mute_switch;
}
#endif



#if SPEECH_CODEC_CAPTURE_SAMPLE == 48000 || SPEECH_CODEC_CAPTURE_SAMPLE == 44100 || SPEECH_CODEC_CAPTURE_SAMPLE == 32000

static uint32_t app_high_data_come(uint8_t *buf, uint32_t len)
{

#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2    

    int32_t stime = 0;
    static int32_t nsx_cnt = 0;
    uint32_t pcm_len = len>>1;

    nsx_cnt++;
    //DUMP16("%d,",(short*)buf,30);
    if(false == (nsx_cnt & 0x3F))
    {
        stime = hal_sys_timer_get();
	    //TRACE("aecm  echo time: lens:%d  g_time_cnt:%d ",len, g_time_cnt);
    }


    aaudio_div_stero_to_lmono(one_buff,(int16_t*)buf,pcm_len);
    aaudio_div_stero_to_rmono(two_buff,(int16_t*)buf,pcm_len);

#ifdef GCC_PLAT

    static uint32_t gcc_plat_count = 0;

    if(gcc_plat_count > GCC_PLAT_START_THD)
    {
        uint32_t gcc_gain = gcc_plat_process(one_buff,two_buff,pcm_len>>1);

        for(uint32_t icnt = 0; icnt < pcm_len>>1; icnt++)
        {
            one_buff[icnt] = one_buff[icnt]>>gcc_gain;
        }
    }
    else
    {
        /* code */
        gcc_plat_count++;
        // for(uint32_t icnt = 0; icnt < pcm_len>>1; icnt++)
        // {
        //     one_buff[icnt] = 0;
        // }
    }
    
#endif

    //nsx denosie alg
#ifdef WL_NSX

    wl_nsx_16k_denoise(one_buff,two_buff);
    memcpy(one_buff,two_buff,pcm_len);

#endif

    //DUMP16("%5d, ",temp_buff,20);
#ifdef AUDIO_DEBUG
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, one_buff, pcm_len>>1);
    audio_dump_add_channel_data(1, one_buff, pcm_len>>1);	
    audio_dump_run();
#endif

    if(false == (nsx_cnt & 0x3F))
    {
        TRACE("mic 1 right agc 8 speed  time:%d ms and lens:%d freq:%d ", TICKS_TO_MS(hal_sys_timer_get() - stime), len,hal_sysfreq_get());
    }
    

    app_audio_pcmbuff_put((uint8_t*)one_buff, pcm_len);


#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 3    
    //uint32_t pcm_len = len>>1;
    uint32_t pcm_len = len/3;

    aaudio_div_three_to_mono_one(one_buff,(int16_t*)buf,pcm_len);
    aaudio_div_three_to_mono_two(two_buff,(int16_t*)buf,pcm_len);
    aaudio_div_three_to_mono_three(three_buff,(int16_t*)buf,pcm_len);
    DUMP16("%5d, ",two_buff,30);
    TRACE("one cap buff size :%d ",pcm_len);
    //DUMP16("%5d, ",temp_buff,20);
#ifdef AUDIO_DEBUG
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, one_buff, pcm_len>>1);
    audio_dump_add_channel_data(1, two_buff, pcm_len>>1);	
    audio_dump_add_channel_data(2, three_buff, pcm_len>>1);	

    audio_dump_run();
#endif

    app_audio_pcmbuff_put((uint8_t*)one_buff, pcm_len);

#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 4    
    //uint32_t pcm_len = len>>1;
    uint32_t pcm_len = len/4;

    aaudio_div_four_to_mono_one(one_buff,(int16_t*)buf,pcm_len);
    aaudio_div_four_to_mono_two(two_buff,(int16_t*)buf,pcm_len);
    aaudio_div_four_to_mono_three(three_buff,(int16_t*)buf,pcm_len);
    aaudio_div_four_to_mono_four(four_buff,(int16_t*)buf,pcm_len);
    DUMP16("%5d, ",three_buff,30);
    TRACE("one cap buff size :%d ",pcm_len);
    //DUMP16("%5d, ",temp_buff,20);
#ifdef AUDIO_DEBUG
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, one_buff, pcm_len>>1);
    //audio_dump_add_channel_data(0, two_buff, pcm_len>>1);	
    //audio_dump_add_channel_data(0, three_buff, pcm_len>>1);	

    audio_dump_run();
#endif

    app_audio_pcmbuff_put((uint8_t*)four_buff, pcm_len);

#else

    short *pcm_buff = (short*)buf;

    //DUMP16("%5d, ",pcm_buff,30);

    int32_t stime = 0;
    static int32_t nsx_cnt = 0;
    static int32_t dump_cnt = 0;

    uint32_t pcm_len = len>>1;

    nsx_cnt++;
    dump_cnt++;

    //DUMP16("%d,",(short*)buf,30);
    if(false == (nsx_cnt & 0x3F))
    {
        stime = hal_sys_timer_get();
	    //TRACE("aecm  echo time: lens:%d  g_time_cnt:%d ",len, g_time_cnt);
    }



// high speech sample start 
#if SPEECH_CODEC_CAPTURE_SAMPLE == 48000    

    WebRtcNsx_48k_denoise(pcm_buff,out_buff);
    memset(pcm_buff,0x0,len);
    memcpy(pcm_buff,out_buff,len);

#elif SPEECH_CODEC_CAPTURE_SAMPLE == 44100  

    WebRtcNsx_44k_denoise(pcm_buff,out_buff);
    memset(pcm_buff,0x0,len);
    memcpy(pcm_buff,out_buff,len);

#elif SPEECH_CODEC_CAPTURE_SAMPLE == 32000


    wl_nsx_32k_denoise_process(pcm_buff,out_buff);

    memset(pcm_buff,0x0,len);
    memcpy(pcm_buff,out_buff,len);
 #ifdef WL_LED_ZG_SWITCH
        if(app_zg_mute_mode_get())
        {
            memset(pcm_buff,0x0,len);
        }
#endif   

#ifdef AUDIO_DEBUG
    
    audio_dump_clear_up();

    audio_dump_add_channel_data(0, pcm_buff, pcm_len);

    audio_dump_run();
#endif

#else

#ifdef WL_NSX

    wl_nsx_16k_denoise(pcm_buff,out_buff);
    memset(pcm_buff,0x0,len);
    memcpy(pcm_buff,out_buff,len);

#endif

#endif //high speech sample end

#ifdef AUDIO_DEBUG
    
    #if 0
    if(dump_cnt > 0x3FF)
    {
        audio_dump_clear_up();

        // for(uint16_t iicnt = 0; iicnt < pcm_len; iicnt++)
        // {
        //     pcm_buff[iicnt] = 0;
        // }

        audio_dump_add_channel_data(0, pcm_buff, pcm_len);
        //audio_dump_add_channel_data(0, two_buff, pcm_len>>1);	
        //audio_dump_add_channel_data(0, three_buff, pcm_len>>1);	

        audio_dump_run();

        dump_cnt = 0x4FF;
    }

    #else
    audio_dump_clear_up();

    audio_dump_add_channel_data(0, pcm_buff, pcm_len);
    //audio_dump_add_channel_data(0, two_buff, pcm_len>>1);	
    //audio_dump_add_channel_data(0, three_buff, pcm_len>>1);	

    audio_dump_run();
    #endif

#endif

    app_audio_pcmbuff_put((uint8_t*)pcm_buff, len);



#ifdef WL_DEBUG_MODE
    if(nsx_cnt > 0x107AC0)
    {
        nv_record_update_ibrt_info_debug_mode();
        ASSERT(nsx_cnt == 0xff, "%s: wl_debug_mode raise crash ret=%d", __func__, nsx_cnt);
    }
#endif


    if(false == (nsx_cnt & 0x3F))
    {
        TRACE("high sample 32k no denoise agc 15 speed  time:%d ms and pcm_lens:%d freq:%d ", TICKS_TO_MS(hal_sys_timer_get() - stime), pcm_len,hal_sysfreq_get());
    }
    
 #ifdef WL_LED_ZG_SWITCH

    mute_key_state = vad_step_count_process(hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)app_wl_zg_mute_switch_cfg.pin));
    if(mute_key_state)
    {
        TRACE("mute %d",app_zg_mute_mode_get());

        app_zg_mute_mode_switch();
    }
    
    if(app_zg_mute_mode_get())
    {
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_hw_pinmux_pwl[0].pin,HAL_GPIO_DIR_OUT,1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_hw_pinmux_pwl[1].pin,HAL_GPIO_DIR_OUT,0);
    }
    else
    {
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_hw_pinmux_pwl[0].pin,HAL_GPIO_DIR_OUT,0);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_hw_pinmux_pwl[1].pin,HAL_GPIO_DIR_OUT,1);
    }
#endif   
#endif

    if (a2dp_cache_status == APP_AUDIO_CACHE_QTY){
        a2dp_cache_status = APP_AUDIO_CACHE_OK;
    }
    return len;
}

#else
//static uint32_t nsx_num = 0;

static uint32_t app_factorymode_data_come(uint8_t *buf, uint32_t len)
{

#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2    

    int32_t stime = 0;
    static int32_t nsx_cnt = 0;
    uint32_t pcm_len = len>>1;

    nsx_cnt++;
    //DUMP16("%d,",(short*)buf,30);
    if(false == (nsx_cnt & 0x3F))
    {
        stime = hal_sys_timer_get();
	    //TRACE("aecm  echo time: lens:%d  g_time_cnt:%d ",len, g_time_cnt);
    }


    aaudio_div_stero_to_lmono(one_buff,(int16_t*)buf,pcm_len);
    aaudio_div_stero_to_rmono(two_buff,(int16_t*)buf,pcm_len);


#ifdef GCC_PLAT

    static uint32_t gcc_plat_count = 0;

    if(gcc_plat_count > GCC_PLAT_START_THD)
    {
        uint32_t gcc_gain = gcc_plat_process(two_buff,one_buff,pcm_len>>1);

        for(uint32_t icnt = 0; icnt < pcm_len>>1; icnt++)
        {
            two_buff[icnt] = two_buff[icnt]>>gcc_gain;
        }
    }
    else
    {
        /* code */
        gcc_plat_count++;
        // for(uint32_t icnt = 0; icnt < pcm_len>>1; icnt++)
        // {
        //     one_buff[icnt] = 0;
        // }
    }
    
#endif

    //nsx denosie alg
#ifdef WL_NSX

    wl_nsx_16k_denoise(one_buff,two_buff);
    //memcpy(one_buff,two_buff,pcm_len);
#endif

    DUMP16("%5d, ",two_buff,20);

#ifdef AUDIO_DEBUG
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, one_buff, pcm_len>>1);
    audio_dump_add_channel_data(1, one_buff, pcm_len>>1);	
    audio_dump_run();
#endif

    if(false == (nsx_cnt & 0x3F))
    {
        TRACE("stero two mic2 right agc 14 speed  time:%d ms and lens:%d freq:%d ", TICKS_TO_MS(hal_sys_timer_get() - stime), len,hal_sysfreq_get());
    }
    

    app_audio_pcmbuff_put((uint8_t*)two_buff, pcm_len);


#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 3    
    //uint32_t pcm_len = len>>1;
    uint32_t pcm_len = len/3;

    aaudio_div_three_to_mono_one(one_buff,(int16_t*)buf,pcm_len);
    aaudio_div_three_to_mono_two(two_buff,(int16_t*)buf,pcm_len);
    aaudio_div_three_to_mono_three(three_buff,(int16_t*)buf,pcm_len);
    DUMP16("%5d, ",two_buff,30);
    TRACE("one cap buff size :%d ",pcm_len);
    //DUMP16("%5d, ",temp_buff,20);
#ifdef AUDIO_DEBUG
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, one_buff, pcm_len>>1);
    audio_dump_add_channel_data(1, two_buff, pcm_len>>1);	
    audio_dump_add_channel_data(2, three_buff, pcm_len>>1);	

    audio_dump_run();
#endif

    app_audio_pcmbuff_put((uint8_t*)one_buff, pcm_len);

#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 4    
    //uint32_t pcm_len = len>>1;
    uint32_t pcm_len = len/4;

    aaudio_div_four_to_mono_one(one_buff,(int16_t*)buf,pcm_len);
    aaudio_div_four_to_mono_two(two_buff,(int16_t*)buf,pcm_len);
    aaudio_div_four_to_mono_three(three_buff,(int16_t*)buf,pcm_len);
    aaudio_div_four_to_mono_four(four_buff,(int16_t*)buf,pcm_len);
    DUMP16("%5d, ",three_buff,30);
    TRACE("one cap buff size :%d ",pcm_len);
    //DUMP16("%5d, ",temp_buff,20);
#ifdef AUDIO_DEBUG
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, one_buff, pcm_len>>1);
    //audio_dump_add_channel_data(0, two_buff, pcm_len>>1);	
    //audio_dump_add_channel_data(0, three_buff, pcm_len>>1);	

    audio_dump_run();
#endif

    app_audio_pcmbuff_put((uint8_t*)four_buff, pcm_len);

#else

    short *pcm_buff = (short*)buf;

    //DUMP16("%5d, ",pcm_buff,30);

    int32_t stime = 0;
    static int32_t nsx_cnt = 0;
    static int32_t dump_cnt = 0;

    uint32_t pcm_len = len>>1;

    nsx_cnt++;
    dump_cnt++;

    //DUMP16("%d,",(short*)buf,30);
    if(false == (nsx_cnt & 0x3F))
    {
        stime = hal_sys_timer_get();
	    //TRACE("aecm  echo time: lens:%d  g_time_cnt:%d ",len, g_time_cnt);
    }

#ifdef WL_VAD
    uint32_t vad_state = 0;
    vad_state = wl_vad_process_frame(pcm_buff,pcm_len);\

    if(vad_state == ON)
    {
	    TRACE("vad_state is:%d  ",vad_state);
    }

#endif

#ifdef NOTCH_FILTER
    //DUMP16("%5d, ",pcm_buff,30);
    for(uint16_t icnt = 0; icnt < pcm_len; icnt++)
    {

        double yout = (double)notch_filter_process(pcm_buff[icnt]);
        pcm_buff[icnt] = (short)yout;
        //printf("yout:%f \n\t",yout);

    }
#endif


#ifdef WL_NSX

    wl_nsx_16k_denoise(pcm_buff,out_buff);
    //memset(pcm_buff,0x0,len);
    memcpy(pcm_buff,out_buff,len);

#ifdef WL_STEREO_AUDIO
    for(uint32_t inum = 0; inum<pcm_len;inum++)
    {
        revert_buff[inum] = -pcm_buff[inum];
    }
#endif

#endif

// #ifdef WL_VAD
//     //memset(pcm_buff,0x0,pcm_len*2);
//     wl_vad_process_frame(pcm_buff,pcm_len);
// #endif


#ifdef WL_FIR_FILTER
        double floatInput[pcm_len];
        double floatOutput[pcm_len];
        // convert to doubles
        intToFloat(pcm_buff, floatInput, pcm_len);
        // perform the filtering
        firFloat(fir_coeffs, floatInput, floatOutput, pcm_len,FILTER_LEN);
        // convert to ints
        floatToInt(floatOutput, pcm_buff, pcm_len);
#endif

#ifdef AUDIO_DEBUG
    
#ifdef WL_GPIO_SWITCH
    if(0 == hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)app_wl_nsx_switch_detecter_cfg.pin))
    {

        audio_dump_clear_up();
        audio_dump_add_channel_data(0, pcm_buff, pcm_len);
#ifdef WL_STEREO_AUDIO
        audio_dump_add_channel_data(1, revert_buff, pcm_len);
#endif
        audio_dump_run();

    }

#else
        audio_dump_clear_up();
        audio_dump_add_channel_data(0, pcm_buff, pcm_len);
        audio_dump_run();
#endif
#endif

    app_audio_pcmbuff_put((uint8_t*)pcm_buff, len);


    if(false == (nsx_cnt & 0x3F))
    {
        TRACE("mic2 nsx 2 agc 12 speed  time:%d ms and pcm_lens:%d ", TICKS_TO_MS(hal_sys_timer_get() - stime), pcm_len);
#ifdef WL_GPIO_SWITCH
        TRACE("nsx_gpio_pin_value:%d ", hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)app_wl_nsx_switch_detecter_cfg.pin));
#endif

    }
    
#endif

    if (a2dp_cache_status == APP_AUDIO_CACHE_QTY){
        a2dp_cache_status = APP_AUDIO_CACHE_OK;
    }
    return len;
}

#endif

static uint32_t app_factorymode_more_data(uint8_t *buf, uint32_t len)
{
    if (a2dp_cache_status != APP_AUDIO_CACHE_QTY){

#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2 && !defined(LINGJI_CODEC)  
        app_audio_pcmbuff_get((uint8_t *)buf, len);
#else
        app_audio_pcmbuff_get((uint8_t *)app_audioloop_play_cache, len/2);
        app_bt_stream_copy_track_one_to_two_16bits((int16_t *)buf, app_audioloop_play_cache, len/2/2);
#endif
    }
    return len;
}

int app_factorymode_audioloop(bool on, enum APP_SYSFREQ_FREQ_T freq)
{
    uint8_t *buff_play = NULL;
    uint8_t *buff_capture = NULL;
    uint8_t *buff_loop = NULL;
    struct AF_STREAM_CONFIG_T stream_cfg;
    static bool isRun =  false;
    APP_FACTORY_TRACE("app_factorymode_audioloop work:%d op:%d freq:%d", isRun, on, freq);

    if (isRun==on)
        return 0;

    if (on){
        if (freq < APP_SYSFREQ_52M) {
            freq = APP_SYSFREQ_52M;
        }


#ifdef GCC_PLAT
        gcc_plat_init();
#endif

#ifdef NOTCH_FILTER
        notch_filter_init();
#endif

        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq);

        a2dp_cache_status = APP_AUDIO_CACHE_QTY;
        app_audio_mempool_init();
#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2  
        app_audio_mempool_get_buff(&buff_capture, 2*BT_AUDIO_FACTORMODE_BUFF_SIZE);
#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 3
        app_audio_mempool_get_buff(&buff_capture, 3*BT_AUDIO_FACTORMODE_BUFF_SIZE);
#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 4
        app_audio_mempool_get_buff(&buff_capture, 4*BT_AUDIO_FACTORMODE_BUFF_SIZE);
#else
        app_audio_mempool_get_buff(&buff_capture, BT_AUDIO_FACTORMODE_BUFF_SIZE);

#endif

        app_audio_mempool_get_buff(&buff_play, BT_AUDIO_FACTORMODE_BUFF_SIZE*2);
        app_audio_mempool_get_buff((uint8_t **)&app_audioloop_play_cache, BT_AUDIO_FACTORMODE_BUFF_SIZE*2/2/2);
        app_audio_mempool_get_buff(&buff_loop, BT_AUDIO_FACTORMODE_BUFF_SIZE<<4);
        app_audio_pcmbuff_init(buff_loop, BT_AUDIO_FACTORMODE_BUFF_SIZE<<4);
        memset(&stream_cfg, 0, sizeof(stream_cfg));

// speech high start
#if SPEECH_CODEC_CAPTURE_SAMPLE == 48000    

        app_overlay_select(APP_OVERLAY_FM);
        uint8_t* nsx_heap;
        app_audio_mempool_get_buff(&nsx_heap, WEBRTC_NSX_BUFF_SIZE);
        wl_nsx_denoise_init(16000,1, nsx_heap);
        nsx_resample_init();

#elif SPEECH_CODEC_CAPTURE_SAMPLE == 44100  
        app_overlay_select(APP_OVERLAY_FM);
        uint8_t* nsx_heap;
        app_audio_mempool_get_buff(&nsx_heap, WEBRTC_NSX_BUFF_SIZE);
        wl_nsx_denoise_init(16000,1, nsx_heap);
        nsx_resample_init();
#elif SPEECH_CODEC_CAPTURE_SAMPLE == 32000  
        app_overlay_select(APP_OVERLAY_FM);
        uint8_t* nsx_heap;
        app_audio_mempool_get_buff(&nsx_heap, WEBRTC_NSX_BUFF_SIZE);
        wl_nsx_denoise_init(32000,2, nsx_heap);
        nsx_resample_init();

        #ifdef WEBRTC_AGC_32K
        wl_agc_32k_init();
        #endif

#else

#ifdef WL_NSX
        app_overlay_select(APP_OVERLAY_FM);
        uint8_t* nsx_heap;
        app_audio_mempool_get_buff(&nsx_heap, WEBRTC_NSX_BUFF_SIZE);
        wl_nsx_denoise_init(16000,2, nsx_heap);
#endif

#ifdef WEBRTC_AGC
        WebRtcAgc_init();
#endif

#endif // speech high end
 

#ifdef WL_VAD
        wl_vad_init(VAD_MODE);
#endif

#ifdef WL_FIR_FILTER
        firFloatInit();
#endif

        stream_cfg.bits = AUD_BITS_16;
#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2  
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 3  
        stream_cfg.channel_num = AUD_CHANNEL_NUM_3;
#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 4  
        stream_cfg.channel_num = AUD_CHANNEL_NUM_4;
#else
        stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
#endif

#ifdef AUDIO_DEBUG
#ifdef WL_STEREO_AUDIO
        audio_dump_init(BT_AUDIO_FACTORMODE_BUFF_SIZE>>2, sizeof(short), 2);
#else
        audio_dump_init(BT_AUDIO_FACTORMODE_BUFF_SIZE>>2, sizeof(short), 1);
#endif

#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_CAPTURE_RESAMPLE)
        stream_cfg.sample_rate = AUD_SAMPRATE_8463;
#else

#if SPEECH_CODEC_CAPTURE_SAMPLE == 48000    
        stream_cfg.sample_rate = AUD_SAMPRATE_48000;
#elif SPEECH_CODEC_CAPTURE_SAMPLE == 44100 
        stream_cfg.sample_rate = AUD_SAMPRATE_44100;
#elif SPEECH_CODEC_CAPTURE_SAMPLE == 32000 
        stream_cfg.sample_rate = AUD_SAMPRATE_32000;
#else
        stream_cfg.sample_rate = AUD_SAMPRATE_16000;
#endif

#endif //end resample


#if FPGA==0
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#endif
        stream_cfg.vol = CODEC_SADC_VOL;
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;

#if SPEECH_CODEC_CAPTURE_SAMPLE == 48000    

        stream_cfg.handler = app_high_data_come;

#elif SPEECH_CODEC_CAPTURE_SAMPLE == 44100

        stream_cfg.handler = app_high_data_come;

#elif SPEECH_CODEC_CAPTURE_SAMPLE == 32000

        stream_cfg.handler = app_high_data_come;

#else
        stream_cfg.handler = app_factorymode_data_come;

#endif


        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_capture);
        stream_cfg.data_size = BT_AUDIO_FACTORMODE_BUFF_SIZE*stream_cfg.channel_num;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = app_factorymode_more_data;

        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_play);
        stream_cfg.data_size = BT_AUDIO_FACTORMODE_BUFF_SIZE*2;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        APP_FACTORY_TRACE("app_factorymode_audioloop on");
    } else {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        APP_FACTORY_TRACE("app_factorymode_audioloop off");

        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    }

    isRun=on;
    return 0;
}


#ifdef APP_LINEIN_SOURCE

static short POSSIBLY_UNUSED left_out[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
static short POSSIBLY_UNUSED right_out[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];

static uint32_t app_linein_data_come(uint8_t *buf, uint32_t len)
{

    int32_t stime = 0;
    static int32_t nsx_cnt = 0;
    uint32_t pcm_len = len>>1;
    short POSSIBLY_UNUSED *pcm_buff = (short*)buf;

    nsx_cnt++;
    //DUMP16("%d,",(short*)buf,30);
    if(false == (nsx_cnt & 0x3F))
    {
        stime = hal_sys_timer_get();
	    //TRACE("aecm  echo time: lens:%d  g_time_cnt:%d ",len, g_time_cnt);
    }


    aaudio_div_stero_to_lmono(one_buff,(int16_t*)buf,pcm_len);
    aaudio_div_stero_to_rmono(two_buff,(int16_t*)buf,pcm_len);


    //nsx denosie alg
#ifdef WL_NSX

    for(uint32_t icnt = 0; icnt < pcm_len>>1; icnt++)
    {
        one_buff[icnt] = pcm_buff[icnt];
        two_buff[icnt] = pcm_buff[pcm_len/2 + icnt];
    }

    wl_nsx_16k_denoise(one_buff,left_out);
    wl_nsx_16k_denoise(two_buff,right_out);

    for(uint32_t icnt = 0; icnt < pcm_len>>1; icnt++)
    {
        pcm_buff[icnt] = left_out[icnt];
        pcm_buff[pcm_len/2 + icnt] = right_out[icnt];
    }
#endif

    //DUMP16("%5d, ",temp_buff,20);
#ifdef AUDIO_DEBUG
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, one_buff, pcm_len>>1);
    audio_dump_add_channel_data(1, one_buff, pcm_len>>1);	
    audio_dump_run();
#endif

    if(false == (nsx_cnt & 0x3F))
    {
        TRACE("linein nsx 1 agc 10 speed  time:%d ms and lens:%d freq:%d ", TICKS_TO_MS(hal_sys_timer_get() - stime), len,hal_sysfreq_get());
    }
    
    app_audio_pcmbuff_put((uint8_t*)pcm_buff, len);

    if (a2dp_cache_status == APP_AUDIO_CACHE_QTY){
        a2dp_cache_status = APP_AUDIO_CACHE_OK;
    }
    return len;
}


#define LINEIN_CAPTURE_CHANNEL (2)


static uint32_t app_linein_play_more_data(uint8_t *buf, uint32_t len)
{
    if (a2dp_cache_status != APP_AUDIO_CACHE_QTY){
        app_audio_pcmbuff_get((uint8_t *)buf, len);
    }
    return len;
}

int app_source_linein_loopback_test(bool on)
{
    uint8_t *buff_play = NULL;
    uint8_t *buff_capture = NULL;
    uint8_t *buff_loop = NULL;
    struct AF_STREAM_CONFIG_T stream_cfg;
    static bool isRun =  false;
    APP_FACTORY_TRACE("app_source_linein_loopback_test work:%d op:%d freq:%d", isRun, on);

    if (isRun==on)
        return 0;

    if (on){

#ifdef GCC_PLAT
        gcc_plat_init();
#endif

#ifdef NOTCH_FILTER
        notch_filter_init();
#endif

        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_208M);

        a2dp_cache_status = APP_AUDIO_CACHE_QTY;
        app_audio_mempool_init();

        app_audio_mempool_get_buff(&buff_capture, LINEIN_CAPTURE_CHANNEL*BT_AUDIO_FACTORMODE_BUFF_SIZE);

        app_audio_mempool_get_buff(&buff_play, BT_AUDIO_FACTORMODE_BUFF_SIZE*2);
        app_audio_mempool_get_buff((uint8_t **)&app_audioloop_play_cache, BT_AUDIO_FACTORMODE_BUFF_SIZE*2/2/2);
        app_audio_mempool_get_buff(&buff_loop, BT_AUDIO_FACTORMODE_BUFF_SIZE<<4);
        app_audio_pcmbuff_init(buff_loop, BT_AUDIO_FACTORMODE_BUFF_SIZE<<4);
        memset(&stream_cfg, 0, sizeof(stream_cfg));

// speech high start
#ifdef WL_NSX
        app_overlay_select(APP_OVERLAY_FM);
        uint8_t* nsx_heap;
        app_audio_mempool_get_buff(&nsx_heap, WEBRTC_NSX_BUFF_SIZE);
        wl_nsx_denoise_init(16000,2, nsx_heap);
#endif

#ifdef WEBRTC_AGC
        WebRtcAgc_init();
#endif
 

#ifdef WL_VAD
        wl_vad_init(VAD_MODE);
#endif

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;

#ifdef AUDIO_DEBUG
#ifdef WL_STEREO_AUDIO
        audio_dump_init(BT_AUDIO_FACTORMODE_BUFF_SIZE>>2, sizeof(short), 2);
#else
        audio_dump_init(BT_AUDIO_FACTORMODE_BUFF_SIZE>>2, sizeof(short), 1);
#endif

#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_CAPTURE_RESAMPLE)
        stream_cfg.sample_rate = AUD_SAMPRATE_8463;
#else
        stream_cfg.sample_rate = AUD_SAMPRATE_16000;
#endif //end resample


#if FPGA==0
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#endif
        stream_cfg.vol = CODEC_SADC_VOL;
        stream_cfg.io_path = AUD_INPUT_PATH_LINEIN;

        stream_cfg.handler = app_linein_data_come;


        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_capture);
        stream_cfg.data_size = BT_AUDIO_FACTORMODE_BUFF_SIZE*stream_cfg.channel_num;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = app_linein_play_more_data;

        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_play);
        stream_cfg.data_size = BT_AUDIO_FACTORMODE_BUFF_SIZE*2;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        APP_FACTORY_TRACE("app_factorymode_audioloop on");
    } else {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        APP_FACTORY_TRACE("app_factorymode_audioloop off");

        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    }

    isRun=on;
    return 0;
}

#endif

int app_factorymode_output_pcmpatten(audio_test_pcmpatten_t *pcmpatten, uint8_t *buf, uint32_t len)
{
    uint32_t remain_size = len;
    uint32_t curr_size = 0;

    if (remain_size > pcmpatten->len)
    {
        do{
            if (pcmpatten->cuur_buf_pos)
            {
                curr_size = pcmpatten->len-pcmpatten->cuur_buf_pos;
                memcpy(buf,&(pcmpatten->buf[pcmpatten->cuur_buf_pos/2]), curr_size);
                remain_size -= curr_size;
                pcmpatten->cuur_buf_pos = 0;
            }
            else if (remain_size>pcmpatten->len)
            {
                memcpy(buf+curr_size, pcmpatten->buf, pcmpatten->len);
                curr_size += pcmpatten->len;
                remain_size -= pcmpatten->len;
            }
            else
            {
                memcpy(buf+curr_size,pcmpatten->buf, remain_size);
                pcmpatten->cuur_buf_pos = remain_size;
                remain_size = 0;
            }
        }while(remain_size);
    }
    else
    {
        if ((pcmpatten->len - pcmpatten->cuur_buf_pos) >= len)
        {
            memcpy(buf, &(pcmpatten->buf[pcmpatten->cuur_buf_pos/2]),len);
            pcmpatten->cuur_buf_pos += len;
        }
        else 
        {
            curr_size = pcmpatten->len-pcmpatten->cuur_buf_pos;
            memcpy(buf, &(pcmpatten->buf[pcmpatten->cuur_buf_pos/2]),curr_size);
            pcmpatten->cuur_buf_pos = len - curr_size;
            memcpy(buf+curr_size, pcmpatten->buf, pcmpatten->cuur_buf_pos);                
        }
    }
    
    return 0;
}

#include "fft128dot.h"

#define N 64
#define NFFT 128

struct mic_st_t{
    FftTwiddle_t w[N];
    FftTwiddle_t w128[N*2];
    FftData_t x[N*2];
    FftData_t data_odd[N];
    FftData_t data_even[N];
    FftData_t data_odd_d[N];
    FftData_t data_even_d[N];
    FftData_t data[N*2];
    signed long out[N];
};

int app_factorymode_mic_cancellation_run(void * mic_st, signed short *inbuf, int sample)
{
    struct mic_st_t *st = (struct mic_st_t *)mic_st;
    int i,k,jj,ii;
    //int dataWidth = 16;     // input word format is 16 bit twos complement fractional format 1.15
    int twiddleWidth = 16;  // input word format is 16 bit twos complement fractional format 2.14
    FftMode_t ifft = FFT_MODE;

    make_symmetric_twiddles(st->w,N,twiddleWidth);
    make_symmetric_twiddles(st->w128,N*2,twiddleWidth);
    // input data
    for (i=0; i<sample; i++){
        st->x[i].re = inbuf[i];
        st->x[i].im = 0;
    }

    for(ii = 0; ii < 1; ii++)
    {
      k = 0;
        for (jj = 0; jj < N*2; jj+=2)
        {
            FftData_t tmp;

            tmp.re     = st->x[jj].re;
            tmp.im     = st->x[jj].im;

              st->data_even[k].re = tmp.re;//(int) (double(tmp.re)*double(1 << FFTR4_INPUT_FORMAT_Y)) ;
              st->data_even[k].im = tmp.im;//(int) (double(tmp.im)*double(1 << FFTR4_INPUT_FORMAT_Y)) ;
              tmp.re     = st->x[jj+1].re;
              tmp.im     = st->x[jj+1].im;
              st->data_odd[k].re  = tmp.re;//(int) (double(tmp.re)*double(1 << FFTR4_INPUT_FORMAT_Y)) ;
              st->data_odd[k].im  = tmp.im;//(int) (double(tmp.im)*double(1 << FFTR4_INPUT_FORMAT_Y)) ;
              k++;
        }

        fftr4(NFFT/2, st->data_even, st->w, FFTR4_TWIDDLE_WIDTH, FFTR4_DATA_WIDTH, ifft);
        fftr4(NFFT/2, st->data_odd, st->w, FFTR4_TWIDDLE_WIDTH, FFTR4_DATA_WIDTH, ifft);

        for (jj = 0; jj < NFFT/2; jj++)
        {

        int idx = dibit_reverse_int(jj, NFFT/2);
            st->data_even_d[jj].re = st->data_even[idx].re;
            st->data_even_d[jj].im = st->data_even[idx].im;
            st->data_odd_d[jj].re  = st->data_odd[idx].re;
            st->data_odd_d[jj].im  = st->data_odd[idx].im;
        }
        for (jj=0;jj<NFFT/2;jj++)
        {
          long long mbr,mbi;
          FftData_t ta;
          FftData_t tmp;
          double a;
      mbr = (long long)(st->data_odd_d[jj].re) * st->w128[jj].re - (long long)(st->data_odd_d[jj].im) * st->w128[jj].im;
      mbi = (long long)(st->data_odd_d[jj].im) * st->w128[jj].re + (long long)(st->data_odd_d[jj].re) * st->w128[jj].im;
      ta.re = int(mbr>>(FFTR4_TWIDDLE_WIDTH-2));
      ta.im = int(mbi>>(FFTR4_TWIDDLE_WIDTH-2));
      st->data[jj].re = (st->data_even_d[jj].re + ta.re)/2;
      st->data[jj].im = (st->data_even_d[jj].im + ta.im)/2;
      //data[jj] = sat(data[jj],FFTR4_DATA_WIDTH);
      st->data[jj+NFFT/2].re = (st->data_even_d[jj].re - ta.re)/2;
      st->data[jj+NFFT/2].im = (st->data_even_d[jj].im - ta.im)/2;
      //data[jj+NFFT/2] = sat(data[jj+NFFT/2],FFTR4_DATA_WIDTH);

      a = st->data[jj].re;///double(1 << FFTR4_OUTPUT_FORMAT_Y);// * double(1 << FFTR4_SCALE);
          tmp.re              = (int)a;
          a = st->data[jj].im;///double(1 << FFTR4_OUTPUT_FORMAT_Y);// * double(1 << FFTR4_SCALE);
          tmp.im              = (int)a;
            st->x[ii*NFFT+jj].re = (int) tmp.re;
            st->x[ii*NFFT+jj].im = (int) tmp.im;
      a = st->data[jj+NFFT/2].re;///double(1 << FFTR4_OUTPUT_FORMAT_Y);// * double(1 << FFTR4_SCALE);
          tmp.re              = (int)a;
          a = st->data[jj+NFFT/2].im;///double(1 << FFTR4_OUTPUT_FORMAT_Y);// * double(1 << FFTR4_SCALE);
          tmp.im              = (int)a;
          st->x[ii*NFFT+jj+NFFT/2].re = (int) tmp.re;
            st->x[ii*NFFT+jj+NFFT/2].im = (int) tmp.im;
        }
    }

    for (i=0; i<N; i++){
        st->out[i] = st->x[i].re * st->x[i].re +  st->x[i].im * st->x[i].im;
    }

    return 0;

}

void *app_factorymode_mic_cancellation_init(void* (* alloc_ext)(int))
{
    struct mic_st_t *mic_st;
    mic_st = (struct mic_st_t *)alloc_ext(sizeof(struct mic_st_t));
    return (void *)mic_st;
}

#endif


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
#include "string.h"

// for audio
#include "audioflinger.h"
#include "app_audio.h"
#include "app_utils.h"
#include "hal_timer.h"

#include "app_mic_32k.h"
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


#ifdef MIC_32K_LOOPBACK

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

static short POSSIBLY_UNUSED out_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];


#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2    

static short one_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
static short two_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];

static short POSSIBLY_UNUSED left_out[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
static short POSSIBLY_UNUSED right_out[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];

static void POSSIBLY_UNUSED aaudio_div_stero_to_rmono(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (uint32_t i = 0; i < src_len>>1; i++)
    {
        dst_buf[i] = src_buf[i*2 + 1];
    }
}

static void POSSIBLY_UNUSED aaudio_div_stero_to_lmono(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (uint32_t i = 0; i < src_len>>1; i++)
    {
        dst_buf[i] = src_buf[i*2 + 0];
    }
}

static void POSSIBLY_UNUSED audio_mono2stereo_16bits(int16_t *dst_buf, int16_t *left_buf, int16_t *right_buf, uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; ++i) {
        dst_buf[i*2 + 0] = left_buf[i];
        dst_buf[i*2 + 1] = right_buf[i];
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

static uint32_t app_mic_32k_data_come(uint8_t *buf, uint32_t len)
{

#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2    

    int32_t stime = 0;
    static int32_t nsx_cnt = 0;
    uint32_t pcm_len = len>>1;
    short *pcm_buff = (short*)buf;


    nsx_cnt++;
    //DUMP16("%d,",(short*)buf,30);
    if(false == (nsx_cnt & 0x3F))
    {
        stime = hal_sys_timer_get();
	    //TRACE("aecm  echo time: lens:%d  g_time_cnt:%d ",len, g_time_cnt);
    }


    // aaudio_div_stero_to_lmono(one_buff,(int16_t*)buf,pcm_len);
    // aaudio_div_stero_to_rmono(two_buff,(int16_t*)buf,pcm_len);

    //nsx denosie alg

#ifdef WL_NSX
    for(uint32_t icnt = 0; icnt < pcm_len>>1; icnt++)
    {
        one_buff[icnt] = pcm_buff[icnt];
        two_buff[icnt] = pcm_buff[pcm_len/2 + icnt];
    }

    wl_nsx_32k_denoise_process(one_buff,left_out);
    wl_nsx_32k_denoise_process(two_buff,right_out);

    for(uint32_t icnt = 0; icnt < pcm_len>>1; icnt++)
    {
        pcm_buff[icnt] = one_buff[icnt];
        pcm_buff[pcm_len/2 + icnt] = two_buff[icnt];
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
        TRACE("32k stereo agc 8 speed a time:%d ms and lens:%d freq:%d ", TICKS_TO_MS(hal_sys_timer_get() - stime), len,hal_sysfreq_get());
    }
    

    app_audio_pcmbuff_put((uint8_t*)pcm_buff, 2*pcm_len);


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

    //DUMP16("%d,",(short*)buf,30);

    wl_nsx_32k_denoise_process(pcm_buff,out_buff);

    memset(pcm_buff,0x0,len);
    memcpy(pcm_buff,out_buff,len);
 #ifdef WL_LED_ZG_SWITCH
        if(app_zg_mute_mode_get())
        {
            memset(pcm_buff,0x0,len);
        }
#endif   

#endif //high speech sample end

#ifdef AUDIO_DEBUG
    
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, pcm_buff, pcm_len);
    audio_dump_run();

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
        TRACE("mic 32k nsx 3 agc 15 speed  time:%d ms and pcm_lens:%d freq:%d ", TICKS_TO_MS(hal_sys_timer_get() - stime), pcm_len,hal_sysfreq_get());
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


void app_bt_stream_copy_track_one_to_stero_16bits(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (int i = (int)(src_len - 1); i >= 0; i--)
    {
        dst_buf[i*2 + 0] = src_buf[i];
        dst_buf[i*2 + 1] = -src_buf[i];
    }
}


static uint32_t app_mic_32k_playback_data(uint8_t *buf, uint32_t len)
{
    if (a2dp_cache_status != APP_AUDIO_CACHE_QTY){
#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2    

        app_audio_pcmbuff_get((uint8_t *)buf, len);
#else
        app_audio_pcmbuff_get((uint8_t *)app_audioloop_play_cache, len/2);
        app_bt_stream_copy_track_one_to_stero_16bits((int16_t *)buf, app_audioloop_play_cache, len/2/2);
#endif
    }
    return len;
}

int app_mic_32k_audioloop(bool on, enum APP_SYSFREQ_FREQ_T freq)
{
    uint8_t *buff_play = NULL;
    uint8_t *buff_capture = NULL;
    uint8_t *buff_loop = NULL;
    struct AF_STREAM_CONFIG_T stream_cfg;
    static bool isRun =  false;
    TRACE("app_mic_32k loopback work:%d op:%d freq:%d", isRun, on, freq);

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
        AutoWah_init(20000,16000,420,380,1,0.662,10);
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
        wl_nsx_denoise_init(32000,3, nsx_heap);
        nsx_resample_init();

        #ifdef WEBRTC_AGC_32K
        wl_agc_32k_init();
        #endif

#endif // speech high end
 

#ifdef WL_VAD
        wl_vad_init(VAD_MODE);
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

        stream_cfg.handler = app_mic_32k_data_come;

#endif

        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_capture);
        stream_cfg.data_size = BT_AUDIO_FACTORMODE_BUFF_SIZE*stream_cfg.channel_num;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = app_mic_32k_playback_data;

        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_play);
        stream_cfg.data_size = BT_AUDIO_FACTORMODE_BUFF_SIZE*2;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        TRACE("app_mic_32k audioloop on");
    } else {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        TRACE("app_mic_32k audioloop off");

        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    }

    isRun=on;
    return 0;
}

#endif


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
//#include "app_factory.h"
#include "string.h"

// for audio
#include "audioflinger.h"
#include "app_audio.h"
#include "app_utils.h"
#include "hal_timer.h"
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

#if defined(WL_AEC)
#include "wl_sco_process.h"
#endif
#include "speech_memory.h"


#ifdef DUAL_MIC_NSX

#include "gcc_phat_fixed.h"

#include "data.h"
#define NUM_CHANNEL 2


#define BT_AUDIO_FACTORMODE_BUFF_SIZE    	(1024*2)




#define NSX_FRAME_SIZE 160


static enum APP_AUDIO_CACHE_T a2dp_cache_status = APP_AUDIO_CACHE_QTY;
static int16_t *app_audioloop_play_cache = NULL;


#ifdef WL_STEREO_AUDIO
static short revert_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
#endif

static short POSSIBLY_UNUSED out_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];

//static short revert_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
//static short audio_uart_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];


#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2    

static short POSSIBLY_UNUSED  one_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
static short POSSIBLY_UNUSED  two_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];

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

#endif


static void DelaySequence(float *in_data, int num, int delay, float *out_data) {
    for (int i = 0; i < num; i++) {
        if (i - delay >= 0 && i - delay < num) {
            out_data[i] = in_data[i - delay];
        }
        else {
            out_data[i] = 0;
        }
    }   
}

#ifdef GCC_PHAT_FIXED
float delay_data1[NUM_DATA];
float delay_data2[NUM_DATA];
float all_data[NUM_DATA*NUM_CHANNEL];
#endif


static uint32_t dual_mic_nsx_data_come(uint8_t *buf, uint32_t len)
{

#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2    
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

#ifdef GCC_PLAT


#ifdef STEREO_MIC

        uint32_t plat_val = gcc_plat_get(two_buff,one_buff,pcm_len>>1);

        if(plat_val > 1000)
        {   
            for(uint32_t inum = 0; inum < pcm_len>>1; inum++)
            {
                one_buff[inum] = two_buff[inum];
            }  
        }

#else
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
    }

#endif
#endif


#ifdef GCC_PHAT_FIXED
    static int ret = 0;
    DelaySequence(g_data, NUM_DATA, -50, delay_data1);
    DelaySequence(g_data, NUM_DATA, -5, delay_data2);
    
    // memcpy(all_data, g_data, sizeof(double) * NUM_DATA);
    for (int icnt = 0; icnt < NUM_DATA; icnt++)
    {
        /* code */
        all_data[icnt] = g_data[icnt];
        all_data[NUM_DATA+icnt] = delay_data1[icnt];

    }
    
    // memcpy(all_data + NUM_DATA, delay_data1, sizeof(double) * NUM_DATA);
    //memcpy(all_data + 2 * NUM_DATA, delay_data2, sizeof(double) * NUM_DATA);

    int tdoa[NUM_CHANNEL] = {0};
    GccPhatTdoa(all_data, NUM_CHANNEL, NUM_DATA, 0, NUM_DATA/2, tdoa);

    if(ret == 0)
    {
        for (int i = 0; i < NUM_CHANNEL; i++) {
            TRACE("%d\n", tdoa[i]);
        }

        ret = 1;
    }
  
#endif

#ifdef WL_NSX
    //wl_nsx_16k_denoise(one_buff,left_out);
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
        TRACE("dual_mic_nsx speed  time:%d ms and lens:%d freq:%d ", TICKS_TO_MS(hal_sys_timer_get() - stime), len,hal_sysfreq_get());
    }
    
#if defined(STEREO_MIC)
    app_audio_pcmbuff_put((uint8_t*)left_out, 1*pcm_len);
#else
    app_audio_pcmbuff_put((uint8_t*)pcm_buff, 2*pcm_len);
#endif

#endif

    if (a2dp_cache_status == APP_AUDIO_CACHE_QTY){
        a2dp_cache_status = APP_AUDIO_CACHE_OK;
    }
    return len;
}

static uint32_t dual_mic_nsx_playback_data(uint8_t *buf, uint32_t len)
{
    if (a2dp_cache_status != APP_AUDIO_CACHE_QTY){
#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2    
        #if defined(WL_AEC) || defined(STEREO_MIC)
        app_audio_pcmbuff_get((uint8_t *)app_audioloop_play_cache, len/2);
        app_bt_stream_copy_track_one_to_two_16bits((int16_t *)buf, app_audioloop_play_cache, len/2/2);
        #else
        app_audio_pcmbuff_get((uint8_t *)buf, len);
        #endif
#else
        app_audio_pcmbuff_get((uint8_t *)app_audioloop_play_cache, len/2);
        app_bt_stream_copy_track_one_to_two_16bits((int16_t *)buf, app_audioloop_play_cache, len/2/2);
#endif
    }
    return len;
}

int dual_mic_nsx_audioloop(bool on, enum APP_SYSFREQ_FREQ_T freq)
{
    uint8_t *buff_play = NULL;
    uint8_t *buff_capture = NULL;
    uint8_t *buff_loop = NULL;
    struct AF_STREAM_CONFIG_T stream_cfg;
    static bool isRun =  false;

    TRACE("app_mic_16k_audioloop work:%d op:%d freq:%d", isRun, on, freq);

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
#endif

        app_audio_mempool_get_buff(&buff_play, BT_AUDIO_FACTORMODE_BUFF_SIZE*2);
        app_audio_mempool_get_buff((uint8_t **)&app_audioloop_play_cache, BT_AUDIO_FACTORMODE_BUFF_SIZE*2/2/2);
        app_audio_mempool_get_buff(&buff_loop, BT_AUDIO_FACTORMODE_BUFF_SIZE<<4);
        app_audio_pcmbuff_init(buff_loop, BT_AUDIO_FACTORMODE_BUFF_SIZE<<4);
        memset(&stream_cfg, 0, sizeof(stream_cfg));

// speech nsx start
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

#ifdef DUAL_MIC_NSX
        uint8_t POSSIBLY_UNUSED *speech_buf = NULL;
        int POSSIBLY_UNUSED speech_len = 0;

        speech_len = app_audio_mempool_free_buff_size() - 1024*4;
        app_audio_mempool_get_buff(&speech_buf, speech_len);
        speech_heap_init(speech_buf, speech_len);

        uint8_t POSSIBLY_UNUSED *hp_test = NULL;
        hp_test = (uint8_t*)speech_malloc(320);
        //double *need_size = (double*)hp_test;
        //need_size[1] = 0.12345;
        TRACE("sizeof float:%d sizeof double:%d ",sizeof(float),sizeof(double));
#endif

        stream_cfg.bits = AUD_BITS_16;
#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2  
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
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
        stream_cfg.sample_rate = AUD_SAMPRATE_16000;

#endif //end resample


#if FPGA==0
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#endif
        stream_cfg.vol = CODEC_SADC_VOL;
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;

        stream_cfg.handler = dual_mic_nsx_data_come;

        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_capture);
        stream_cfg.data_size = BT_AUDIO_FACTORMODE_BUFF_SIZE*stream_cfg.channel_num;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = dual_mic_nsx_playback_data;

        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_play);
        stream_cfg.data_size = BT_AUDIO_FACTORMODE_BUFF_SIZE*2;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        TRACE("app_mic_16k loopback on");
    } else {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        TRACE("app_mic_16k loopback off");

        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    }

    isRun=on;
    return 0;
}

#endif

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
#include <stdio.h>
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "analog.h"
#include "app.h"
#include "bt_drv.h"
#include "app_audio.h"
#include "bluetooth.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"
#include "hal_location.h"


#if defined(A2DP_LHDC_ON)
//#include "../liblhdc-dec/lhdcUtil.h"
#include "lhdcUtil.h"
#endif

extern "C" {
#if defined(A2DP_LDAC_ON)
#include"ldacBT.h"
#endif
}

#include "a2dp_api.h"
#include "besbt.h"

#include "cqueue.h"
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "resources.h"
#include "app_bt_media_manager.h"
#include "tgt_hardware.h"
#include "bt_drv_interface.h"
#include "hci_api.h"

#ifdef BT_USB_AUDIO_DUAL_MODE
#include "btusb_audio.h"
#endif

#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
#include "app_a2dp_source.h"
extern enum AUD_SAMPRATE_T bt_parse_sbc_sample_rate(uint8_t sbc_samp_rate);
#endif

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#ifdef VOICE_DATAPATH
#include "app_voicepath.h"
#endif
#ifdef __AI_VOICE__
#include "ai_thread.h"
#endif
#define _FILE_TAG_ "A2DP"
#include "color_log.h"
#include "app_bt_func.h"
#include "os_api.h"

#if (A2DP_DECODER_VER >= 2)
#include "a2dp_decoder.h"
#endif

#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_tws_ibrt_cmd_sync_a2dp_status.h"
#endif

#define APP_A2DP_STRM_FLAG_QUERY_CODEC  0x08

#define APP_A2DP__DEBUG

#ifdef  APP_A2DP__DEBUG
#define APP_A2DP_TRACE(str, ...)   TRACE(str, ##__VA_ARGS__)
#else
#define APP_A2DP_TRACE(str, ...)
#endif


typedef uint8_t tx_done_flag;

#define TX_DONE_FLAG_INIT           ((uint8_t)0)
#define TX_DONE_FLAG_SUCCESS        ((uint8_t)1)
#define TX_DONE_FLAG_FAIL           ((uint8_t)2)
#define TX_DONE_FLAG_TXING          ((uint8_t)3)
tx_done_flag vol_ctrl_done_flag[BT_DEVICE_NUM] = {TX_DONE_FLAG_INIT};





int a2dp_volume_get(enum BT_DEVICE_ID_T id);

static int a2dp_volume_set(enum BT_DEVICE_ID_T id, uint8_t vol);
#ifdef _GFPS_
extern "C" void app_exit_fastpairing_mode(void);
#endif
extern int app_bt_stream_volumeset(int8_t vol);

#ifdef __A2DP_AVDTP_CP__
AvdtpContentProt a2dp_avdtpCp[BT_DEVICE_NUM];
U8 a2dp_avdtpCp_securityData[AVDTP_MAX_CP_VALUE_SIZE]=
{
};
#endif


#ifdef GSOUND_ENABLED
extern osTimerId sppserver_delay_timer;
#endif


static void app_AVRCP_sendCustomCmdRsp(uint8_t device_id, btif_avrcp_channel_t *chnl, uint8_t isAccept, uint8_t transId);
static void app_AVRCP_CustomCmd_Received(uint8_t* ptrData, uint32_t len);

void get_value1_pos(U8 mask,U8 *start_pos, U8 *end_pos)
{
    U8 num = 0;

    for(U8 i=0;i<8;i++){
        if((0x01<<i) & mask){
            *start_pos = i;//start_pos,end_pos stands for the start and end position of value 1 in mask
            break;
        }
    }
    for(U8 i=0;i<8;i++){
        if((0x01<<i) & mask)
            num++;//number of value1 in mask
    }
    *end_pos = *start_pos + num - 1;
}
U8 get_valid_bit(U8 elements, U8 mask)
{
    U8 start_pos,end_pos;

    get_value1_pos(mask,&start_pos,&end_pos);
//    TRACE("!!!start_pos:%d,end_pos:%d\n",start_pos,end_pos);
    for(U8 i = start_pos; i <= end_pos; i++){
        if((0x01<<i) & elements){
            elements = ((0x01<<i) | (elements & (~mask)));
            break;
        }
    }
    return elements;
}


struct BT_DEVICE_T  app_bt_device;
uint8_t a2dp_channel_num[BT_DEVICE_NUM];

#ifdef BTIF_AVRCP_ADVANCED_CONTROLLER

void a2dp_init(void)
{
     btif_a2dp_init();
     for(int i =0; i< BT_DEVICE_NUM; i++ ){
         app_bt_device.a2dp_stream[i] = btif_a2dp_alloc_stream();
#if defined(A2DP_LHDC_ON)
     app_bt_device.a2dp_lhdc_stream[i]  = btif_a2dp_alloc_stream();
#endif
#if defined(A2DP_SCALABLE_ON)
     app_bt_device.a2dp_scalable_stream[i]  = btif_a2dp_alloc_stream();
#endif
#if defined(ALL_USE_OPUS)
     app_bt_device.a2dp_aac_stream[i] = btif_a2dp_alloc_stream();
#endif
#if defined(A2DP_AAC_ON)
     app_bt_device.a2dp_aac_stream[i] = btif_a2dp_alloc_stream();
#endif
#if defined(A2DP_LDAC_ON)
    app_bt_device.a2dp_ldac_stream[i]  = btif_a2dp_alloc_stream();
#endif

     }

#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);
    app_bt_device.src_or_snk=nvrecord_env->src_snk_flag.src_snk_mode;
//  app_bt_device.src_or_snk=BT_DEVICE_SRC;
//  app_bt_device.src_or_snk=BT_DEVICE_SNK;
    TRACE("src_or_snk:%d\n",app_bt_device.src_or_snk);
    app_bt_device.input_onoff=0;
#endif

    for(uint8_t i=0; i<BT_DEVICE_NUM; i++)
    {
        a2dp_channel_num[i] = BTIF_SBC_CHNL_MODE_STEREO;
        app_bt_device.a2dp_state[i]=0;
        app_bt_device.a2dp_streamming[i] = 0;
        app_bt_device.avrcp_get_capabilities_rsp[i] = NULL;
        app_bt_device.avrcp_control_rsp[i] = NULL;
        app_bt_device.avrcp_notify_rsp[i] = NULL;
        app_bt_device.avrcp_cmd1[i] = NULL;
        app_bt_device.avrcp_cmd2[i] = NULL;
        app_bt_device.a2dp_connected_stream[i] = NULL;
#ifdef __A2DP_AVDTP_CP__
        app_bt_device.avdtp_cp[i]=0;
#endif

        app_bt_device.latestPausedDevId = BT_DEVICE_ID_1;
    }

    btif_app_a2dp_avrcpadvancedpdu_mempool_init();

    app_bt_device.a2dp_state[BT_DEVICE_ID_1]=0;
    app_bt_device.a2dp_play_pause_flag = 0;
    app_bt_device.curr_a2dp_stream_id= BT_DEVICE_ID_1;

    app_bt_device.callSetupBitRec = 0;
}

static bool a2dp_bdaddr_from_id(uint8_t id, bt_bdaddr_t *bd_addr) {
  btif_remote_device_t *remDev = NULL;
  ASSERT(id < BT_DEVICE_NUM, "invalid bt device id");
  if(NULL != bd_addr) {
    remDev = btif_a2dp_get_remote_device(
        app_bt_device.a2dp_connected_stream[id]);
    memset(bd_addr, 0, sizeof(bt_bdaddr_t));
    if (NULL != remDev) {
      memcpy(bd_addr, btif_me_get_remote_device_bdaddr(remDev), sizeof(bt_bdaddr_t));
      return true;
    }
  }
  return false;
}

static bool a2dp_bdaddr_cmp(bt_bdaddr_t *bd_addr_1, bt_bdaddr_t *bd_addr_2) {
  if((NULL == bd_addr_1) || (NULL == bd_addr_2)) {
    return false;
  }
  return (memcmp(bd_addr_1->address,
      bd_addr_2->address, BTIF_BD_ADDR_SIZE) == 0);
}

bool a2dp_id_from_bdaddr(bt_bdaddr_t *bd_addr, uint8_t *id)
{
  bt_bdaddr_t curr_addr = {0};
  uint8_t curr_id = BT_DEVICE_NUM;

  if (app_bt_is_device_connected(BT_DEVICE_ID_1))
  {
      a2dp_bdaddr_from_id(BT_DEVICE_ID_1, &curr_addr);
      if(a2dp_bdaddr_cmp(&curr_addr, bd_addr)) {
        curr_id = BT_DEVICE_ID_1;
      }
  }

#ifdef __BT_ONE_BRING_TWO__
  if (app_bt_is_device_connected(BT_DEVICE_ID_2))
  {
    a2dp_bdaddr_from_id(BT_DEVICE_ID_2, &curr_addr);
    if(a2dp_bdaddr_cmp(&curr_addr, bd_addr)) {
        curr_id = BT_DEVICE_ID_2;
    }
  }
#endif
  if(id) {
    *id = curr_id;
  }
  return (curr_id < BT_DEVICE_NUM);
}

#ifdef __BT_ONE_BRING_TWO__
#define APP_BT_PAUSE_MEDIA_PLAYER_DELAY  300
osTimerId   app_bt_pause_media_player_delay_timer_id = NULL;
extern uint8_t avrcp_palyback_status[BT_DEVICE_NUM];
static uint8_t deviceIdPendingForMediaPlayerPause = 0;
static uint8_t deviceIdInMediaPlayHoldState = BT_DEVICE_NUM;
static void app_bt_pause_media_player_delay_timer_handler(void const *n);
osTimerDef (APP_BT_PAUSE_MEDIA_PLAYER_DELAY_TIMER, app_bt_pause_media_player_delay_timer_handler);

static void app_bt_pause_media_player_delay_timer_handler(void const *n)
{
    app_bt_start_custom_function_in_bt_thread(deviceIdPendingForMediaPlayerPause, 0, (uint32_t)app_bt_pause_music_player);
}

void app_bt_pause_media_player_again(uint8_t deviceId)
{
    if (NULL == app_bt_pause_media_player_delay_timer_id)
    {
        app_bt_pause_media_player_delay_timer_id = osTimerCreate(
            osTimer(APP_BT_PAUSE_MEDIA_PLAYER_DELAY_TIMER), osTimerOnce, NULL);
    }

    TRACE("The media player on device %d is resumed before it's allowed, so pause it again.",
        deviceId);

    deviceIdPendingForMediaPlayerPause = deviceId;
    osTimerStart(app_bt_pause_media_player_delay_timer_id,
        APP_BT_PAUSE_MEDIA_PLAYER_DELAY);
}

bool app_bt_is_to_resume_music_player(uint8_t deviceId)
{
    return (deviceIdInMediaPlayHoldState == deviceId);
}

void app_bt_reset_music_player_resume_state(void)
{
    deviceIdInMediaPlayHoldState = BT_DEVICE_NUM;
}

void app_bt_set_music_player_resume_device(uint8_t deviceId)
{
    deviceIdInMediaPlayHoldState = deviceId;
}

bool app_bt_is_music_player_working(uint8_t deviceId)
{
    TRACE("device %d a2dp streaming %d playback state %d",
        deviceId, app_bt_is_a2dp_streaming(deviceId),
        avrcp_palyback_status[deviceId]);
    return (app_bt_is_a2dp_streaming(deviceId) &&
        avrcp_palyback_status[deviceId]);
}

void app_bt_suspend_a2dp_streaming(uint8_t deviceId)
{
    if (!app_bt_is_a2dp_streaming(deviceId))
    {
        return;
    }

    TRACE("Suspend a2dp streaming of device %d", deviceId);
    btif_a2dp_suspend_stream(app_bt_device.a2dp_connected_stream[deviceId]);
}



bool app_bt_pause_music_player(uint8_t deviceId)
{
    if (!app_bt_is_music_player_working(deviceId))
    {
        return false;
    }

    btif_avrcp_channel_t *avrcp_channel_tmp =
        app_bt_device.avrcp_channel[deviceId];

    TRACE("Pause music player of device %d", deviceId);
    app_bt_suspend_a2dp_streaming(deviceId);

    btif_avrcp_set_panel_key(avrcp_channel_tmp,BTIF_AVRCP_POP_PAUSE,TRUE);
    btif_avrcp_set_panel_key(avrcp_channel_tmp,BTIF_AVRCP_POP_PAUSE,FALSE);

    app_bt_device.a2dp_play_pause_flag = 0;

    return true;
}

void app_bt_resume_music_player(uint8_t deviceId)
{
    app_bt_reset_music_player_resume_state();

    if (app_bt_is_music_player_working(deviceId))
    {
        return;
    }

    btif_avrcp_channel_t *avrcp_channel_tmp =
        app_bt_device.avrcp_channel[deviceId];

    TRACE("Resume music player of device %d", deviceId);
    btif_avrcp_set_panel_key(avrcp_channel_tmp,BTIF_AVRCP_POP_PLAY,TRUE);
    btif_avrcp_set_panel_key(avrcp_channel_tmp,BTIF_AVRCP_POP_PLAY,FALSE);
    app_bt_device.a2dp_play_pause_flag = 1;
}
#endif

bool app_bt_is_a2dp_streaming(uint8_t deviceId)
{
    return app_bt_device.a2dp_streamming[deviceId];
}

FRAM_TEXT_LOC uint8_t bt_sbc_player_get_codec_type(void)
{
    enum BT_DEVICE_ID_T st_id = app_bt_device.curr_a2dp_stream_id;
    if(app_bt_device.a2dp_connected_stream[st_id])
        return   btif_a2dp_get_stream_codec(app_bt_device.a2dp_connected_stream[st_id])->codecType;
    else
        return BTIF_AVDTP_CODEC_TYPE_SBC;
}

FRAM_TEXT_LOC uint8_t bt_sbc_player_get_sample_bit(void) {
    enum BT_DEVICE_ID_T st_id = app_bt_device.curr_a2dp_stream_id;

    return app_bt_device.sample_bit[st_id];
}


#ifdef __BT_ONE_BRING_TWO__

uint8_t avrcp_palyback_status[BT_DEVICE_NUM] = {0};
static struct BT_DEVICE_ID_DIFF avrcp_bond_a2dp_stream;
static uint8_t avrcp_bonding_a2dp_id[BT_DEVICE_NUM] = {BT_DEVICE_NUM,BT_DEVICE_NUM};
static POSSIBLY_UNUSED uint8_t a2dp_get_cur_stream_id(void)
{
    return (uint8_t)app_bt_device.curr_a2dp_stream_id;
}
static uint8_t avrcp_find_a2dp_stream_id(btif_avrcp_chnl_handle_t chnl)
{
    btif_remote_device_t * p_avrcp_remDev =   btif_avrcp_get_remote_device(chnl);
    uint8_t i =0;
    btif_remote_device_t * p_a2dp_remDev = 0;
    TRACE("avrcp_remDev = %02x",p_avrcp_remDev);
    for(i = 0; i<BT_DEVICE_NUM;i++)
    {
        p_a2dp_remDev = btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[i]);
        TRACE("p_a2dp_remDev = %02x",p_a2dp_remDev);
        if(p_avrcp_remDev == p_a2dp_remDev)
            break;
    }
    return i;
}
uint8_t get_avrcp_via_a2dp_id(uint8_t a2dp_id)
{
    enum BT_DEVICE_ID_T avrcp_id = BT_DEVICE_NUM;
    if(avrcp_bonding_a2dp_id[BT_DEVICE_ID_1] == a2dp_id)
        avrcp_id = BT_DEVICE_ID_1;
    else if(avrcp_bonding_a2dp_id[BT_DEVICE_ID_2] == a2dp_id)
        avrcp_id = BT_DEVICE_ID_2;
    TRACE("[%s] avrcp_id = %d",__func__,avrcp_id);
    return avrcp_id;
}
static uint8_t avrcp_distinguish_bonding_a2dp_stream(btif_avrcp_chnl_handle_t chnl,uint8_t id)
{
    avrcp_bond_a2dp_stream.id = (enum BT_DEVICE_ID_T)  avrcp_find_a2dp_stream_id(chnl);
    TRACE("%s id = %d",__func__,avrcp_bond_a2dp_stream.id);
    if(avrcp_bond_a2dp_stream.id == BT_DEVICE_NUM){
        avrcp_bond_a2dp_stream.id = BT_DEVICE_ID_1;
        return 2;
    }
    avrcp_bonding_a2dp_id[id] = avrcp_bond_a2dp_stream.id;
    return 0;
}

static void a2dp_to_bond_avrcp_with_stream(a2dp_stream_t* ptrA2dpStream, uint8_t a2dpDevIndex)
{
    for (uint8_t avrcpIndex = 0;avrcpIndex < BT_DEVICE_NUM;avrcpIndex++)
    {
        if ( btif_avrcp_get_remote_device(app_bt_device.avrcp_channel[avrcpIndex]->avrcp_channel_handle)
            ==    btif_a2dp_get_remote_device(ptrA2dpStream))
        {
            avrcp_bond_a2dp_stream.id = (enum BT_DEVICE_ID_T)a2dpDevIndex;
            avrcp_bonding_a2dp_id[avrcpIndex] = a2dpDevIndex;
            break;
        }
    }
}

#endif

#ifdef __BT_ONE_BRING_TWO__
void a2dp_update_music_link(void);
#endif
static void a2dp_set_cur_stream(enum BT_DEVICE_ID_T id)
{
    TRACE("Set current a2dp stream as %d", id);
    app_bt_device.curr_a2dp_stream_id = id;

#ifdef __BT_ONE_BRING_TWO__
    a2dp_update_music_link();
#endif
}

static uint8_t a2dp_skip_frame_cnt = 0;
enum BT_DEVICE_ID_T a2dp_get_cur_stream(void)
{
    return app_bt_device.curr_a2dp_stream_id;
}
void a2dp_get_curStream_remDev(btif_remote_device_t   **           p_remDev)
{
    enum BT_DEVICE_ID_T id =  a2dp_get_cur_stream();
    uint8_t i =0;
    if(id != BT_DEVICE_NUM)
        i = id;
    *p_remDev = btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[i]);
    //TRACE("temp_addr = %x",*p_remDev);
}
uint8_t app_a2dp_run_skip_frame(void)
{
    if(a2dp_skip_frame_cnt > 0){
        a2dp_skip_frame_cnt--;
        return 1;
    }
    a2dp_skip_frame_cnt = 0;
    return 0;
}
void app_a2dp_set_skip_frame(uint8_t frames)
{
    a2dp_skip_frame_cnt = frames;
}
#define SWITCH_MUTE_FRAMES_CNT  65
void app_a2dp_hold_mute()
{
    TRACE("mute a2dp streaming.");
    a2dp_skip_frame_cnt = SWITCH_MUTE_FRAMES_CNT;
}
void app_a2dp_unhold_mute()
{
    a2dp_skip_frame_cnt = 0;
}

avrcp_advanced_pdu_t * avrcp_get_play_status_cmd = NULL;

void avrcp_get_current_media_status(enum BT_DEVICE_ID_T device_id)
{
    if(app_bt_device.avrcp_state[device_id] == 0)
        return;
    if (NULL == avrcp_get_play_status_cmd)
    {
        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&avrcp_get_play_status_cmd);
    }
    btif_avrcp_ct_get_play_status(app_bt_device.avrcp_channel[device_id], avrcp_get_play_status_cmd);
}

void btapp_a2dp_suspend_music(enum BT_DEVICE_ID_T stream_id);
uint8_t a2dp_get_streaming_id(void);
void avrcp_set_media_status(uint8_t status);
int avrcp_callback_getplaystatus_init(void);

extern "C" void avrcp_callback_CT(btif_avrcp_chnl_handle_t chnl, const avrcp_callback_parms_t* parms)
{
     btif_avrcp_channel_t * channel = btif_get_avrcp_channel( chnl);
     btif_avctp_event_t event =  btif_avrcp_get_callback_event( parms);
     APP_A2DP_TRACE("%s : chnl %p, Parms %p\n", __func__,chnl, parms);
     APP_A2DP_TRACE("::%s Parms->event %d\n", __func__, btif_avrcp_get_callback_event(parms));
#ifdef __BT_ONE_BRING_TWO__
    enum BT_DEVICE_ID_T device_id = (chnl == app_bt_device.avrcp_channel[0]->avrcp_channel_handle) ? BT_DEVICE_ID_1 : BT_DEVICE_ID_2;
    enum BT_DEVICE_ID_T device_id_other = (device_id==BT_DEVICE_ID_1)?(BT_DEVICE_ID_2):(BT_DEVICE_ID_1);
#else
    enum BT_DEVICE_ID_T device_id = BT_DEVICE_ID_1;
#endif
    APP_A2DP_TRACE("device_id = %d",device_id);
    switch(event)
    {
        case BTIF_AVRCP_EVENT_CONNECT_IND:
#if defined(_AUTO_TEST_)
            AUTO_TEST_SEND("Connect ok.");
#endif
            APP_A2DP_TRACE("::avrcp_callback_CT AVRCP_EVENT_CONNECT_IND %d\n", btif_avrcp_get_callback_event(parms));
            btif_avrcp_connect_rsp (channel, 1);
#ifdef __TWS__
              app_bt_device.avrcpPendingKey = AVRCP_KEY_NULL;
              app_bt_device.ptrAvrcpChannel[device_id]->avrcp_channel_handle = chnl;
#endif
            break;
        case BTIF_AVRCP_EVENT_CONNECT:
#if defined(_AUTO_TEST_)
            AUTO_TEST_SEND("Connect ok.");
#endif
            app_bt_device.avrcp_state[device_id] = 1;
            if(btif_get_avrcp_version( channel) >= 0x103)
            {
                APP_A2DP_TRACE("::AVRCP_GET_CAPABILITY\n");
                if (app_bt_device.avrcp_cmd1[device_id] == NULL){
                   btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_cmd1[device_id]);
                }
				btif_avrcp_ct_get_capabilities(channel, app_bt_device.avrcp_cmd1[device_id],BTIF_AVRCP_CAPABILITY_EVENTS_SUPPORTED);
#ifdef __TWS__
                avrcp_set_media_status(0xff);
#endif
            }
            #ifdef __TWS__
            app_bt_device.ptrAvrcpChannel[device_id]->avrcp_channel_handle = chnl;
            app_bt_device.avrcpPendingKey = AVRCP_KEY_NULL;
            btif_set_avrcp_state(app_bt_device.avrcp_channel[device_id],BTIF_AVRCP_STATE_CONNECTED);
            #endif

#ifdef _AVRCP_TRACK_CHANGED
            if (app_bt_device.avrcp_cmd2[device_id] == NULL)
                btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_cmd2[device_id]);
            btif_avrcp_ct_register_notification(channel,app_bt_device.avrcp_cmd2[device_id],AVRCP_EID_TRACK_CHANGED,0);
#endif
#ifdef __AVRCP_TRACK_PLAY_STATUS__
            TRACE("::REG AVRCP_EID_MEDIA_STATUS_CHANGED\n");
            if (app_bt_device.avrcp_cmd1[device_id] == NULL)
                app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_cmd1[device_id]);
            btif_avrcp_ct_register_notification(channel,app_bt_device.avrcp_cmd1[device_id],AVRCP_EID_MEDIA_STATUS_CHANGED,0);
#endif
            avrcp_get_current_media_status(device_id);
            APP_A2DP_TRACE("::avrcp_callback_CT AVRCP_EVENT_CONNECT %x,device_id=%d\n", btif_get_avrcp_version( channel),device_id);
#ifdef __BT_ONE_BRING_TWO__
            avrcp_palyback_status[device_id] = 0;
            avrcp_bonding_a2dp_id[device_id] = BT_DEVICE_NUM;
            avrcp_distinguish_bonding_a2dp_stream(chnl,device_id);
#endif
            break;
        case BTIF_AVRCP_EVENT_DISCONNECT:
            app_bt_device.avrcp_state[device_id] = 0;
            TRACE("::AVRCP_EVENT_DISCONNECT");
        #ifdef __BT_ONE_BRING_TWO__
            if (app_bt_is_to_resume_music_player(device_id))
            {
                app_bt_reset_music_player_resume_state();
            }
        #endif
            if (app_bt_device.avrcp_get_capabilities_rsp[device_id]){
                btif_app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_get_capabilities_rsp[device_id]);
                app_bt_device.avrcp_get_capabilities_rsp[device_id] = NULL;
            }
        #ifdef __TWS__
            btif_set_avrcp_state(app_bt_device.avrcp_channel[device_id],BTIF_AVRCP_STATE_DISCONNECTED);

        #endif
            if (app_bt_device.avrcp_control_rsp[device_id]){
                btif_app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_control_rsp[device_id]);
                app_bt_device.avrcp_control_rsp[device_id] = NULL;
            }
            if (app_bt_device.avrcp_notify_rsp[device_id]){
                btif_app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_notify_rsp[device_id]);
                app_bt_device.avrcp_notify_rsp[device_id] = NULL;
            }

            if (app_bt_device.avrcp_cmd1[device_id]){
                btif_app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_cmd1[device_id]);
                app_bt_device.avrcp_cmd1[device_id] = NULL;
            }
            if (app_bt_device.avrcp_cmd2[device_id]){
                btif_app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_cmd2[device_id]);
                app_bt_device.avrcp_cmd2[device_id] = NULL;
            }
            app_bt_device.volume_report[device_id] = 0;
#ifdef AVRCP_TRACK_CHANGED
            app_bt_device.track_changed[device_id] = 0;
#endif
#ifdef __BT_ONE_BRING_TWO__
            avrcp_palyback_status[device_id ] = 0;
            avrcp_bonding_a2dp_id[device_id] = BT_DEVICE_NUM;
#endif
#ifdef __TWS__
            app_bt_device.ptrAvrcpChannel[device_id] = NULL;
            avrcp_set_media_status(0xff);
            avrcp_callback_getplaystatus_deinit();
            app_disconnection_handler_before_role_switch();

            osTimerStop(app_avrcp_connect_timer);
            if(!(is_simulating_reconnecting_in_progress()) &&
                    !(CONN_OP_STATE_MACHINE_DISCONNECT_ALL == CURRENT_CONN_OP_STATE_MACHINE()) &&
                    !(CONN_OP_STATE_MACHINE_DISCONNECT_ALL_BEFORE_RE_PAIRING == CURRENT_CONN_OP_STATE_MACHINE()) &&
                    !(CONN_OP_STATE_MACHINE_DISCONNECT_SLAVE == CURRENT_CONN_OP_STATE_MACHINE()) &&
                    !(CONN_OP_STATE_MACHINE_DISCONNECT_ALL_BEFORE_CHANGE_ROLE == CURRENT_CONN_OP_STATE_MACHINE())&&
               (!is_slave_tws_mode() )){

                osTimerStart(app_avrcp_connect_timer, 5000);
            }

#endif
            break;
        case BTIF_AVRCP_EVENT_RESPONSE:
            APP_A2DP_TRACE("::avrcp_callback_CT AVRCP_EVENT_RESPONSE op=%x,status=%x\n", btif_get_avrcp_cb_channel_advOp(parms),
                btif_get_avrcp_cb_channel_state(parms));


       if(   btif_get_avrcp_cb_channel_advOp(parms) == BTIF_AVRCP_OP_GET_PLAY_STATUS)
            {
                if( btif_get_avrcp_cb_channel_state(parms) == 0x1)
                {
                    app_bt_device.a2dp_play_pause_flag = 1;
                }
                else if(btif_get_avrcp_adv_rsp_play_status(parms)->mediaStatus == 0x0||btif_get_avrcp_adv_rsp_play_status(parms)->mediaStatus == 0x2)
                {
                    app_bt_device.a2dp_play_pause_flag = 0;
                }
            }

            break;
#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
/*For Sony Compability Consideration*/
        case BTIF_AVRCP_EVENT_PANEL_PRESS:
            TRACE("::avrcp_callback_TG AVRCP_EVENT_PANEL_PRESS %x,%x,device_id=%d",
                         btif_get_avrcp_panel_cnf (parms)->operation,  btif_get_avrcp_panel_ind( parms)->operation,device_id);

            switch( btif_get_avrcp_panel_ind( parms)->operation)
            {
                case BTIF_AVRCP_POP_VOLUME_UP:
                    TRACE("avrcp_callback_TG avrcp_key = AVRCP_KEY_VOLUME_UP");
                    app_bt_stream_volumeup();
                    break;
                case BTIF_AVRCP_POP_VOLUME_DOWN:
                    TRACE("avrcp_callback_TG avrcp_key = AVRCP_KEY_VOLUME_DOWN");
                    app_bt_stream_volumedown();
                    break;
                //SRC:for accept play/pause command from snk
                case BTIF_AVRCP_POP_PLAY:
                    TRACE("::avrcp start stream !");
                    app_a2dp_start_stream();
                    break;
                case BTIF_AVRCP_POP_PAUSE:
                    TRACE("::acrcp stop stream !");
                    app_a2dp_suspend_stream();
                    break;
                default :
                    break;
            }
            break;
#else
         /*For Sony Compability Consideration*/
        case BTIF_AVRCP_EVENT_PANEL_PRESS:
            TRACE("::avrcp_callback_TG AVRCP_EVENT_PANEL_PRESS %x,%x,device_id=%d",
                btif_get_avrcp_panel_cnf (parms)->operation, btif_get_avrcp_panel_ind( parms)->operation,device_id);
            switch(btif_get_avrcp_panel_cnf (parms)->operation)
            {
                case BTIF_AVRCP_POP_VOLUME_UP:
                    TRACE("avrcp_callback_TG avrcp_key = AVRCP_KEY_VOLUME_UP");
                    app_bt_stream_volumeup();
                    break;
                case BTIF_AVRCP_POP_VOLUME_DOWN:
                    TRACE("avrcp_callback_TG avrcp_key = AVRCP_KEY_VOLUME_DOWN");
                    app_bt_stream_volumedown();
                    break;
                default :
                    break;
            }
            break;

#endif
        case BTIF_AVRCP_EVENT_PANEL_HOLD:
            TRACE("::avrcp_callback_TG AVRCP_EVENT_PANEL_HOLD %x,%x",
               btif_get_avrcp_panel_cnf (parms)->operation, btif_get_avrcp_panel_ind( parms)->operation);
            break;
        case BTIF_AVRCP_EVENT_PANEL_RELEASE:
            TRACE("::avrcp_callback_TG AVRCP_EVENT_PANEL_RELEASE %x,%x",
                btif_get_avrcp_panel_cnf (parms)->operation,btif_get_avrcp_panel_ind( parms)->operation);
            break;
         /*For Sony Compability Consideration End*/
        case BTIF_AVRCP_EVENT_PANEL_CNF:
            TRACE("::AVRCP_EVENT_PANEL_CNF %x,%x,%x",
                btif_get_avrcp_panel_cnf (parms)->operation, btif_get_avrcp_panel_cnf (parms)->press, btif_get_avrcp_panel_cnf (parms)->response);
            break;
        case BTIF_AVRCP_EVENT_ADV_TX_DONE:
            TRACE("::AVRCP_EVENT_ADV_TX_DONE op:%d\n", btif_get_avrcp_cb_txPdu_Op(parms));
            if (btif_get_avrcp_cb_txPdu_Op(parms) == BTIF_AVRCP_OP_GET_CAPABILITIES){
                if (app_bt_device.avrcp_get_capabilities_rsp[device_id] == btif_get_avrcp_cb_txPdu( parms)){
                    app_bt_device.avrcp_get_capabilities_rsp[device_id] = NULL;
                    btif_app_a2dp_avrcpadvancedpdu_mempool_free(btif_get_avrcp_cb_txPdu( parms));
                }
            }
            if( btif_get_avrcp_pdu_ctype(app_bt_device.avrcp_notify_rsp[device_id]) ==BTIF_AVCTP_RESPONSE_INTERIM)
            {
                    vol_ctrl_done_flag[device_id] = TX_DONE_FLAG_SUCCESS;
                    //LOG_E("send AVCTP_RESPONSE_INTERIM successful device id %d = TX_DONE_FLAG_SUCCESS",device_id);
            }


            break;
        case BTIF_AVRCP_EVENT_ADV_RESPONSE:
            TRACE("::avrcp_callback_CT AVRCP_EVENT_ADV_RESPONSE device_id=%d,role=%x\n",device_id,btif_get_avrcp_channel_role(channel));
            TRACE("::avrcp_callback_CT AVRCP_EVENT_ADV_RESPONSE op=%x,status=%x\n", btif_get_avrcp_cb_channel_advOp(parms)
                , btif_get_avrcp_cb_channel_state(parms));

            if(btif_get_avrcp_cb_channel_advOp(parms) == BTIF_AVRCP_OP_GET_PLAY_STATUS && btif_get_avrcp_cb_channel_state(parms) == BT_STS_SUCCESS)
            {
                TRACE("::AVRCP_OP_GET_PLAY_STATUS %d/%d Status:%d, response=%d",
                                btif_get_avrcp_adv_rsp_play_status(parms)->position,
                                btif_get_avrcp_adv_rsp_play_status(parms)->length,
                                btif_get_avrcp_adv_rsp_play_status(parms)->mediaStatus);
                 avrcp_set_media_status(btif_get_avrcp_adv_rsp_play_status(parms)->mediaStatus);
            }

            if(btif_get_avrcp_cb_channel_advOp(parms)  == BTIF_AVRCP_OP_GET_CAPABILITIES && btif_get_avrcp_cb_channel_state(parms) == BT_STS_SUCCESS)
            {
                TRACE("::avrcp_callback_CT AVRCP eventmask=%x\n", btif_get_avrcp_adv_rsp(parms)->capability.info.eventMask);

                btif_set_avrcp_adv_rem_event_mask(channel, btif_get_avrcp_adv_rsp(parms)->capability.info.eventMask);
                if(btif_get_avrcp_adv_rem_event_mask(channel) & BTIF_AVRCP_ENABLE_PLAY_STATUS_CHANGED)
                {
                    TRACE("::avrcp_callback_CT AVRCP send notification\n");
                    if (app_bt_device.avrcp_cmd1[device_id] == NULL){
                        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_cmd1[device_id]);
                    }
                    btif_avrcp_ct_register_notification( channel, app_bt_device.avrcp_cmd1[device_id], BTIF_AVRCP_EID_MEDIA_STATUS_CHANGED,0);

                }
                if( btif_get_avrcp_adv_rem_event_mask( channel) & BTIF_AVRCP_ENABLE_PLAY_POS_CHANGED)
                {
                    if (app_bt_device.avrcp_cmd2[device_id] == NULL){
                        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_cmd2[device_id]);
                    }
                    btif_avrcp_ct_register_notification(channel,app_bt_device.avrcp_cmd2[device_id],BTIF_AVRCP_EID_PLAY_POS_CHANGED,1);

                }
            }
            else if(btif_get_avrcp_cb_channel_advOp(parms)  == BTIF_AVRCP_OP_REGISTER_NOTIFY &&btif_get_avrcp_cb_channel_state(parms) == BT_STS_SUCCESS)
            {
                if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_MEDIA_STATUS_CHANGED)
                {
                        TRACE("::avrcp_callback_CT ACRCP notify rsp playback states=%x",btif_get_avrcp_adv_notify(parms)->p.mediaStatus);
#if defined(__BT_ONE_BRING_TWO__)
                    if(btif_get_avrcp_adv_notify(parms)->p.mediaStatus == 0x1){
                        avrcp_palyback_status[device_id] = 0x01;
                        if (app_bt_is_to_resume_music_player(device_id) ||
                                (BTIF_HF_AUDIO_CON == app_bt_device.hf_audio_state[device_id_other]))
                        {
                            app_bt_pause_media_player_again(device_id);
                        }
                    }else if(btif_get_avrcp_adv_notify(parms)->p.mediaStatus == 0x0||btif_get_avrcp_adv_notify(parms)->p.mediaStatus == 0x2){
                        avrcp_palyback_status[device_id] = 0x00;
                        app_bt_device.latestPausedDevId = device_id;
                    }
#endif
#if defined(__BT_ONE_BRING_TWO__) && defined(__MULTIPOINT_A2DP_PREEMPT__)
                    uint8_t is_a2dp_streaming = 0;
                    enum BT_DEVICE_ID_T avrcp_bonding_a2dp_another = BT_DEVICE_NUM;
                    is_a2dp_streaming = a2dp_get_streaming_id();
                    TRACE("is_a2dp_streaming = %d",is_a2dp_streaming);
                    TRACE("device_id = %d other_id = %d",device_id,device_id_other);
                    if(avrcp_distinguish_bonding_a2dp_stream(chnl,device_id) == 0x02){
                        return;
                    }
                    avrcp_bonding_a2dp_another = (avrcp_bond_a2dp_stream.id== BT_DEVICE_ID_1)?(BT_DEVICE_ID_2):(BT_DEVICE_ID_1);
                    TRACE("avrcp_palyback_status_id[%d] = %d , avrcp_palyback_status_id_other[%d] = %d cur_a2dp_stream = %d",device_id,
                            avrcp_palyback_status[device_id],device_id_other,avrcp_palyback_status[device_id_other],app_bt_device.curr_a2dp_stream_id);
                    if((avrcp_palyback_status[BT_DEVICE_ID_1] == 0) &&
                       (avrcp_palyback_status[BT_DEVICE_ID_2] == 0)){
                        app_bt_device.a2dp_play_pause_flag = 0;
                    }
                    else{
                        app_bt_device.a2dp_play_pause_flag = 1;
                    }
                    if(is_a2dp_streaming !=0){
                        if(avrcp_palyback_status[device_id] == 1) /*&&
                                                                    (avrcp_palyback_status[BT_DEVICE_ID_2] == 1))*/ {
                            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC,avrcp_bond_a2dp_stream.id,MAX_RECORD_NUM);
                            a2dp_set_cur_stream(avrcp_bond_a2dp_stream.id);
#if 1
                            if(avrcp_palyback_status[device_id_other] == 1){
                                btapp_a2dp_suspend_music(device_id_other);
                                app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,avrcp_bonding_a2dp_another,MAX_RECORD_NUM);
                                /*if(bt_media_is_media_active_by_device(BT_STREAM_SBC,avrcp_bond_a2dp_stream.id_other) != 0)*/
                                {
                                    app_bt_stream_volumeset(TGT_VOLUME_LEVEL_MUTE);
                                    app_a2dp_hold_mute();
                                }
                            }
#endif
                        }
                        if(is_a2dp_streaming >2){
                            if((avrcp_palyback_status[device_id] == 1) &&
                                    (avrcp_palyback_status[device_id_other] == 0)){
                                a2dp_set_cur_stream(avrcp_bond_a2dp_stream.id);
                            }
                            if(app_bt_device.a2dp_play_pause_flag == 0){
                                app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,avrcp_bond_a2dp_stream.id,MAX_RECORD_NUM);
                            }
                        }
                        if(app_bt_device.a2dp_play_pause_flag == 1){
                            if(avrcp_palyback_status[device_id] == 0){
                                a2dp_set_cur_stream(avrcp_bonding_a2dp_another);
                                app_a2dp_unhold_mute();
                            }
                        }
                    }
                    TRACE(">stream_id = %d",a2dp_get_cur_stream_id());
#else
                    if(btif_get_avrcp_adv_notify(parms)->p.mediaStatus == 0x1) {
                        app_bt_device.a2dp_play_pause_flag = 1;
                    } else if(btif_get_avrcp_adv_notify(parms)->p.mediaStatus == 0x0||btif_get_avrcp_adv_notify(parms)->p.mediaStatus == 0x2){
                        // app_bt_device.a2dp_play_pause_flag = 0;
                    }
#endif
                }
                else if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_PLAY_POS_CHANGED)
                {
                    TRACE("::ACRCP notify rsp play pos =%x",btif_get_avrcp_adv_notify(parms)->p.position);
                }
                else if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_VOLUME_CHANGED){
                    TRACE("::ACRCP notify rsp volume =%x",btif_get_avrcp_adv_notify(parms)->p.volume);
                    a2dp_volume_set(device_id, btif_get_avrcp_adv_notify(parms)->p.volume);
                }
#ifdef AVRCP_TRACK_CHANGED
                    else if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_TRACK_CHANGED){
                    //   TRACE("::AVRCP_EID_TRACK_CHANGED transId:%d\n", Parms->p.cmdFrame->transId);
                    if (app_bt_device.avrcp_notify_rsp[device_id] == NULL)
                            btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_notify_rsp[device_id]);

                        btif_set_app_bt_device_avrcp_notify_rsp_ctype(app_bt_device.avrcp_notify_rsp[device_id], BTIF_AVCTP_RESPONSE_INTERIM);

                        app_bt_device.track_changed[device_id] = BTIF_AVCTP_RESPONSE_INTERIM;
                        btif_avrcp_ct_get_media_Info(channel,app_bt_device.avrcp_notify_rsp[device_id],0x7f)    ;
                }
#endif
            }
            else if(btif_get_avrcp_cb_channel_advOp(parms) == BTIF_AVRCP_OP_GET_PLAY_STATUS && btif_get_avrcp_cb_channel_state(parms)  == BT_STS_SUCCESS)
            {
                TRACE("AVRCP get play status returns %d", btif_get_avrcp_adv_rsp_play_status(parms)->mediaStatus);
            #if defined(__BT_ONE_BRING_TWO__)
                if(btif_get_avrcp_adv_rsp_play_status(parms)->mediaStatus == 0x1){
                    avrcp_palyback_status[device_id] = 0x01;
                }else if(btif_get_avrcp_adv_rsp_play_status(parms)->mediaStatus == 0x0||btif_get_avrcp_adv_rsp_play_status(parms)->mediaStatus == 0x2){
                    avrcp_palyback_status[device_id] = 0x00;
                }
            #endif
            }
#ifdef AVRCP_TRACK_CHANGED
            else if(btif_get_avrcp_cb_channel_advOp(parms) == BTIF_AVRCP_OP_GET_MEDIA_INFO && btif_get_avrcp_cb_channel_state(parms) == BT_STS_SUCCESS){
                TRACE("AVRCP_TRACK_CHANGED numid=%d",btif_get_avrcp_adv_rsp(parms)->element.numIds);
                for(uint8_t i=0;i<7;i++)
                {
                    if(btif_get_avrcp_adv_rsp(parms)->element.txt[i].length>0)
                    {
                        TRACE("Id=%d,%s\n",i,btif_get_avrcp_adv_rsp(parms)->element.txt[i].string);
                    }
                }

            }
#endif
            break;
        case BTIF_AVRCP_EVENT_COMMAND:
            TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND device_id=%d,role=%x\n",device_id,btif_get_avrcp_channel_role(channel));
            TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND ctype=%x,subunitype=%x\n", btif_get_avrcp_cmd_frame(parms)->ctype,btif_get_avrcp_cmd_frame(parms)->subunitType);
            TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND subunitId=%x,opcode=%x\n", btif_get_avrcp_cmd_frame(parms)->subunitId,btif_get_avrcp_cmd_frame(parms)->opcode);
            TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND operands=%x,operandLen=%x\n", btif_get_avrcp_cmd_frame(parms)->operands,btif_get_avrcp_cmd_frame(parms)->operandLen);
            TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND more=%x\n", btif_get_avrcp_cmd_frame(parms)->more);
            if( btif_get_avrcp_cmd_frame(parms)->ctype == BTIF_AVRCP_CTYPE_STATUS)
            {
                uint32_t company_id = *(btif_get_avrcp_cmd_frame(parms)->operands+2) + ((uint32_t)(*(btif_get_avrcp_cmd_frame(parms)->operands+1))<<8) + ((uint32_t)(*(btif_get_avrcp_cmd_frame(parms)->operands))<<16);
                TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND company_id=%x\n", company_id);
                if(company_id == 0x001958)  //bt sig
                {
                    avrcp_operation_t op = *(btif_get_avrcp_cmd_frame(parms)->operands+3);
                    uint8_t oplen =  *(btif_get_avrcp_cmd_frame(parms)->operands+6)+ ((uint32_t)(*(btif_get_avrcp_cmd_frame(parms)->operands+5))<<8);
                    TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND op=%x,oplen=%x\n", op,oplen);
                    switch(op)
                    {
                        case BTIF_AVRCP_OP_GET_CAPABILITIES:
                            {
                                uint8_t event = *(btif_get_avrcp_cmd_frame(parms)->operands+7);
                                if(event==BTIF_AVRCP_CAPABILITY_COMPANY_ID)
                                {
                                    TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND send support compay id");
                                }
                                else if(event == BTIF_AVRCP_CAPABILITY_EVENTS_SUPPORTED)
                                {
                                    TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND send support event transId:%d", btif_get_avrcp_cmd_frame(parms)->transId);
#ifdef __AVRCP_EVENT_COMMAND_VOLUME_SKIP__
                                    //chnl->adv.eventMask = 0;
                                     btif_set_avrcp_adv_rem_event_mask(channel, 0);

#else
                                     btif_set_avrcp_adv_rem_event_mask(channel, BTIF_AVRCP_ENABLE_VOLUME_CHANGED);
#endif
                                    if (app_bt_device.avrcp_get_capabilities_rsp[device_id] == NULL)
                                        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_get_capabilities_rsp[device_id]);
                                         btif_avrcp_set_capabilities_rsp_cmd(app_bt_device.avrcp_get_capabilities_rsp[device_id],btif_get_avrcp_cmd_frame(parms)->transId, BTIF_AVCTP_RESPONSE_IMPLEMENTED_STABLE);
                                         TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND send support event transId:%d",    btif_get_app_bt_device_avrcp_notify_rsp_transid(app_bt_device.avrcp_get_capabilities_rsp[device_id]));
                                         btif_avrcp_ct_get_capabilities_rsp(channel,app_bt_device.avrcp_get_capabilities_rsp[device_id],BTIF_AVRCP_CAPABILITY_EVENTS_SUPPORTED,btif_get_avrcp_adv_rem_event_mask(channel));
                                         avrcp_callback_getplaystatus_init();
                                }
                                else
                                {
                                    TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND send error event value");
                                }
                        }
                        break;
                    }

                }

            }else if(btif_get_avrcp_cmd_frame(parms)->ctype == BTIF_AVCTP_CTYPE_CONTROL){
                TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND AVCTP_CTYPE_CONTROL\n");
                DUMP8("%02x ", btif_get_avrcp_cmd_frame(parms)->operands, btif_get_avrcp_cmd_frame(parms)->operandLen);
                if (btif_get_avrcp_cmd_frame(parms)->operands[3] == BTIF_AVRCP_OP_SET_ABSOLUTE_VOLUME){
                    TRACE("::avrcp_callback_CT AVRCP_EID_VOLUME_CHANGED transId:%d\n", btif_get_avrcp_cmd_frame(parms)->transId);
                    if((btif_get_avrcp_cmd_frame(parms)->operands[7]<127)&&(btif_get_avrcp_cmd_frame(parms)->operands[7]>1)){
                        a2dp_volume_set(device_id,btif_get_avrcp_cmd_frame(parms)->operands[7] + 1);
                    }else{
                        a2dp_volume_set(device_id,btif_get_avrcp_cmd_frame(parms)->operands[7]);
                    }
                    //a2dp_volume_set(device_id,  btif_get_avrcp_cmd_frame(parms)->operands[7]);
                    if (app_bt_device.avrcp_control_rsp[device_id] == NULL)
                        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_control_rsp[device_id]);
#if defined(__BQB_PROFILE_TEST__)
                    if((btif_get_avrcp_cmd_frame(parms)->operands[7] > 0x7f) ||(btif_get_avrcp_cmd_frame(parms)->operandLen > 8))//it works for BQB
                    {
                        btif_avrcp_set_control_rsp_cmd_witherror(app_bt_device.avrcp_control_rsp[device_id],btif_get_avrcp_cmd_frame(parms)->transId,  BTIF_AVCTP_RESPONSE_REJECTED, BTIF_AVRCP_ERR_INVALID_PARM);
                        TRACE("reject invalid volume");
                    }
                    else
#endif
                    btif_avrcp_set_control_rsp_cmd(app_bt_device.avrcp_control_rsp[device_id],btif_get_avrcp_cmd_frame(parms)->transId,  BTIF_AVCTP_RESPONSE_ACCEPTED);

                    DUMP8("%02x ", btif_get_avrcp_cmd_frame(parms)->operands, btif_get_avrcp_cmd_frame(parms)->operandLen);
                    btif_avrcp_ct_accept_absolute_volume_rsp(channel, app_bt_device.avrcp_control_rsp[device_id], btif_get_avrcp_cmd_frame(parms)->operands[7]);
#ifdef __TWS__
                if(btif_get_avrcp_cmd_frame(parms)->operands[7]<127){
                    app_tws_set_slave_volume(btif_get_avrcp_cmd_frame(parms)->operands[7] + 1);
                }else{
                    app_tws_set_slave_volume(btif_get_avrcp_cmd_frame(parms)->operands[7]);
                }
                    // avrcp_set_slave_volume(Parms->p.cmdFrame->transId,Parms->p.cmdFrame->operands[7]);/* for volume up/down on phone */
#endif
                } else if (BTIF_AVRCP_OP_CUSTOM_CMD == btif_get_avrcp_cmd_frame(parms)->operands[3]) {
                    app_AVRCP_CustomCmd_Received(&btif_get_avrcp_cmd_frame(parms)->operands[7], btif_get_avrcp_cmd_frame(parms)->operandLen - 7);
                    app_AVRCP_sendCustomCmdRsp(device_id, channel, true,btif_get_avrcp_cmd_frame(parms)->transId);
                }
            }else if (btif_get_avrcp_cmd_frame(parms)->ctype == BTIF_AVCTP_CTYPE_NOTIFY){
                bt_status_t status;
                TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND AVCTP_CTYPE_NOTIFY\n");
                DUMP8("%02x ", btif_get_avrcp_cmd_frame(parms)->operands, btif_get_avrcp_cmd_frame(parms)->operandLen);
                if (btif_get_avrcp_cmd_frame(parms)->operands[7] == BTIF_AVRCP_EID_VOLUME_CHANGED){
                    TRACE("::avrcp_callback_CT AVRCP_EID_VOLUME_CHANGED transId:%d\n",   btif_get_avrcp_cmd_frame(parms)->transId);
                    if (app_bt_device.avrcp_notify_rsp[device_id] == NULL){
                        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_notify_rsp[device_id]);
                    }
					btif_avrcp_set_notify_rsp_cmd(app_bt_device.avrcp_notify_rsp[device_id],btif_get_avrcp_cmd_frame(parms)->transId,  BTIF_AVCTP_RESPONSE_INTERIM) ;
                    app_bt_device.volume_report[device_id] = BTIF_AVCTP_RESPONSE_INTERIM;
                    status = btif_avrcp_ct_get_absolute_volume_rsp(channel,
                                                  app_bt_device.avrcp_notify_rsp[device_id],a2dp_volume_get(device_id));
                    TRACE("::avrcp_callback_CT AVRCP_EVENT_COMMAND AVRCP_EID_VOLUME_CHANGED nRet:%x\n",status);
#ifdef __TWS__
                    app_tws_set_slave_volume(a2dp_volume_get_tws());
#endif
                }
#if defined(__BQB_PROFILE_TEST__)
                else if(btif_get_avrcp_cmd_frame(parms)->operands[7] == 0xff)//it works for BQB
                {
                    TRACE("trances id:%d",btif_get_avrcp_cmd_frame(parms)->transId);
                        if (app_bt_device.avrcp_notify_rsp[device_id] == NULL){
                            btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_notify_rsp[device_id]);
                        }
                        btif_avrcp_set_notify_rsp_cmd_witherror(app_bt_device.avrcp_notify_rsp[device_id],btif_get_avrcp_cmd_frame(parms)->transId,  BTIF_AVCTP_RESPONSE_REJECTED, BTIF_AVRCP_ERR_INVALID_PARM);

                        status = btif_avrcp_ct_invalid_volume_rsp(channel, app_bt_device.avrcp_notify_rsp[device_id]);
                        TRACE("AVRCP_CtInvalidVolume_Rsp,status%d",status);
                }
#endif
            }
//#endif
            break;
        case BTIF_AVRCP_EVENT_ADV_NOTIFY:
            TRACE("::avrcp_callback_CT AVRCP_EVENT_ADV_NOTIFY  adv.notify.event=%x,device_id=%d,chnl->role=%x\n",btif_get_avrcp_adv_notify(parms)->event,device_id, btif_get_avrcp_cb_channel_role( channel));
            if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_VOLUME_CHANGED)
            {
                 TRACE("::avrcp_callback_CT ACRCP notify  vol =%x",btif_get_avrcp_adv_notify(parms)->p.volume);
                 // AVRCP_CtRegisterNotification(chnl,app_bt_device.avrcp_notify_rsp[device_id],BTIF_AVRCP_EID_VOLUME_CHANGED,0);
                 btif_avrcp_ct_register_notification(channel,app_bt_device.avrcp_notify_rsp[device_id],BTIF_AVRCP_EID_VOLUME_CHANGED,0);
            }
           else if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_MEDIA_STATUS_CHANGED)
            {
                TRACE("::avrcp_callback_CT ACRCP notify  playback states=%x",btif_get_avrcp_adv_notify(parms)->p.mediaStatus);
                if (app_bt_device.avrcp_cmd1[device_id] == NULL){
                    btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_cmd1[device_id]);
                }
                btif_avrcp_ct_register_notification(channel,app_bt_device.avrcp_cmd1[device_id],BTIF_AVRCP_EID_MEDIA_STATUS_CHANGED,0);
            }
           else if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_PLAY_POS_CHANGED)
            {
                TRACE("::avrcp_callback_CT ACRCP notify  play pos =%x",btif_get_avrcp_adv_notify(parms)->p.position);
                if (app_bt_device.avrcp_cmd2[device_id] == NULL){
                    btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_cmd2[device_id]);
                }
                btif_avrcp_ct_register_notification(channel,app_bt_device.avrcp_cmd2[device_id],BTIF_AVRCP_EID_PLAY_POS_CHANGED,1);
            }
#ifdef AVRCP_TRACK_CHANGED
         else if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_TRACK_CHANGED){
            TRACE("::AVRCP notify track msU32=%x, lsU32=%x",btif_get_avrcp_adv_notify(parms)->p.track.msU32,btif_get_avrcp_adv_notify(parms)->p.track.lsU32);
                if (app_bt_device.avrcp_cmd2[device_id] == NULL){
                    btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_cmd2[device_id]);
                }
                btif_avrcp_ct_register_notification(channel,app_bt_device.avrcp_cmd2[device_id],BTIF_AVRCP_EID_TRACK_CHANGED,0);
            }
#endif
            break;
        case BTIF_AVRCP_EVENT_ADV_CMD_TIMEOUT:
            TRACE("::avrcp_callback_CT AVRCP_EVENT_ADV_CMD_TIMEOUT device_id=%d,role=%x\n",device_id,btif_get_avrcp_cb_channel_role(channel));
            break;

    }

#if defined(IBRT)
        app_tws_ibrt_profile_callback(IBRT_AVRCP_PROFILE,(void *)chnl, (void *)parms);
#endif

}

#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
extern "C" void avrcp_callback_TG(btif_avrcp_chnl_handle_t chnl, const avrcp_callback_parms_t *parms)
{
    //do nothing
}
#endif
#ifdef __TWS__
typedef uint8_t tx_done_flag;
#define TX_DONE_FLAG_INIT           0
#define TX_DONE_FLAG_SUCCESS    1
#define TX_DONE_FLAG_FAIL           2
#define TX_DONE_FLAG_TXING        3
tx_done_flag TG_tx_done_flag = TX_DONE_FLAG_INIT;


void avrcp_callback_TG(btif_avrcp_chnl_handle_t chnl, const avrcp_callback_parms_t *parms)
{
    APP_A2DP_TRACE("avrcp_callback_TG : chnl %p, Parms %p\n", chnl, parms);
    APP_A2DP_TRACE("::avrcp_callback_TG Parms->event %d\n", btif_get_avrcp_cb_channel_state(parms));
    btif_avrcp_channel_t*  channel =  btif_get_avrcp_channel(chnl);


#ifdef __BT_ONE_BRING_TWO__
    enum BT_DEVICE_ID_T device_id = (chnl == app_bt_device.avrcp_channel[0]->avrcp_channel_handle)?BT_DEVICE_ID_1:BT_DEVICE_ID_2;
#else
    enum BT_DEVICE_ID_T device_id = BT_DEVICE_ID_1;
#endif
    switch(btif_avrcp_get_callback_event((avrcp_callback_parms_t *)parms))
    {
        case BTIF_AVRCP_EVENT_CONNECT:
        btif_set_avrcp_state(channel,BTIF_AVRCP_STATE_CONNECTED);
#ifdef __TWS__
            if(is_tws_master_conn_slave_state())
            {
                // avrcp_set_slave_volume(0,a2dp_volume_get_tws());
            }
#endif

            if (app_bt_device.avrcp_custom_cmd[device_id] == NULL)
            {
                btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_custom_cmd[device_id]);
            }
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_CONNECT %x,device_id=%d\n", btif_get_avrcp_version( channel),device_id);

            break;
        case BTIF_AVRCP_EVENT_DISCONNECT:
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_DISCONNECT");


        btif_set_avrcp_state(channel,BTIF_AVRCP_STATE_DISCONNECTED) ;
            if (app_bt_device.avrcp_get_capabilities_rsp[device_id]){
                btif_app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_get_capabilities_rsp[device_id]);
                app_bt_device.avrcp_get_capabilities_rsp[device_id] = NULL;
            }
            if (app_bt_device.avrcp_control_rsp[device_id]){
                btif_app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_control_rsp[device_id]);
                app_bt_device.avrcp_control_rsp[device_id] = NULL;
            }
            if (app_bt_device.avrcp_notify_rsp[device_id]){
                btif_app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_notify_rsp[device_id]);
                app_bt_device.avrcp_notify_rsp[device_id] = NULL;
            }

            if (app_bt_device.avrcp_cmd1[device_id]){
                btif_app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_cmd1[device_id]);
                app_bt_device.avrcp_cmd1[device_id] = NULL;
            }
            if (app_bt_device.avrcp_cmd2[device_id]){
                btif_app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_cmd2[device_id]);
                app_bt_device.avrcp_cmd2[device_id] = NULL;
            }

            if (app_bt_device.avrcp_custom_cmd[device_id]){
                btif_app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_custom_cmd[device_id]);
                app_bt_device.avrcp_custom_cmd[device_id] = NULL;
            }

            app_bt_device.volume_report[device_id] = 0;


            break;
        case BTIF_AVRCP_EVENT_RESPONSE:
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_RESPONSE op=%x,status=%x\n", btif_get_avrcp_cb_channel_advOp(parms),
                btif_get_avrcp_cb_channel_state(parms));

            break;
        case BTIF_AVRCP_EVENT_PANEL_CNF:
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_PANEL_CNF %x,%x,%x",
               btif_get_avrcp_panel_cnf(parms)->response, btif_get_avrcp_panel_cnf(parms)->operation,
                                                              btif_get_avrcp_panel_cnf(parms)->press);
            break;
#ifdef __TWS__
        case BTIF_AVRCP_EVENT_PANEL_PRESS:
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_PANEL_PRESS %x,%x,device_id=%d",
                 btif_get_avrcp_panel_cnf(parms)->operation,   btif_get_avrcp_panel_ind((avrcp_callback_parms_t *)parms)->operation,   device_id);
            switch(btif_get_avrcp_panel_ind((avrcp_callback_parms_t *)parms)->operation)
            {
                case BTIF_AVRCP_POP_STOP:
                    APP_A2DP_TRACE("avrcp_callback_TG avrcp_key = AVRCP_KEY_STOP");
                    btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_STOP,TRUE);
                    btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_STOP,FALSE);

                    app_bt_device.a2dp_play_pause_flag = 0;
                    break;
                case BTIF_AVRCP_POP_PLAY:
                case BTIF_AVRCP_POP_PAUSE:
                    if(app_bt_device.a2dp_play_pause_flag == 0){
                        btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_PLAY,TRUE);
                        btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_PLAY,FALSE);
                        app_bt_device.a2dp_play_pause_flag = 1;
                    }else{
                        btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_PAUSE,TRUE);
                        btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_PAUSE,FALSE);
                        app_bt_device.a2dp_play_pause_flag = 0;
                    }

                    break;
                case BTIF_AVRCP_POP_FORWARD:
                    APP_A2DP_TRACE("avrcp_callback_TG avrcp_key = AVRCP_KEY_FORWARD");
                    btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_FORWARD,TRUE);
                    btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_FORWARD,FALSE);
                    app_bt_device.a2dp_play_pause_flag = 1;
                    break;
                case BTIF_AVRCP_POP_BACKWARD:
                    APP_A2DP_TRACE("avrcp_callback_TG avrcp_key = AVRCP_KEY_BACKWARD");
                    btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_BACKWARD,TRUE);
                    btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_BACKWARD,FALSE);
                    app_bt_device.a2dp_play_pause_flag = 1;
                    break;
                case BTIF_AVRCP_POP_VOLUME_UP:
                    APP_A2DP_TRACE("avrcp_callback_TG avrcp_key = AVRCP_KEY_VOLUME_UP");
                    btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_VOLUME_UP,TRUE);
                    btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_VOLUME_UP,FALSE);
                    app_bt_stream_volumeup();
                    btapp_hfp_report_speak_gain();
                    btapp_a2dp_report_speak_gain();/* for press volume up key on slave */
                    break;
                case BTIF_AVRCP_POP_VOLUME_DOWN:
                    APP_A2DP_TRACE("avrcp_callback_TG avrcp_key = AVRCP_KEY_VOLUME_DOWN");
                    btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_VOLUME_DOWN,TRUE);
                    btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],BTIF_AVRCP_POP_VOLUME_DOWN,FALSE);
                    app_bt_stream_volumedown();
                    btapp_hfp_report_speak_gain();
                    btapp_a2dp_report_speak_gain();/* for press volume down key on slave */
                    break;
                default :
                    break;
            }
            break;
        case BTIF_AVRCP_EVENT_PANEL_HOLD:
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_PANEL_HOLD %x,%x",
                btif_get_avrcp_panel_cnf(parms)->operation,btif_get_avrcp_panel_ind(parms)->operation);
            break;
        case BTIF_AVRCP_EVENT_PANEL_RELEASE:
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_PANEL_RELEASE %x,%x",
               btif_get_avrcp_panel_cnf(parms)->operation,btif_get_avrcp_panel_ind(parms)->operation);
            break;
#endif
        case BTIF_AVRCP_EVENT_ADV_TX_DONE:
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_ADV_TX_DONE device_id=%d,status=%x,errorcode=%x\n",device_id,
                btif_get_avrcp_cb_channel_state(parms),btif_get_avrcp_cb_channel_error_code(parms));
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_ADV_TX_DONE op:%d, transid:%x\n", btif_get_avrcp_cb_txPdu_Op(parms),
                btif_get_avrcp_cb_txPdu_transId(parms));
            if (btif_get_avrcp_cb_txPdu_Op(parms) == BTIF_AVRCP_OP_GET_CAPABILITIES){
                if (app_bt_device.avrcp_get_capabilities_rsp[device_id] == btif_get_avrcp_cb_txPdu(parms)){
                    app_bt_device.avrcp_get_capabilities_rsp[device_id] = NULL;
                    btif_app_a2dp_avrcpadvancedpdu_mempool_free(btif_get_avrcp_cb_txPdu(parms));
                }
            }
            TG_tx_done_flag = TX_DONE_FLAG_SUCCESS;
#if 0
            if (Parms->p.adv.txPdu->op == AVRCP_OP_SET_ABSOLUTE_VOLUME){
                if (Parms->p.adv.txPdu->ctype != AVCTP_RESPONSE_INTERIM){
                    if (app_bt_device.avrcp_control_rsp[device_id] == Parms->p.adv.txPdu){
                        app_bt_device.avrcp_control_rsp[device_id] = NULL;
                        app_a2dp_avrcpadvancedpdu_mempool_free(Parms->p.adv.txPdu);
                    }
                }
            }
            if (Parms->p.adv.txPdu->op == AVRCP_OP_REGISTER_NOTIFY){
                if (Parms->p.adv.txPdu->ctype != AVCTP_RESPONSE_INTERIM){
                    if (Parms->p.adv.txPdu->parms[0] == AVRCP_EID_VOLUME_CHANGED){
                        app_bt_device.avrcp_notify_rsp[device_id] = NULL;
                        app_a2dp_avrcpadvancedpdu_mempool_free(Parms->p.adv.txPdu);
                    }
                }
            }
#endif

            break;
        case BTIF_AVRCP_EVENT_COMMAND:
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND device_id=%d,role=%x\n",device_id, btif_get_avrcp_channel_role(channel));
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND ctype=%x,subunitype=%x\n", btif_get_avrcp_cmd_frame(parms)->ctype,
                btif_get_avrcp_cmd_frame(parms)->subunitType);
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND subunitId=%x,opcode=%x\n", btif_get_avrcp_cmd_frame(parms)->subunitId,
                btif_get_avrcp_cmd_frame(parms)->opcode);
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND operands=%x,operandLen=%x\n",
                btif_get_avrcp_cmd_frame(parms)->operands,btif_get_avrcp_cmd_frame(parms)->operandLen);
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND more=%x\n", btif_get_avrcp_cmd_frame(parms)->more);
            if(btif_get_avrcp_cmd_frame(parms)->ctype == BTIF_AVRCP_CTYPE_STATUS)
            {
                uint32_t company_id = *(btif_get_avrcp_cmd_frame(parms)->operands+2) + ((uint32_t)(*(btif_get_avrcp_cmd_frame(parms)->operands+1))<<8) +
                    ((uint32_t)(*(btif_get_avrcp_cmd_frame(parms)->operands))<<16);
                TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND company_id=%x\n", company_id);
                if(company_id == 0x001958)  //bt sig
                {
                    avrcp_operation_t op = *(btif_get_avrcp_cmd_frame(parms)->operands+3);
                    uint8_t oplen =  *(btif_get_avrcp_cmd_frame(parms)->operands+6)+ ((uint32_t)(*(btif_get_avrcp_cmd_frame(parms)->operands+5))<<8);
                    APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND op=%x,oplen=%x\n", op,oplen);
                    switch(op)
                    {
                        case BTIF_AVRCP_OP_GET_CAPABILITIES:
                            {
                                uint8_t event = *(btif_get_avrcp_cmd_frame(parms)->operands+7);
                                if(event==BTIF_AVRCP_CAPABILITY_COMPANY_ID)
                                {
                                    APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND send support compay id");
                                }
                                else if(event == BTIF_AVRCP_CAPABILITY_EVENTS_SUPPORTED)
                                {
                                    APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND send support event transId:%d",btif_get_avrcp_cmd_frame(parms)->transId);
#ifdef __AVRCP_EVENT_COMMAND_VOLUME_SKIP__
                                   btif_avrcp_set_channel_adv_event_mask(chnl, 0);   ///volume control
#else
                                    btif_avrcp_set_channel_adv_event_mask(chnl, BTIF_AVRCP_ENABLE_VOLUME_CHANGED);   ///volume control
#endif
                                    if (app_bt_device.avrcp_get_capabilities_rsp[device_id] == NULL)
                                        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_get_capabilities_rsp[device_id]);

                                   // app_bt_device.avrcp_get_capabilities_rsp[device_id]->transId = btif_get_avrcp_cmd_frame(parms)->transId;
                                    //app_bt_device.avrcp_get_capabilities_rsp[device_id]->ctype = BTIF_AVCTP_RESPONSE_IMPLEMENTED_STABLE;
                    btif_avrcp_set_capabilities_rsp_cmd( app_bt_device.avrcp_get_capabilities_rsp[device_id],btif_get_avrcp_cmd_frame(parms)->transId, BTIF_AVCTP_RESPONSE_IMPLEMENTED_STABLE);
                                    APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND send support event transId:%d",  btif_get_app_bt_device_avrcp_notify_rsp_transid(app_bt_device.avrcp_get_capabilities_rsp[device_id]));
                                    btif_avrcp_ct_get_capabilities_rsp(channel,app_bt_device.avrcp_get_capabilities_rsp[device_id],BTIF_AVRCP_CAPABILITY_EVENTS_SUPPORTED,btif_get_avrcp_adv_rem_event_mask(channel));
                                   // AVRCP_CtGetCapabilities_Rsp(chnl,app_bt_device.avrcp_get_capabilities_rsp[device_id],BTIF_AVRCP_CAPABILITY_EVENTS_SUPPORTED,chnl->adv.eventMask);
                                }
                                else
                                {
                                    APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND send error event value");
                                }
                            }
                            break;
                    }

                }

            }else if(btif_get_avrcp_cmd_frame(parms)->ctype == BTIF_AVCTP_CTYPE_CONTROL){
                APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND AVCTP_CTYPE_CONTROL\n");
                DUMP8("%02x ", btif_get_avrcp_cmd_frame(parms)->operands, btif_get_avrcp_cmd_frame(parms)->operandLen);
                if (btif_get_avrcp_cmd_frame(parms)->operands[3] == BTIF_AVRCP_OP_SET_ABSOLUTE_VOLUME){
                    APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EID_VOLUME_CHANGED transId:%d\n", btif_get_avrcp_cmd_frame(parms)->transId);
                    a2dp_volume_set(device_id, (btif_get_avrcp_cmd_frame(parms)->operands[7])+1);
                    //a2dp_volume_set(btif_get_avrcp_cmd_frame(parms)->operands[7]);
                    log_event_2(EVENT_AVRCP_VOLUME_CHANGE_REQ_RECEIVED, (btif_avrcp_get_cmgrhandler_remDev_hciHandle(channel))&0x3,
                        btif_get_avrcp_cmd_frame(parms)->operands[7]);

                    if (app_bt_device.avrcp_control_rsp[device_id] == NULL)
                        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_control_rsp[device_id]);

                    //app_bt_device.avrcp_control_rsp[device_id]->transId = btif_get_avrcp_cmd_frame(parms)->transId;
                    //app_bt_device.avrcp_control_rsp[device_id]->ctype = BTIF_AVCTP_RESPONSE_ACCEPTED;
                    btif_avrcp_set_control_rsp_cmd(app_bt_device.avrcp_control_rsp[device_id],
                    btif_get_avrcp_cmd_frame(parms)->transId, BTIF_AVCTP_RESPONSE_ACCEPTED);
                    DUMP8("%02x ",  btif_get_avrcp_cmd_frame(parms)->operands,  btif_get_avrcp_cmd_frame(parms)->operandLen);

                    //AVRCP_CtAcceptAbsoluteVolume_Rsp(chnl, app_bt_device.avrcp_control_rsp[device_id],  btif_get_avrcp_cmd_frame(parms)->operands[7]);
                    btif_avrcp_ct_accept_absolute_volume_rsp(channel, app_bt_device.avrcp_control_rsp[device_id],  btif_get_avrcp_cmd_frame(parms)->operands[7]);
                }
                else if (BTIF_AVRCP_OP_CUSTOM_CMD == btif_get_avrcp_cmd_frame(parms)->operands[3])
                {
                    app_AVRCP_CustomCmd_Received(& btif_get_avrcp_cmd_frame(parms)->operands[7],  btif_get_avrcp_cmd_frame(parms)->operandLen - 7);
                    app_AVRCP_sendCustomCmdRsp(device_id, channel, true,  btif_get_avrcp_cmd_frame(parms)->transId);
                }
            }else if ( btif_get_avrcp_cmd_frame(parms)->ctype == BTIF_AVCTP_CTYPE_NOTIFY){
                bt_status_t status;
                APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND AVCTP_CTYPE_NOTIFY\n");
                DUMP8("%02x ",  btif_get_avrcp_cmd_frame(parms)->operands, btif_get_avrcp_cmd_frame(parms)->operandLen);
                if ( btif_get_avrcp_cmd_frame(parms)->operands[7] == BTIF_AVRCP_EID_VOLUME_CHANGED){
                    APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EID_VOLUME_CHANGED transId:%d\n",  btif_get_avrcp_cmd_frame(parms)->transId);
                    if (app_bt_device.avrcp_notify_rsp[device_id] == NULL)
                        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_notify_rsp[device_id]);

                    //app_bt_device.avrcp_notify_rsp[device_id]->transId =  btif_get_avrcp_cmd_frame(parms)->transId;
                    //app_bt_device.avrcp_notify_rsp[device_id]->ctype = BTIF_AVCTP_RESPONSE_INTERIM;
                    btif_avrcp_set_notify_rsp_cmd(app_bt_device.avrcp_notify_rsp[device_id],btif_get_avrcp_cmd_frame(parms)->transId,  BTIF_AVCTP_RESPONSE_INTERIM) ;
                    app_bt_device.volume_report[device_id] = BTIF_AVCTP_RESPONSE_INTERIM;

                    //status = AVRCP_CtGetAbsoluteVolume_Rsp(chnl, app_bt_device.avrcp_notify_rsp[device_id], a2dp_volume_get());
                    status = btif_avrcp_ct_get_absolute_volume_rsp(channel, app_bt_device.avrcp_notify_rsp[device_id], a2dp_volume_get());
                    APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_COMMAND AVRCP_EID_VOLUME_CHANGED nRet:%x\n",status);
                }
            }
            break;
        case BTIF_AVRCP_EVENT_ADV_CMD_TIMEOUT:
            APP_A2DP_TRACE("::avrcp_callback_TG AVRCP_EVENT_ADV_CMD_TIMEOUT device_id=%d,role=%x\n",device_id,btif_get_avrcp_channel_role(channel));
            break;
    }
}

#endif

//#ifdef AVRCP_TRACK_CHANGED
//#define MAX_NUM_LETTER 20
//#endif
int app_tws_control_avrcp_media_reinit(void);

enum AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS {
    AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS_NULL,
    AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS_INIT,
    AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS_ONPROC,
    AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS_NORESP,
    AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS_OK,
};

enum AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS avrcp_getplaystatus_cmdstatus = AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS_NULL;

avrcp_advanced_pdu_t *avrcp_play_status_cmd = NULL;
static osTimerId avrcp_play_status_cmd_timer = NULL;

#define AVRCP_PLAY_STATUS_CMD_TIMER_TIMEOUT_MS        (5000)

static void avrcp_callback_getplaystatus_timeout_handler(void const *param);
osTimerDef (AVRCP_PLAY_STATUS_CMD_TIMER, avrcp_callback_getplaystatus_timeout_handler);

static void avrcp_callback_getplaystatus_timeout_handler(void const *param)
{
    avrcp_getplaystatus_cmdstatus = AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS_NORESP;
}


int avrcp_callback_getplaystatus(void)
{
    if (avrcp_getplaystatus_cmdstatus == AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS_NORESP ||
            avrcp_getplaystatus_cmdstatus == AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS_NULL){
        return -1;
    }

    avrcp_getplaystatus_cmdstatus = AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS_ONPROC;

    osTimerStop(avrcp_play_status_cmd_timer);
    osTimerStart(avrcp_play_status_cmd_timer, AVRCP_PLAY_STATUS_CMD_TIMER_TIMEOUT_MS);

    btif_avrcp_ct_get_play_status(app_bt_device.avrcp_channel[0], avrcp_play_status_cmd);
    return 0;
}

int avrcp_callback_getplaystatus_init(void)
{
    APP_A2DP_TRACE("%s",__func__);

    if (avrcp_play_status_cmd_timer == NULL) {
        avrcp_play_status_cmd_timer = osTimerCreate (osTimer(AVRCP_PLAY_STATUS_CMD_TIMER), osTimerOnce, NULL);
    }

    if (avrcp_play_status_cmd == NULL){
        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&avrcp_play_status_cmd);
    }
#ifdef __TWS__
    app_tws_control_avrcp_media_reinit();
#endif
    avrcp_getplaystatus_cmdstatus = AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS_INIT;
    avrcp_callback_getplaystatus();

    return 0;
}

int avrcp_callback_getplaystatus_deinit(void)
{
    APP_A2DP_TRACE("%s",__func__);

    avrcp_getplaystatus_cmdstatus = AVRCP_CALLBACK_GETPLAYSTATUS_CMDSTATUS_NULL;
    app_tws_control_avrcp_media_reinit();

    return 0;
}

avrcp_media_status_t    media_status = 0xff;
uint8_t avrcp_get_media_status(void)
{
    APP_A2DP_TRACE("%s %d",__func__, media_status);
    return media_status;
}
 uint8_t avrcp_ctrl_music_flag;
void avrcp_set_media_status(uint8_t status)
{
    APP_A2DP_TRACE("%s %d",__func__, status);
    if ((status == 1 && avrcp_ctrl_music_flag == 2) || (status == 2 && avrcp_ctrl_music_flag == 1))
        avrcp_ctrl_music_flag = 0;


    media_status = status;
}





#else
void a2dp_init(void)
{
    for(uint8_t i=0; i<BT_DEVICE_NUM; i++)
    {
        a2dp_channel_num[i] = BTIF_SBC_CHNL_MODE_STEREO;
        app_bt_device.a2dp_state[i]=0;
    }

    app_bt_device.a2dp_state[BT_DEVICE_ID_1]=0;
    app_bt_device.a2dp_play_pause_flag = 0;
    app_bt_device.curr_a2dp_stream_id= BT_DEVICE_ID_1;
}

extern "C" void avrcp_callback(AvrcpChannel *chnl, const AvrcpCallbackParms *Parms)
{
    TRACE("avrcp_callback : chnl %p, Parms %p\n", chnl, Parms);
    TRACE("::Parms->event %d\n", Parms->event);
    switch(Parms->event)
    {
        case AVRCP_EVENT_CONNECT_IND:
            TRACE("::AVRCP_EVENT_CONNECT_IND %d\n", Parms->event);
            AVRCP_ConnectRsp(chnl, 1);
            break;
        case AVRCP_EVENT_CONNECT:
            TRACE("::AVRCP_EVENT_CONNECT %d\n", Parms->event);
            break;
        case AVRCP_EVENT_RESPONSE:
            TRACE("::AVRCP_EVENT_RESPONSE %d\n", Parms->event);

            break;
        case AVRCP_EVENT_PANEL_CNF:
            TRACE("::AVRCP_EVENT_PANEL_CNF %x,%x,%x",
                Parms->p.panelCnf.response,Parms->p.panelCnf.operation,Parms->p.panelCnf.press);
#if 0
            if((Parms->p.panelCnf.response == AVCTP_RESPONSE_ACCEPTED) && (Parms->p.panelCnf.press == TRUE))
            {
                AVRCP_SetPanelKey(chnl,Parms->p.panelCnf.operation,FALSE);
            }
#endif
            break;
    }
}
#endif
//void avrcp_init(void)
//{
//  hal_uart_open(HAL_UART_ID_0,NULL);
//    TRACE("avrcp_init...OK\n");
//}




int store_sbc_buffer(unsigned char *buf, unsigned int len);
int a2dp_audio_sbc_set_frame_info(int rcv_len, int frame_num);

void btapp_send_pause_key(enum BT_DEVICE_ID_T stream_id)
{
    TRACE("btapp_send_pause_key id = %x",stream_id);
    btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[stream_id],BTIF_AVRCP_POP_PAUSE,TRUE);
    btif_avrcp_set_panel_key(app_bt_device.avrcp_channel[stream_id],BTIF_AVRCP_POP_PAUSE,FALSE);
 //   app_bt_device.a2dp_play_pause_flag = 0;
}


void btapp_a2dp_suspend_music(enum BT_DEVICE_ID_T stream_id)
{
    TRACE("btapp_a2dp_suspend_music id = %x",stream_id);

    btapp_send_pause_key(stream_id);
}

extern enum AUD_SAMPRATE_T a2dp_sample_rate;

#define A2DP_TIMESTAMP_TRACE(s,...)
//TRACE(s, ##__VA_ARGS__)

#define A2DP_TIMESTAMP_DEBOUNCE_DURATION (1000)
#define A2DP_TIMESTAMP_MODE_SAMPLE_THRESHOLD (2000)

#define A2DP_TIMESTAMP_SYNC_LIMIT_CNT (100)
#define A2DP_TIMESTAMP_SYNC_TIME_THRESHOLD (60)
#define A2DP_TIMESTAMP_SYNC_SAMPLE_THRESHOLD ((int64_t)a2dp_sample_rate*A2DP_TIMESTAMP_SYNC_TIME_THRESHOLD/1000)

#define RICE_THRESHOLD
#define RICE_THRESHOLD

struct A2DP_TIMESTAMP_INFO_T{
    uint16_t rtp_timestamp;
    uint32_t loc_timestamp;
    uint16_t frame_num;
    int32_t rtp_timestamp_diff_sum;
};

enum A2DP_TIMESTAMP_MODE_T{
    A2DP_TIMESTAMP_MODE_NONE,
    A2DP_TIMESTAMP_MODE_SAMPLE,
    A2DP_TIMESTAMP_MODE_TIME,
};

enum A2DP_TIMESTAMP_MODE_T a2dp_timestamp_mode = A2DP_TIMESTAMP_MODE_NONE;

struct A2DP_TIMESTAMP_INFO_T a2dp_timestamp_pre = {0,0,0};
bool a2dp_timestamp_parser_need_sync = false;

int a2dp_timestamp_parser_init(void)
{
    a2dp_timestamp_mode = A2DP_TIMESTAMP_MODE_NONE;
    a2dp_timestamp_pre.rtp_timestamp = 0;
    a2dp_timestamp_pre.loc_timestamp = 0;
    a2dp_timestamp_pre.frame_num = 0;
    a2dp_timestamp_pre.rtp_timestamp_diff_sum = 0;
    a2dp_timestamp_parser_need_sync = false;
    return 0;
}

int a2dp_timestamp_parser_needsync(void)
{
    a2dp_timestamp_parser_need_sync = true;
    return 0;
}

int a2dp_timestamp_parser_run(uint16_t timestamp, uint16_t framenum)
{
    static int skip_cnt = 0;
    struct A2DP_TIMESTAMP_INFO_T curr_timestamp;
    int skipframe = 0;
    uint16_t rtpdiff;
    int32_t locdiff;
    bool needsave_rtp_timestamp = true;
    bool needsave_loc_timestamp = true;

    curr_timestamp.rtp_timestamp = timestamp;
    curr_timestamp.loc_timestamp = hal_sys_timer_get();
    curr_timestamp.frame_num = framenum;

    switch(a2dp_timestamp_mode) {
        case A2DP_TIMESTAMP_MODE_NONE:

//            TRACE("parser rtp:%d loc:%d num:%d prertp:%d preloc:%d\n", curr_timestamp.rtp_timestamp, curr_timestamp.loc_timestamp, curr_timestamp.frame_num,
//                                                   a2dp_timestamp_pre.rtp_timestamp, a2dp_timestamp_pre.loc_timestamp);
            if (a2dp_timestamp_pre.rtp_timestamp){
                locdiff = curr_timestamp.loc_timestamp - a2dp_timestamp_pre.loc_timestamp;
                if (TICKS_TO_MS(locdiff) > A2DP_TIMESTAMP_DEBOUNCE_DURATION){
                    rtpdiff = curr_timestamp.rtp_timestamp - a2dp_timestamp_pre.rtp_timestamp;
                    if (ABS((int16_t)TICKS_TO_MS(locdiff)-rtpdiff)>A2DP_TIMESTAMP_MODE_SAMPLE_THRESHOLD){
                        a2dp_timestamp_mode = A2DP_TIMESTAMP_MODE_SAMPLE;
                        TRACE("A2DP_TIMESTAMP_MODE_SAMPLE\n");
                    }else{
                        a2dp_timestamp_mode = A2DP_TIMESTAMP_MODE_TIME;
                        TRACE("A2DP_TIMESTAMP_MODE_TIME\n");
                    }
                }else{
                    needsave_rtp_timestamp = false;
                    needsave_loc_timestamp = false;
                }
            }
            break;
        case A2DP_TIMESTAMP_MODE_SAMPLE:
            if (a2dp_timestamp_parser_need_sync){
                skip_cnt++;
                rtpdiff = curr_timestamp.rtp_timestamp - a2dp_timestamp_pre.rtp_timestamp;
                locdiff = curr_timestamp.loc_timestamp - a2dp_timestamp_pre.loc_timestamp;
                a2dp_timestamp_pre.rtp_timestamp_diff_sum += rtpdiff;

                A2DP_TIMESTAMP_TRACE("%d-%d=%d",  curr_timestamp.rtp_timestamp, a2dp_timestamp_pre.rtp_timestamp, rtpdiff);

                A2DP_TIMESTAMP_TRACE("%d-%d=%d",  curr_timestamp.loc_timestamp , a2dp_timestamp_pre.loc_timestamp, locdiff);

                A2DP_TIMESTAMP_TRACE("%d-%d=%d", (int32_t)((int64_t)(TICKS_TO_MS(locdiff))*(uint32_t)a2dp_sample_rate/1000),
                                     a2dp_timestamp_pre.rtp_timestamp_diff_sum,
                                     (int32_t)((TICKS_TO_MS(locdiff)*a2dp_sample_rate/1000) - a2dp_timestamp_pre.rtp_timestamp_diff_sum));


                A2DP_TIMESTAMP_TRACE("A2DP_TIMESTAMP_MODE_SAMPLE SYNC diff:%d cnt:%d\n", (int32_t)((int64_t)(TICKS_TO_MS(locdiff)*a2dp_sample_rate/1000) - a2dp_timestamp_pre.rtp_timestamp_diff_sum), skip_cnt);
                if (((int64_t)(TICKS_TO_MS(locdiff)*a2dp_sample_rate/1000) - a2dp_timestamp_pre.rtp_timestamp_diff_sum) < (int32_t)A2DP_TIMESTAMP_SYNC_SAMPLE_THRESHOLD){
                    TRACE("A2DP_TIMESTAMP_MODE_SAMPLE RESYNC OK cnt:%d\n", skip_cnt);
                    skip_cnt = 0;
                    a2dp_timestamp_parser_need_sync = false;
                }else if (skip_cnt > A2DP_TIMESTAMP_SYNC_LIMIT_CNT){
                    TRACE("A2DP_TIMESTAMP_MODE_SAMPLE RESYNC FORCE END\n");
                    skip_cnt = 0;
                    a2dp_timestamp_parser_need_sync = false;
                }else{
                    needsave_loc_timestamp = false;
                    skipframe = 1;
                }
            }else{
                a2dp_timestamp_pre.rtp_timestamp_diff_sum = 0;
            }
            break;
        case A2DP_TIMESTAMP_MODE_TIME:
            if (a2dp_timestamp_parser_need_sync){
                skip_cnt++;
                rtpdiff = curr_timestamp.rtp_timestamp - a2dp_timestamp_pre.rtp_timestamp;
                locdiff = curr_timestamp.loc_timestamp - a2dp_timestamp_pre.loc_timestamp;
                a2dp_timestamp_pre.rtp_timestamp_diff_sum += rtpdiff;

                A2DP_TIMESTAMP_TRACE("%d/%d/ %d/%d %d\n",  rtpdiff,a2dp_timestamp_pre.rtp_timestamp_diff_sum,
                                            a2dp_timestamp_pre.loc_timestamp, curr_timestamp.loc_timestamp,
                                            TICKS_TO_MS(locdiff));
                A2DP_TIMESTAMP_TRACE("A2DP_TIMESTAMP_MODE_TIME SYNC diff:%d cnt:%d\n", (int32_t)ABS(TICKS_TO_MS(locdiff) - a2dp_timestamp_pre.rtp_timestamp_diff_sum), skip_cnt);
                if (((int64_t)TICKS_TO_MS(locdiff) - a2dp_timestamp_pre.rtp_timestamp_diff_sum) < A2DP_TIMESTAMP_SYNC_TIME_THRESHOLD){
                    TRACE("A2DP_TIMESTAMP_MODE_TIME RESYNC OK cnt:%d\n", skip_cnt);
                    skip_cnt = 0;
                    needsave_loc_timestamp = false;
                    a2dp_timestamp_parser_need_sync = false;
                }else if (skip_cnt > A2DP_TIMESTAMP_SYNC_LIMIT_CNT){
                    TRACE("A2DP_TIMESTAMP_MODE_TIME RESYNC FORCE END\n");
                    skip_cnt = 0;
                    a2dp_timestamp_parser_need_sync = false;
                }else{
                    needsave_loc_timestamp = false;
                    skipframe = 1;
                }
            }else{
                a2dp_timestamp_pre.rtp_timestamp_diff_sum = 0;
            }
            break;
    }

    if (needsave_rtp_timestamp){
        a2dp_timestamp_pre.rtp_timestamp = curr_timestamp.rtp_timestamp;
    }

    if (needsave_loc_timestamp){
        a2dp_timestamp_pre.loc_timestamp = curr_timestamp.loc_timestamp;
    }

    return skipframe;
}

static struct BT_DEVICE_ID_DIFF stream_id_flag;

#if defined(A2DP_LHDC_ON)
uint8_t bits_depth;
uint8_t bt_sbc_player_get_bitsDepth(void){
    if (app_bt_device.sample_bit[stream_id_flag.id] != bits_depth) {
        /* code */
        bits_depth = app_bt_device.sample_bit[stream_id_flag.id];
    }
    return bits_depth;
}
#endif


#ifdef __BT_ONE_BRING_TWO__


uint8_t  a2dp_stream_id_distinguish(a2dp_stream_t *Stream , uint8_t event_type)
{
    uint8_t found_device_id = BT_DEVICE_NUM;
    if(Stream == app_bt_device.a2dp_connected_stream[BT_DEVICE_ID_1])
    {
        found_device_id = BT_DEVICE_ID_1;
        stream_id_flag.id = BT_DEVICE_ID_1;
    }
    else if(Stream == app_bt_device.a2dp_connected_stream[BT_DEVICE_ID_2])
    {
        found_device_id = BT_DEVICE_ID_2;
        stream_id_flag.id = BT_DEVICE_ID_2;
    }else/* if(event_type == A2DP_EVENT_STREAM_CLOSED)*/{
        btif_remote_device_t * remDev = 0;
        btif_remote_device_t * connected_remDev[BT_DEVICE_NUM];
        remDev = btif_a2dp_get_remote_device(Stream);
        connected_remDev[0] = btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[BT_DEVICE_ID_1]);
        connected_remDev[1] = btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[BT_DEVICE_ID_2]);
        if((connected_remDev[0] == remDev) && (remDev!= 0)){
            stream_id_flag.id = BT_DEVICE_ID_1;
            found_device_id = BT_DEVICE_ID_1;
        }
        else if((connected_remDev[1] == remDev) && (remDev !=0)){
            stream_id_flag.id = BT_DEVICE_ID_2;
            found_device_id = BT_DEVICE_ID_2;
        }
    }
    return found_device_id;
}

uint8_t POSSIBLY_UNUSED a2dp_get_streaming_id(void)
{
    uint8_t nRet = 0;
    if( btif_a2dp_get_stream_state(app_bt_device.a2dp_connected_stream[BT_DEVICE_ID_1])  == BTIF_AVDTP_STRM_STATE_STREAMING)
        nRet |= 1<<0;
    if(btif_a2dp_get_stream_state(app_bt_device.a2dp_connected_stream[BT_DEVICE_ID_2]) == BTIF_AVDTP_STRM_STATE_STREAMING)
        nRet |= 1<<1;
    return nRet;
}

uint8_t a2dp_stream_locate_the_connected_dev_id(a2dp_stream_t *Stream)
{
    for (uint8_t index = 0;index < BT_DEVICE_NUM;index++)
    {
        if ((app_bt_device.a2dp_stream[index]->a2dp_stream) == Stream)
        {
            TRACE("Get a2dp stream index %d", index);
            return index;
        }

    #if defined(A2DP_AAC_ON)
        if ((app_bt_device.a2dp_aac_stream[index]->a2dp_stream) == Stream)
        {
            TRACE("Get a2dp aac stream index %d", index);
            return index;
        }
    #endif
#if defined(A2DP_LHDC_ON)
        if ((app_bt_device.a2dp_lhdc_stream[index]->a2dp_stream) == Stream)
        {
            TRACE("Get a2dp lhdc stream index %d", index);
            return index;
        }
#endif

#if defined(A2DP_SCALABLE_ON)
        if ((app_bt_device.a2dp_scalable_stream[index]->a2dp_stream) == Stream)
        {
            TRACE("Get a2dp scalable stream index %d", index);
            return index;
        }
#endif
	
#if defined(A2DP_LDAC_ON)
		if ((app_bt_device.a2dp_ldac_stream[index]->a2dp_stream) == Stream)
		{
			TRACE("Get a2dp ldac stream index %d", index);
			return index;
		}
#endif

    }

    ASSERT(false, "Connected to an non-existing a2dp stream instance.");

    return 0;
}

void a2dp_stream_push_connected_stream(a2dp_stream_t *Stream)
{

    uint8_t reserved_device = BT_DEVICE_NUM;
    reserved_device = a2dp_stream_locate_the_connected_dev_id(Stream);
    app_bt_device.a2dp_connected_stream[reserved_device] = Stream;
}

void a2dp_stream_push_configed_stream(a2dp_stream_t *Stream,btif_remote_device_t *rem)
{
    uint8_t reserved_device = BT_DEVICE_NUM;
	reserved_device = a2dp_stream_locate_the_connected_dev_id(Stream);
	app_bt_device.a2dp_outconfiged_stream[reserved_device] = Stream;
    app_bt_device.a2dp_outconfiged_rem[reserved_device] = rem;

}

#ifdef A2DP_AAC_ON
uint8_t is_aac_stream(a2dp_stream_t *Stream)
{
    if((Stream == app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream)
        || (Stream == app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_2]->a2dp_stream))
        return TRUE;
    else
        return FALSE;
}
#endif
#endif


#if defined( __BTIF_EARPHONE__) && defined(__BTIF_BT_RECONNECT__)

#ifdef __BT_ONE_BRING_TWO__
extern btif_device_record_t record2_copy;
extern uint8_t record2_avalible;
#endif

#endif

#if defined(A2DP_LHDC_ON)
void a2dp_lhdc_config(uint8_t * elements){
    //uint8_t * elements = &(Info->p.configReq->codec.elements[0]);
    uint32_t vendor_id = (uint32_t)elements[0];
    vendor_id |= ((uint32_t)elements[1]) << 8;
    vendor_id |= ((uint32_t)elements[2]) << 16;
    vendor_id |= ((uint32_t)elements[3]) << 24;
    uint16_t codec_id = (uint16_t)elements[4];
    codec_id |= ((uint16_t)elements[5]) << 8;
    uint8_t config = elements[6];
    TRACE("##codecType: LHDC Codec, config value = 0x%02x, elements[6]=0x%02x elements[7]=0x%02x\n", BTIF_A2DP_LHDC_SR_DATA(config), elements[6], elements[7]);
    if (vendor_id == BTIF_A2DP_LHDC_VENDOR_ID && codec_id == BTIF_A2DP_LHDC_CODEC_ID) {
        TRACE("Vendor ID = 0x%08x, Codec ID = 0x%04x, LHDC Codec\n", vendor_id, codec_id);
        switch (BTIF_A2DP_LHDC_SR_DATA(config)) {
            case BTIF_A2DP_LHDC_SR_96000:
            app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_96;
            TRACE("%s:CodecCfg sample_rate 96000\n", __func__);
            break;
            case BTIF_A2DP_LHDC_SR_48000:
            app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_48;
            TRACE("%s:CodecCfg sample_rate 48000\n", __func__);
            break;
            case BTIF_A2DP_LHDC_SR_44100:
            app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_44;
            TRACE("%s:CodecCfg sample_rate 44100\n", __func__);
            break;
        }
        switch (BTIF_A2DP_LHDC_FMT_DATA(config)) {
            case BTIF_A2DP_LHDC_FMT_16:
            app_bt_device.sample_bit[stream_id_flag.id] = 16;
            TRACE("%s:CodecCfg bits per sampe = 16", __func__);
            break;
            case BTIF_A2DP_LHDC_FMT_24:
            TRACE("%s:CodecCfg bits per sampe = 24", __func__);
            app_bt_device.sample_bit[stream_id_flag.id] = 24;
            break;
        }
        
        if (elements[7]&BTIF_A2DP_LHDC_LLC_ENABLE){
            app_bt_device.a2dp_lhdc_llc[stream_id_flag.id] = true;;
        }else{
            app_bt_device.a2dp_lhdc_llc[stream_id_flag.id] = false;;
        }
    }
}

uint8_t a2dp_lhdc_config_llc_get(void)
{
    return app_bt_device.a2dp_lhdc_llc[stream_id_flag.id]; 
}

#endif

#if defined(A2DP_SCALABLE_ON)
void a2dp_scalable_config(uint8_t * elements){
    //uint8_t * elements = &(Info->p.configReq->codec.elements[0]);
    uint32_t vendor_id = (uint32_t)elements[0];
    vendor_id |= ((uint32_t)elements[1]) << 8;
    vendor_id |= ((uint32_t)elements[2]) << 16;
    vendor_id |= ((uint32_t)elements[3]) << 24;
    uint16_t codec_id = (uint16_t)elements[4];
    codec_id |= ((uint16_t)elements[5]) << 8;
    uint8_t config = elements[6];
    TRACE("##codecType: Scalable Codec, config value = 0x%02x, elements[6]=0x%02x\n", BTIF_A2DP_SCALABLE_SR_DATA(config), elements[6]);
    if (vendor_id == BTIF_A2DP_SCALABLE_VENDOR_ID && codec_id == BTIF_A2DP_SCALABLE_CODEC_ID) {
        TRACE("Vendor ID = 0x%08x, Codec ID = 0x%04x, Scalable Codec\n", vendor_id, codec_id);
        switch (BTIF_A2DP_SCALABLE_SR_DATA(config)) {
            case BTIF_A2DP_SCALABLE_SR_96000:
            app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_96;
            TRACE("%s:CodecCfg sample_rate 96000\n", __func__);
            break;
            case BTIF_A2DP_SCALABLE_SR_48000:
            app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_48;
            TRACE("%s:CodecCfg sample_rate 48000\n", __func__);
            break;
            case BTIF_A2DP_SCALABLE_SR_44100:
            app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_44;
            TRACE("%s:CodecCfg sample_rate 44100\n", __func__);
            break;
            case BTIF_A2DP_SCALABLE_SR_32000:
            app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_32;
            TRACE("%s:CodecCfg sample_rate 32000\n", __func__);
            break;
        }
        switch (BTIF_A2DP_SCALABLE_FMT_DATA(config)) {
            case BTIF_A2DP_SCALABLE_FMT_16:
            app_bt_device.sample_bit[stream_id_flag.id] = 16;
            TRACE("%s:CodecCfg bits per sampe = 16", __func__);
            break;
            case BTIF_A2DP_SCALABLE_FMT_24:
            app_bt_device.sample_bit[stream_id_flag.id] = 24;
            TRACE("%s:CodecCfg bits per sampe = 24", __func__);
            if (app_bt_device.sample_rate[stream_id_flag.id] != A2D_SBC_IE_SAMP_FREQ_96) {
                app_bt_device.sample_bit[stream_id_flag.id] = 16;
                TRACE("%s:CodeCfg reset bit per sample to 16 when samplerate is not 96k", __func__);
            }
            break;
        }
    }
}
#endif

#if defined(A2DP_LDAC_ON)
int ldac_decoder_sf = 0;
int ldac_decoder_cm = 0;
void a2dp_ldac_config(uint8_t * elements){
    //uint8_t * elements = &(Info->p.configReq->codec.elements[0]);
    uint32_t vendor_id = (uint32_t)elements[0];
    vendor_id |= ((uint32_t)elements[1]) << 8;
    vendor_id |= ((uint32_t)elements[2]) << 16;
    vendor_id |= ((uint32_t)elements[3]) << 24;
    uint16_t codec_id = (uint16_t)elements[4];
    codec_id |= ((uint16_t)elements[5]) << 8;
    uint8_t sf_config = elements[6];
	uint8_t cm_config = elements[7];
    TRACE("##codecType: LDAC Codec, config value = 0x%02x, elements[6]=0x%02x\n", A2DP_LDAC_SR_DATA(sf_config), elements[6]);
	TRACE("##codecType: LDAC Codec, config value = 0x%02x, elements[7]=0x%02x\n", A2DP_LDAC_CM_DATA(cm_config), elements[7]);
	TRACE("Vendor ID = 0x%08x, Codec ID = 0x%04x, LHDC Codec\n", vendor_id, codec_id);
    if (vendor_id == A2DP_LDAC_VENDOR_ID && codec_id == A2DP_LDAC_CODEC_ID) {
        switch (A2DP_LDAC_SR_DATA(sf_config)) {
            case A2DP_LDAC_SR_96000:
            app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_96;
			ldac_decoder_sf = 96000;
            TRACE("%s:ldac CodecCfg sample_rate 96000\n", __func__);
            break;
            case A2DP_LDAC_SR_48000:
            app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_48;
			ldac_decoder_sf = 48000;
            TRACE("%s:ldac CodecCfg sample_rate 48000\n", __func__);
            break;
            case A2DP_LDAC_SR_44100:
            app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_44;
			ldac_decoder_sf = 44100;
            TRACE("%sldac :CodecCfg sample_rate 44100\n", __func__);
            break;
			/*case A2DP_LDAC_SR_88200:
            app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_88;
			ldac_decoder_sf = 88200;
            TRACE("%s:ldac CodecCfg sample_rate 88200\n", __func__);
            break;*/
        }
        switch (A2DP_LDAC_CM_DATA(cm_config)) {
            case A2DP_LDAC_CM_MONO:
            app_bt_device.channel_mode = LDACBT_CHANNEL_MODE_MONO;
            TRACE("%s:ldac CodecCfg A2DP_LDAC_CM_MONO", __func__);
            break;
            case A2DP_LDAC_CM_DUAL:
            TRACE("%s:ldac CodecCfg A2DP_LDAC_CM_DUAL", __func__);
            app_bt_device.channel_mode = LDACBT_CHANNEL_MODE_DUAL_CHANNEL;
            break;
			case A2DP_LDAC_CM_STEREO:
            TRACE("%s:ldac ldac CodecCfg A2DP_LDAC_CM_STEREO", __func__);
            app_bt_device.channel_mode = LDACBT_CHANNEL_MODE_STEREO;
            break;
        }
    }
}

int channel_mode;
int bt_ldac_player_get_channelmode(void){
     if (app_bt_device.channel_mode != channel_mode) {
        /* code */
        channel_mode = app_bt_device.channel_mode;
    }
    return channel_mode;
}
int bt_get_ladc_sample_rate(void){

	return ldac_decoder_sf;
}

#endif



extern void app_bt_profile_connect_manager_a2dp(enum BT_DEVICE_ID_T id, a2dp_stream_t *Stream, const a2dp_callback_parms_t *Info);

#ifdef __BT_ONE_BRING_TWO__


void a2dp_dual_slave_setup_during_sco(enum BT_DEVICE_ID_T currentId)
{
    if (app_bt_device.a2dp_state[BT_DEVICE_ID_1] ||
        app_bt_device.a2dp_state[BT_DEVICE_ID_2])
    {
        uint8_t activeDevice = currentId;
        uint8_t idleDevice = (BT_DEVICE_ID_1 == activeDevice)?BT_DEVICE_ID_2:BT_DEVICE_ID_1;

        btif_remote_device_t* activeRem =
            btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[activeDevice]);
        btif_remote_device_t* idleRem =
            btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[idleDevice]);


        TRACE("activeRem 0x%x idleRem 0x%x", activeRem, idleRem);
        if (idleRem && activeRem)
        {
            btdrv_enable_dual_slave_configurable_slot_mode(true,
                btif_me_get_remote_device_hci_handle(activeRem),
                btif_me_get_remote_device_role(activeRem),
                btif_me_get_remote_device_hci_handle(idleRem),
                btif_me_get_remote_device_role(idleRem));
        }
        else
        {
            btdrv_enable_dual_slave_configurable_slot_mode(false,
                0x80,
                BTIF_BCR_SLAVE,
                0x81,
                BTIF_BCR_SLAVE);
        }
    }
    else
    {
        btdrv_enable_dual_slave_configurable_slot_mode(false,
            0x80,
            BTIF_BCR_SLAVE,
            0x81,
            BTIF_BCR_SLAVE);
    }
}


void a2dp_dual_slave_handling_refresh(void)
{
    if (app_bt_device.a2dp_state[BT_DEVICE_ID_1] ||
        app_bt_device.a2dp_state[BT_DEVICE_ID_2])
    {
        TRACE("current a2dp streaming id %d", app_bt_device.curr_a2dp_stream_id);
        uint8_t activeDevice = app_bt_device.curr_a2dp_stream_id;
        uint8_t idleDevice = (BT_DEVICE_ID_1 == activeDevice)?BT_DEVICE_ID_2:BT_DEVICE_ID_1;

        btif_remote_device_t* activeRem =
            btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[activeDevice]);
        btif_remote_device_t* idleRem =
            btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[idleDevice]);
        TRACE("activeRem 0x%x idleRem 0x%x", activeRem, idleRem);
        if (idleRem && activeRem)
        {
            btdrv_enable_dual_slave_configurable_slot_mode(true,
            btif_me_get_remote_device_hci_handle(activeRem),
            btif_me_get_remote_device_role(activeRem),
            btif_me_get_remote_device_hci_handle(idleRem),
            btif_me_get_remote_device_role(idleRem));
        }
        else
        {
            btdrv_enable_dual_slave_configurable_slot_mode(false,
                0x80,
                BTIF_BCR_SLAVE,
                0x81,
                BTIF_BCR_SLAVE);
        }
    }
    else
    {
        btdrv_enable_dual_slave_configurable_slot_mode(false,
            0x80,
            BTIF_BCR_SLAVE,
            0x81,
            BTIF_BCR_SLAVE);
    }
}

void a2dp_update_music_link(void)
{
        TRACE("current a2dp streaming id %d", app_bt_device.curr_a2dp_stream_id);
        uint8_t activeDevice = app_bt_device.curr_a2dp_stream_id;
        uint8_t idleDevice = (BT_DEVICE_ID_1 == activeDevice)?BT_DEVICE_ID_2:BT_DEVICE_ID_1;

        btif_remote_device_t* activeRem =
            btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[activeDevice]);
        btif_remote_device_t* idleRem =
            btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[idleDevice]);
        TRACE("activeRem 0x%x idleRem 0x%x", activeRem, idleRem);
        if (idleRem && activeRem)
        {
            bt_dev_music_link_config(
            btif_me_get_remote_device_hci_handle(activeRem),
            btif_me_get_remote_device_role(activeRem),
            btif_me_get_remote_device_hci_handle(idleRem),
            btif_me_get_remote_device_role(idleRem));
        }
        else if(activeRem)
        {
            bt_dev_music_link_config(
                btif_me_get_remote_device_hci_handle(activeRem),
                btif_me_get_remote_device_role(activeRem),
                0xff,
                BTIF_BCR_SLAVE);
        }

}

#endif

uint8_t a2dp_get_latest_paused_device(void)
{
    return app_bt_device.latestPausedDevId;
}

#ifdef __BT_ONE_BRING_TWO__
#if !defined(__BT_SELECT_PROF_DEVICE_ID__)
static void a2dp_avdtp_stream_state_lock_management(a2dp_stream_t *Stream, const a2dp_callback_parms_t *info)
{
    btif_a2dp_callback_parms_t * Info = (btif_a2dp_callback_parms_t *)info;

    static uint8_t last_locked_configedid = 0xff;
    if(Info->error!=BTIF_AVDTP_ERR_NO_ERROR)
        return;
    switch(Info->event)
    {

        case BTIF_A2DP_AVDTP_EVENT_SET_CFG_CNF:  //-->set config
                LOG_D("outgoing set cfg");
                //record the outgoing configed stream and rem
                a2dp_stream_push_configed_stream(Stream,Info->remDev);
        break;
    
        case BTIF_A2DP_EVENT_STREAM_RECONFIG_CNF: //reconfig-->  
        case BTIF_A2DP_EVENT_STREAM_RECONFIG_IND: //rsp reconfig <--       
            
        
		case BTIF_A2DP_EVENT_STREAM_OPEN_IND:   //set configed <--
        case BTIF_A2DP_EVENT_STREAM_OPEN:        //after avdtp open rsp  AVDTP medio connected
        
            LOG_E("evt %d STREAM %x ",Info->event,Stream);
            btif_a2dp_lock_same_deviceid_endpoint(Stream,Info->remDev);
        break;
        case BTIF_A2DP_EVENT_AVDTP_CLOSE_IND:
        case BTIF_A2DP_EVENT_STREAM_CLOSED: //unlock the stream state before the connected stream been released
        
             LOG_E("evt %d STREAM %x ",Info->event,Stream);
             btif_a2dp_unlock_same_deviceid_endpoint(Stream,Info->remDev);
        break;
        case BTIF_A2DP_EVENT_AVDTP_DISCOVER_IND://<-- discover 
            
            LOG_E("DISCOVER_IND");
            btif_a2dp_unlock_the_connected_stream_byRemdev(Info->remDev);

            //lock the other id
            last_locked_configedid =btif_a2dp_trylock_the_other_id_by_configedid(Info->remDev);
        break;
        case BTIF_A2DP_EVENT_AVDTP_DISCOVER_RSP:
            LOG_D("disc rsp");
                btif_a2dp_unlock_deviceid_endpoint(last_locked_configedid);
        break;
        default:

            break;
    }
}
#endif
#endif

#if defined(__BT_SELECT_PROF_DEVICE_ID__)
static void _a2dp_select_stream(a2dp_stream_t *Stream, a2dp_callback_parms_t *info)
{
    uint32_t i = 0, stream_is_on_device_id = 0;
    a2dp_stream_t *dst_stream = NULL;
    btif_remote_device_t *matching_remdev = NULL, *wanted_remdev = NULL;
    // 1. find already connected profile, if any other one profile was conncected, select that device id
    // 2. if no other profile connected, select min device id
    btif_a2dp_set_dst_stream(info, Stream);
    wanted_remdev = btif_a2dp_get_remote_device_from_cbparms(Stream, info);
    for(i = 0 ; i < BT_DEVICE_NUM ; i++) {
        // Other profile connected
        stream_is_on_device_id = app_bt_a2dp_is_stream_on_device_id(Stream, i);
        if (app_bt_is_any_profile_connected(i)) {
            matching_remdev = app_bt_get_connected_profile_remdev(i);
            LOG_D("device_id=%d, a2dp_select_stream : remdev=0x%x:0x%x.", i, wanted_remdev, matching_remdev);
            LOG_D("device_id=%d, a2dp_select_stream : other_profile_connected.", i);
            if (wanted_remdev == matching_remdev) {
                LOG_D("device_id=%d, a2dp_select_stream : same_remdev.", i);
                // SWITCH COND 1 : same remdev but cross device id to select stream
                if (!stream_is_on_device_id) {
                    app_bt_a2dp_find_same_unused_stream(Stream, &dst_stream, i);
                    btif_a2dp_set_dst_stream(info, dst_stream);
                    LOG_D("device_id=%d, a2dp_select_stream : Switch_cond 1 : 0x%x:0x%x", i, Stream, dst_stream);
                }
                else {
                    LOG_D("device_id=%d, a2dp_select_stream : same device id, no_need_to_switch.", i);
                }
                break;
            }
            else {
                LOG_D("device_id=%d, a2dp_select_stream : different_remdev.", i);
                if (stream_is_on_device_id) {
                    LOG_D("device_id=%d, a2dp_select_stream : error : different_remdev_but_same_device_id, see next device id.", i);
                }
                else {
                    LOG_D("device_id=%d, a2dp_select_stream : good : different_remdev_different_device_id, see next device id.", i);
                }
            }
        }
        else {
            LOG_D("device_id=%d, a2dp_select_stream : other_profile_not_connected.", i);
            // first found idle device id is min device id we want
            // Assume : other profile will use device id ascending
            // TODO to keep other profile use device id ascending
            if (!stream_is_on_device_id) {
                app_bt_a2dp_find_same_unused_stream(Stream, &dst_stream, i);
                btif_a2dp_set_dst_stream(info, dst_stream);
                LOG_D("device_id=%d, a2dp_select_stream : Switch_cond 2 : 0x%x:0x%x", i, Stream, dst_stream);
            }
            else {
                LOG_D("device_id=%d, a2dp_select_stream : same device id, no_need_to_switch.", i);
            }
            break;
        }
    }
}
#endif

extern "C" void a2dp_callback(a2dp_stream_t *Stream, const a2dp_callback_parms_t *info)
{
    int header_len = 0;
    btif_avdtp_media_header_t header;
    uint8_t distinguish_found_id = BT_DEVICE_NUM;
    btif_a2dp_callback_parms_t * Info = (btif_a2dp_callback_parms_t *)info;
    btif_avdtp_codec_t  *codec =  btif_a2dp_get_stream_codec( Stream);

#if defined(__BTIF_BT_RECONNECT__)
    static btif_avdtp_codec_t   setconfig_codec;
    static u8 tmp_element[10];
#endif
#ifdef __A2DP_AVDTP_CP__
    static AvdtpContentProt   setconfig_cp[BT_DEVICE_NUM];
#endif

#ifdef __BT_ONE_BRING_TWO__

    if (Info->event == BTIF_A2DP_EVENT_STREAM_SELECT) {
#if defined(__BT_SELECT_PROF_DEVICE_ID__)
        _a2dp_select_stream(Stream, (a2dp_callback_parms_t *)info);
#endif
        return;
    }

    if(Info->event== BTIF_A2DP_EVENT_STREAM_OPEN){
     // BTIF_A2DP_EVENT_STREAM_OPEN means that the
     //  AVDTP opened   the avdtp media has connected 
        a2dp_stream_push_connected_stream(Stream);
    }
#if !defined(__BT_SELECT_PROF_DEVICE_ID__)
    a2dp_avdtp_stream_state_lock_management(Stream,info);
#endif
    if (Info->event == BTIF_A2DP_EVENT_STREAM_SELECT) {
        return;
    }
    distinguish_found_id = a2dp_stream_id_distinguish(Stream,Info->event);
#else
    stream_id_flag.id = BT_DEVICE_ID_1;
    app_bt_device.a2dp_connected_stream[BT_DEVICE_ID_1] = Stream;
    distinguish_found_id = BT_DEVICE_ID_1;
#endif

    if (BTIF_A2DP_EVENT_STREAM_DATA_IND != Info->event)
    {
        TRACE("Get A2DP event %d", Info->event);
    }
#ifdef __BT_ONE_BRING_TWO__

    enum BT_DEVICE_ID_T anotherDevice = (BT_DEVICE_ID_1 == stream_id_flag.id)?BT_DEVICE_ID_2:BT_DEVICE_ID_1;
#endif
    switch(Info->event) {
       case BTIF_A2DP_EVENT_AVDTP_DISCONNECT:
            TRACE("::A2DP_EVENT_AVDTP_DISCONNECT %d st = 0x%x,id %d\n", Info->event, Stream,stream_id_flag.id);
            break;
        case BTIF_A2DP_EVENT_AVDTP_CONNECT:
            TRACE("::A2DP_EVENT_AVDTP_CONNECT %d st = 0x%x id = %d\n", Info->event, Stream,stream_id_flag.id);

#ifdef BT_USB_AUDIO_DUAL_MODE
            if(!btusb_is_bt_mode())
            {
                btusb_btaudio_close(false);
            }
#endif
            break;
        case BTIF_A2DP_EVENT_STREAM_OPEN:
            TRACE("::A2DP_EVENT_STREAM_OPEN stream_id:%d, sample_rate codec.elements 0x%x\n", stream_id_flag.id,Info->p.configReq->codec.elements[0]);
        #ifdef __BT_ONE_BRING_TWO__
            app_bt_device.latestPausedDevId = anotherDevice;
        #else
            app_bt_device.latestPausedDevId = BT_DEVICE_ID_1;
        #endif

        #ifdef _GFPS_
            app_exit_fastpairing_mode();
        #endif

            app_bt_clear_connecting_profiles_state(stream_id_flag.id);

        #ifdef __BT_ONE_BRING_TWO__
            a2dp_to_bond_avrcp_with_stream(Stream, stream_id_flag.id);
        #endif

#ifdef __A2DP_AVDTP_CP__
            A2DP_SecurityControlReq(Stream,(U8 *)&a2dp_avdtpCp_securityData[stream_id_flag.id][0],1);
#endif
            a2dp_timestamp_parser_init();
            app_bt_stream_volume_ptr_update((uint8_t *) btif_me_get_remote_device_bdaddr(btif_a2dp_get_stream_conn_remDev(Stream)));
//            app_bt_stream_a2dpvolume_reset();
            TRACE("codecType 0x%d\n", Info->p.configReq->codec.codecType);
#if defined(A2DP_LHDC_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                TRACE("##codecType: LHDC Codec, Element length = %d, AVDTP_MAX_CODEC_ELEM_SIZE = %d\n", Info->p.configReq->codec.elemLen, BTIF_AVDTP_MAX_CODEC_ELEM_SIZE);
                app_bt_device.codec_type[stream_id_flag.id] = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
                a2dp_lhdc_config(&(Info->p.configReq->codec.elements[0]));
            }else
#endif
#if defined(A2DP_AAC_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
#ifdef __A2DP_AVDTP_CP__
                A2DP_SecurityControlReq(Stream,(U8 *)&a2dp_avdtpCp_aac_securityData[stream_id_flag.id][0],1);
#endif
                TRACE("::A2DP_EVENT_STREAM_OPEN stream_id:%d, aac sample_rate codec.elements 0x%x\n", stream_id_flag.id,Info->p.configReq->codec.elements[1]);
                app_bt_device.codec_type[stream_id_flag.id] = BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC;
                app_bt_device.sample_bit[stream_id_flag.id] = 16;
                // convert aac sample_rate to sbc sample_rate format
                if (Info->p.configReq->codec.elements[1] & BTIF_A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100) {
                    TRACE("::A2DP_EVENT_STREAM_OPEN stream_id:%d, aac sample_rate 44100\n", stream_id_flag.id);
                    app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_44;
                }
                else if (Info->p.configReq->codec.elements[2] & BTIF_A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000) {
                    TRACE("::A2DP_EVENT_STREAM_OPEN stream_id:%d, aac sample_rate 48000\n", stream_id_flag.id);
                    app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_48;
                }
                else {
                    TRACE("::A2DP_EVENT_STREAM_OPEN stream_id:%d, aac sample_rate not 48000 or 44100, set to 44100\n", stream_id_flag.id);
                    app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_44;
                }

                if (Info->p.configReq->codec.elements[2] & BTIF_A2DP_AAC_OCTET2_CHANNELS_1){
                    a2dp_channel_num[stream_id_flag.id] = 1;
                }else{
                    a2dp_channel_num[stream_id_flag.id] = 2;
                }                
            }
            else
#endif
#if defined(A2DP_SCALABLE_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                TRACE("##codecType scalable\n");
                app_bt_device.codec_type[stream_id_flag.id] = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
                //0x75 0x00 0x00 0x00Vid
                //0x03 0x01   Codec id
                if(Info->p.codec->elements[0]==0x75&&
                   Info->p.codec->elements[4]==0x03&&
                   Info->p.codec->elements[5]==0x01
                 ){
                    setconfig_codec.elements = (U8*)&a2dp_codec_scalable_elements;
                 }
                else{
                    if(Info->p.codec->pstreamflags!=NULL)
                    Info->p.codec->pstreamflags[0] &= ~APP_A2DP_STRM_FLAG_QUERY_CODEC;
                    else{
                        ASSERT(false, "pstreamflags not init ..");
                    }

                    a2dp_channel_num[stream_id_flag.id]  = 2;
                }

            }else
#endif
#if defined(A2DP_LDAC_ON)
			if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
				TRACE("##codecType: LDAC Codec, Element length = %d, AVDTP_MAX_CODEC_ELEM_SIZE = %d\n", Info->p.configReq->codec.elemLen, BTIF_AVDTP_MAX_CODEC_ELEM_SIZE);
				app_bt_device.codec_type[stream_id_flag.id] = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
                //
                //Codec Info Element: 0x 2d 01 00 00 aa 00 34 07
                                //
                if(Info->p.codec->elements[0]==0x2d){
                    app_bt_device.sample_bit[stream_id_flag.id] = 16;
                    a2dp_ldac_config(&(Info->p.configReq->codec.elements[0]));

                 }
                else{
                    if(Info->p.codec->pstreamflags!=NULL)
                    Info->p.codec->pstreamflags[0] &= ~APP_A2DP_STRM_FLAG_QUERY_CODEC;
                    else{
                        ASSERT(false, "pstreamflags not init ..");
                    }

                    a2dp_channel_num[stream_id_flag.id]  = 2;
                }

			}else
#endif

            {
                TRACE("app_bt_device.sample_rate::elements[0] %d BITPOOL:%d/%d %02x/%02x\n", Info->p.codec->elements[0], 
                                                                                             Info->p.codec->elements[2],
                                                                                             Info->p.codec->elements[3],
                                                                                             Info->p.codec->elements[2],
                                                                                             Info->p.codec->elements[3]);

                app_bt_device.codec_type[stream_id_flag.id] = BTIF_AVDTP_CODEC_TYPE_SBC;
                app_bt_device.sample_bit[stream_id_flag.id] = 16;
#if defined(A2DP_LDAC_ON)
                app_bt_device.sample_rate[stream_id_flag.id] = (Info->p.configReq->codec.elements[0] & (A2D_SBC_IE_SAMP_FREQ_MSK == 0xff ? (A2D_SBC_IE_SAMP_FREQ_MSK & 0xf0) : A2D_SBC_IE_SAMP_FREQ_MSK));
#else            
                app_bt_device.sample_rate[stream_id_flag.id] = (Info->p.configReq->codec.elements[0] & A2D_SBC_IE_SAMP_FREQ_MSK);
#endif
                
                if(Info->p.configReq->codec.elements[0] & A2D_SBC_IE_CH_MD_MONO)
                    a2dp_channel_num[stream_id_flag.id]  = 1;
                else
                    a2dp_channel_num[stream_id_flag.id] = 2;  

            }
            app_bt_device.a2dp_state[stream_id_flag.id] = 1;
#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
			if(app_bt_device.src_or_snk==BT_DEVICE_SRC)
			{
				if((Info->p.configReq->codec.elements[1] & A2D_SBC_IE_SUBBAND_MSK) == A2D_SBC_IE_SUBBAND_4)
					TRACE("numSubBands is only support 8!");
				a2dp_source.sample_rate =  bt_parse_sbc_sample_rate(btif_a2dp_get_stream_codec_element(Stream,0));
				//TRACE("::AVRCP_Connect\n", __LINE__);
				//AVRCP_Connect(&app_bt_device.avrcp_channel[stream_id_flag.id], &Stream->stream.conn.remDev->bdAddr);
			}
			else
			{
				btif_avrcp_connect(app_bt_device.avrcp_channel[stream_id_flag.id],  btif_me_get_remote_device_bdaddr(btif_a2dp_get_stream_conn_remDev(Stream)));
			}
#else
            btif_avrcp_connect(app_bt_device.avrcp_channel[stream_id_flag.id],  btif_me_get_remote_device_bdaddr(btif_a2dp_get_stream_conn_remDev(Stream)));
#endif
            app_bt_profile_connect_manager_a2dp(stream_id_flag.id, Stream, (a2dp_callback_parms_t *)Info);
#ifdef __BT_ONE_BRING_TWO__

            if(app_bt_device.a2dp_connected_stream[anotherDevice] &&
               (btif_a2dp_get_stream_state( app_bt_device.a2dp_connected_stream[anotherDevice]) != BTIF_AVDTP_STRM_STATE_STREAMING))
            {
                a2dp_set_cur_stream(stream_id_flag.id);
            }else if(!app_bt_device.a2dp_connected_stream[anotherDevice]){
                a2dp_set_cur_stream(stream_id_flag.id);
            }
#else
            a2dp_set_cur_stream(stream_id_flag.id);
#endif

#ifdef __BT_ONE_BRING_TWO__
            a2dp_dual_slave_handling_refresh();
#endif
            break;
        case BTIF_A2DP_EVENT_STREAM_OPEN_IND:
            TRACE("::A2DP_EVENT_STREAM_OPEN_IND %d\n", Info->event);
            btif_a2dp_open_stream_rsp(Stream, BTIF_A2DP_ERR_NO_ERROR, BTIF_AVDTP_SRV_CAT_MEDIA_TRANSPORT);
#ifdef __A2DP_AVDTP_CP__
            if(Info->p.configReq->cp.cpType==BTIF_AVDTP_CP_TYPE_SCMS_T)
            {
                app_bt_device.avdtp_cp[stream_id_flag.id] = 1;
            }
#endif
            break;
        case BTIF_A2DP_EVENT_STREAM_STARTED:
#if defined(_AUTO_TEST_)
            AUTO_TEST_SEND("Music on ok.");
#endif
            if (btif_a2dp_is_stream_device_has_delay_reporting(Stream))
            {
                btif_a2dp_set_sink_delay(Stream, 150);
            }

            a2dp_timestamp_parser_init();
            app_bt_device.a2dp_streamming[stream_id_flag.id] = 1;

            app_bt_stop_sniff(stream_id_flag.id);
            app_bt_stay_active(stream_id_flag.id);

            avrcp_get_current_media_status(stream_id_flag.id);

#ifdef __BT_ONE_BRING_TWO__
            if (app_bt_is_device_connected(anotherDevice))
            {
                app_bt_stop_sniff(anotherDevice);
                app_bt_stay_active(anotherDevice);
            }

            TRACE("::A2DP_EVENT_STREAM_STARTED %d  stream_id:%d %d %d %d\n",   codec->codecType, stream_id_flag.id, app_bt_device.curr_a2dp_stream_id,
                            app_bt_device.a2dp_streamming[0], app_bt_device.a2dp_streamming[1]);


            TRACE("a2dp_connected_stream[id] = %d/id_other = %d:id_stream_state = %d/id_other = %d",app_bt_device.a2dp_connected_stream[stream_id_flag.id] ,
            app_bt_device.a2dp_connected_stream[anotherDevice],
                         btif_a2dp_get_stream_state (app_bt_device.a2dp_connected_stream[stream_id_flag.id]),
                         btif_a2dp_get_stream_state(app_bt_device.a2dp_connected_stream[anotherDevice]));

            TRACE("playback = %d",app_bt_device.a2dp_play_pause_flag);
            if(app_bt_device.a2dp_connected_stream[anotherDevice] &&
                (btif_a2dp_get_stream_state (app_bt_device.a2dp_connected_stream[anotherDevice]) != BTIF_AVDTP_STRM_STATE_STREAMING)){
                a2dp_set_cur_stream(stream_id_flag.id);
                app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC,stream_id_flag.id,MAX_RECORD_NUM);
                app_a2dp_unhold_mute();
            }else if(!app_bt_device.a2dp_connected_stream[anotherDevice]){
                a2dp_set_cur_stream(stream_id_flag.id);
                app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC,stream_id_flag.id,MAX_RECORD_NUM);
                app_a2dp_unhold_mute();
            }
#else
            TRACE("::A2DP_EVENT_STREAM_STARTED %d  stream_id:%d %d %d\n",codec->codecType, stream_id_flag.id, app_bt_device.curr_a2dp_stream_id,
                            app_bt_device.a2dp_streamming[0]);
            a2dp_set_cur_stream(BT_DEVICE_ID_1);
#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
            if(app_bt_device.src_or_snk==BT_DEVICE_SNK)
            {
              app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC,stream_id_flag.id,MAX_RECORD_NUM);
            }
            else
            {
                TRACE("::APP_A2DP_SOURCE START \n");
#if defined(APP_LINEIN_A2DP_SOURCE)
                app_audio_sendrequest(APP_A2DP_SOURCE_LINEIN_AUDIO, (uint8_t)APP_BT_SETTING_OPEN,0);
#else
                app_audio_sendrequest(APP_A2DP_SOURCE_I2S_AUDIO, (uint8_t)APP_BT_SETTING_OPEN,0);
#endif
                app_bt_device.input_onoff=1;
            }
#else
            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC,stream_id_flag.id,MAX_RECORD_NUM);
#endif

#endif

#ifdef __BT_ONE_BRING_TWO__
            if (btapp_hfp_is_dev_sco_connected(anotherDevice))
            {
                a2dp_dual_slave_setup_during_sco(anotherDevice);
            }
            else
            {
                a2dp_dual_slave_handling_refresh();
            }
#endif

#ifdef __IAG_BLE_INCLUDE__
            app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_A2DP_ON, true);
#endif
            break;
        case BTIF_A2DP_EVENT_STREAM_START_IND:
#ifdef __BT_ONE_BRING_TWO__
            TRACE("::A2DP_EVENT_STREAM_START_IND %d  stream_id:%d %d %d %d\n", codec->codecType, stream_id_flag.id, app_bt_device.curr_a2dp_stream_id,
                            app_bt_device.a2dp_streamming[0], app_bt_device.a2dp_streamming[1]);
#else
            TRACE("::A2DP_EVENT_STREAM_START_IND %d  stream_id:%d %d %d\n",codec->codecType, stream_id_flag.id, app_bt_device.curr_a2dp_stream_id,
                            app_bt_device.a2dp_streamming[0]);
#endif
            {
#ifdef BT_USB_AUDIO_DUAL_MODE
                if(!btusb_is_bt_mode())
                {
                    btif_a2dp_start_stream_rsp(Stream, BTIF_A2DP_ERR_INSUFFICIENT_RESOURCE);
                }
                else
#endif
                {
                    btif_a2dp_start_stream_rsp(Stream, BTIF_A2DP_ERR_NO_ERROR);
                    app_bt_device.a2dp_play_pause_flag = 1;
                }
            }
            break;
        case BTIF_A2DP_EVENT_STREAM_IDLE:
			TRACE("BTIF_A2DP_EVENT_STREAM_IDLE");
        case BTIF_A2DP_EVENT_STREAM_SUSPENDED:
#ifdef __BT_ONE_BRING_TWO__
            TRACE("::A2DP_EVENT_STREAM_SUSPENDED %d  stream_id:%d %d %d %d\n",codec->codecType, stream_id_flag.id, app_bt_device.curr_a2dp_stream_id,
                            app_bt_device.a2dp_streamming[0], app_bt_device.a2dp_streamming[1]);
#else
            TRACE("::A2DP_EVENT_STREAM_SUSPENDED %d  stream_id:%d %d %d\n",codec->codecType, stream_id_flag.id, app_bt_device.curr_a2dp_stream_id,
                            app_bt_device.a2dp_streamming[0]);
#endif
            a2dp_timestamp_parser_init();
            app_bt_device.a2dp_streamming[stream_id_flag.id] = 0;
#if defined(_AUTO_TEST_)
            AUTO_TEST_SEND("Music suspend ok.");
#endif
            app_bt_allow_sniff(stream_id_flag.id);

#ifdef __BT_ONE_BRING_TWO__
            if (app_bt_is_device_connected(anotherDevice) &&
                (!app_bt_device.a2dp_streamming[anotherDevice]) &&
                (BTIF_HF_CALL_ACTIVE != app_bt_device.hfchan_call[anotherDevice]))
            {
                app_bt_allow_sniff(anotherDevice);
            }

            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,stream_id_flag.id,0);
            if((app_bt_device.a2dp_connected_stream[anotherDevice]) &&
                (btif_a2dp_get_stream_state(app_bt_device.a2dp_connected_stream[anotherDevice]) == BTIF_AVDTP_STRM_STATE_STREAMING)){
                if(bt_media_is_media_active_by_device(BT_STREAM_SBC,anotherDevice) == 1){
                a2dp_set_cur_stream(anotherDevice);
                app_bt_device.a2dp_play_pause_flag = 1;
                }else{
                    a2dp_set_cur_stream(anotherDevice);
                    app_bt_device.a2dp_play_pause_flag = 1;
                app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC,anotherDevice,0);
                }
            }else{
                app_bt_device.a2dp_play_pause_flag = 0;
                app_a2dp_hold_mute();
            }
#else

#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
            if(app_bt_device.src_or_snk==BT_DEVICE_SNK)
            {
              app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,BT_DEVICE_ID_1,MAX_RECORD_NUM);
            }
            else
            {
                TRACE("::APP_A2DP_SOURCE SUSPEND \n");
#if defined(APP_LINEIN_A2DP_SOURCE)
                app_audio_sendrequest(APP_A2DP_SOURCE_LINEIN_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE,0);
#else
                app_audio_sendrequest(APP_A2DP_SOURCE_I2S_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE,0);
#endif
                app_bt_device.input_onoff=0;
            }
#else
            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,BT_DEVICE_ID_1,MAX_RECORD_NUM);
#endif
            app_bt_device.a2dp_play_pause_flag = 0;
#endif

#ifdef __BT_ONE_BRING_TWO__
            if (btapp_hfp_is_dev_sco_connected(anotherDevice))
            {
                a2dp_dual_slave_setup_during_sco(anotherDevice);
            }
            else
            {
                a2dp_dual_slave_handling_refresh();
            }
#endif

#ifdef __IAG_BLE_INCLUDE__
            app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_A2DP_ON, false);
#endif

            break;
        case BTIF_A2DP_EVENT_STREAM_DATA_IND:
#ifdef __AI_VOICE__
                if(app_ai_is_to_mute_a2dp_during_ai_starting_speech()) {
                    //TRACE("app_ai_is_to_mute_a2dp_during_ai_starting_speech");
                    break;
                }
#endif
                if (btif_me_get_current_mode(btif_a2dp_get_remote_device(Stream)) == BTIF_BLM_SNIFF_MODE){
                    break;
                }
#ifdef __BT_ONE_BRING_TWO__
                ////play music of curr_a2dp_stream_id
                if(app_bt_device.curr_a2dp_stream_id  == stream_id_flag.id &&
                   app_bt_device.hf_audio_state[stream_id_flag.id]==BTIF_HF_AUDIO_DISCON &&
                   app_bt_device.hf_audio_state[anotherDevice]==BTIF_HF_AUDIO_DISCON &&
                   app_bt_device.hfchan_callSetup[anotherDevice] == BTIF_HF_CALL_SETUP_NONE){
#ifdef __A2DP_AVDTP_CP__ //zadd bug fixed sony Z5 no sound
                        header_len = btif_avdtp_parse_mediaHeader(&header, (btif_a2dp_callback_parms_t *)Info,app_bt_device.avdtp_cp[stream_id_flag.id]);
#else
                    header_len = btif_avdtp_parse_mediaHeader(&header,(btif_a2dp_callback_parms_t *) Info,0);
#endif
                    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC) && (btif_a2dp_get_stream_state(Stream) == BTIF_AVDTP_STRM_STATE_STREAMING)){
#ifdef __A2DP_TIMESTAMP_PARSER__
                        if (a2dp_timestamp_parser_run(header.timestamp,(*(((unsigned char *)Info->p.data) + header_len)))){
                            TRACE("::A2DP_EVENT_STREAM_DATA_IND skip frame\n");
                        }else
#endif
                        {
#if (A2DP_DECODER_VER >= 2)                        
                            a2dp_audio_store_packet(&header, ((unsigned char *)Info->p.data) + header_len , Info->len - header_len);
#else
                            a2dp_audio_sbc_set_frame_info(Info->len - header_len - 1, (*(((unsigned char *)Info->p.data) + header_len)));
#if defined(A2DP_LHDC_ON)
                            if (app_bt_device.codec_type[stream_id_flag.id] == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                                store_sbc_buffer(((unsigned char *)Info->p.data) + header_len , Info->len - header_len);
                            }
                            else
#endif
#if defined(A2DP_AAC_ON)
                            if (app_bt_device.codec_type[stream_id_flag.id] == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
#ifdef BT_USB_AUDIO_DUAL_MODE
                                if(btusb_is_bt_mode())
#endif
                                {
                                    store_sbc_buffer(((unsigned char *)Info->p.data) + header_len , Info->len - header_len);
                                }
                            }
                            else
#endif
#if defined(A2DP_SCALABLE_ON)
                            if (app_bt_device.codec_type[stream_id_flag.id] == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                                store_sbc_buffer(((unsigned char *)Info->p.data) + header_len , Info->len - header_len);
                            }else
#endif
                            {
                                store_sbc_buffer(((unsigned char *)Info->p.data) + header_len + 1 , Info->len - header_len - 1);
                            }
#endif
                        }
                    }
                }
#else
#ifdef __A2DP_AVDTP_CP__ //zadd bug fixed sony Z5 no sound
                        header_len = btif_avdtp_parse_mediaHeader(&header, (btif_a2dp_callback_parms_t *)Info,app_bt_device.avdtp_cp[stream_id_flag.id]);
#else
                header_len = btif_avdtp_parse_mediaHeader(&header, (btif_a2dp_callback_parms_t *)Info,0);
#endif
                if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC) && (btif_a2dp_get_stream_state(Stream) == BTIF_AVDTP_STRM_STATE_STREAMING)){
#ifdef __A2DP_TIMESTAMP_PARSER__
                    if (a2dp_timestamp_parser_run(header.timestamp,(*(((unsigned char *)Info->p.data) + header_len)))){
                        TRACE("::A2DP_EVENT_STREAM_DATA_IND skip frame\n");
                    }else
#endif
                    {
#if (A2DP_DECODER_VER >= 2)                                         
                        a2dp_audio_store_packet(&header, ((unsigned char *)Info->p.data) + header_len , Info->len - header_len);
#else
                        a2dp_audio_sbc_set_frame_info(Info->len - header_len - 1, (*(((unsigned char *)Info->p.data) + header_len)));

#if defined(A2DP_LHDC_ON)
                        if (app_bt_device.codec_type[stream_id_flag.id] == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                            store_sbc_buffer(((unsigned char *)Info->p.data) + header_len , Info->len - header_len);
                        }else
#endif
#if defined(A2DP_AAC_ON)
                    //TRACE("%s:%d pt 0x%x, app_bt_device.codec_type[stream_id_flag.id] %d\n",
                    //      __func__, __LINE__, header.payloadType, app_bt_device.codec_type[stream_id_flag.id]);
                        if (app_bt_device.codec_type[stream_id_flag.id] == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
                            store_sbc_buffer(((unsigned char *)Info->p.data) + header_len , Info->len - header_len);
                        }
                        else
#endif
#if defined(A2DP_SCALABLE_ON)
                        if (app_bt_device.codec_type[stream_id_flag.id] == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                            store_sbc_buffer(((unsigned char *)Info->p.data) + header_len , Info->len - header_len);
                        }else
#endif
#if defined(A2DP_LDAC_ON)
						if (app_bt_device.codec_type[stream_id_flag.id] == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
							//if((((unsigned char *)Info->p.data) + header_len))
							store_sbc_buffer(((unsigned char *)Info->p.data) + header_len + 1, Info->len - header_len - 1);
						}
						else
#endif

                        store_sbc_buffer(((unsigned char *)Info->p.data) + header_len + 1 , Info->len - header_len - 1);
#endif
                    }
                }
#endif
            break;
        case BTIF_A2DP_EVENT_STREAM_CLOSED:
            TRACE("::A2DP_EVENT_STREAM_CLOSED stream_id:%d, reason = %x\n", stream_id_flag.id,Info->discReason);
#ifdef __A2DP_AVDTP_CP__
                        app_bt_device.avdtp_cp[stream_id_flag.id]=0;
#endif
            a2dp_timestamp_parser_init();
            app_bt_device.a2dp_streamming[stream_id_flag.id] = 0;
        #ifdef __BT_ONE_BRING_TWO__
            if (app_bt_is_to_resume_music_player(stream_id_flag.id))
            {
                app_bt_reset_music_player_resume_state();
            }
        #endif
            if(distinguish_found_id != BT_DEVICE_NUM){
#ifdef __BT_ONE_BRING_TWO__
                TRACE("found_id=%d state[0]/[1] = %d %d %d",distinguish_found_id,
                    app_bt_device.a2dp_state[BT_DEVICE_ID_1],app_bt_device.a2dp_state[BT_DEVICE_ID_2]);
    //            app_bt_device.curr_a2dp_stream_id = anotherDevice;
            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,stream_id_flag.id,0);
            if((app_bt_device.a2dp_connected_stream[anotherDevice]) &&
                (btif_a2dp_get_stream_state(app_bt_device.a2dp_connected_stream[anotherDevice]) == BTIF_AVDTP_STRM_STATE_STREAMING))
            {
                    if(bt_media_is_media_active_by_device(BT_STREAM_SBC,anotherDevice) == 1)
                    {
                        a2dp_set_cur_stream(anotherDevice);
                        app_bt_device.a2dp_play_pause_flag = 1;
                    }else{
                        a2dp_set_cur_stream(anotherDevice);
                        app_bt_device.a2dp_play_pause_flag = 1;
                app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC,anotherDevice,0);
                    }
            }
            else
            {
                app_bt_device.a2dp_play_pause_flag = 0;
                    app_a2dp_hold_mute();
            }
#else



#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
        if(app_bt_device.src_or_snk==BT_DEVICE_SRC)
        {
            if(app_bt_device.input_onoff!=0)
            {
#if defined(APP_LINEIN_A2DP_SOURCE)
                app_audio_sendrequest(APP_A2DP_SOURCE_LINEIN_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE,0);
#else
                app_audio_sendrequest(APP_A2DP_SOURCE_I2S_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE,0);
#endif
                app_bt_device.input_onoff=0;
            }
            app_bt_device.a2dp_streamming[BT_DEVICE_ID_1] = 0;
            a2dp_source_notify_send();
        }
        else
        {
            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,stream_id_flag.id,0);
            app_bt_profile_connect_manager_a2dp(stream_id_flag.id, Stream, (a2dp_callback_parms_t *)Info);
        }
#else
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,stream_id_flag.id,0);
            app_bt_device.a2dp_play_pause_flag = 0;
#endif

#endif

                app_bt_device.a2dp_state[stream_id_flag.id] = 0;
                app_bt_device.a2dp_connected_stream[stream_id_flag.id] = NULL;
#ifdef __BT_ONE_BRING_TWO__
        ///a2dp disconnect so check the other stream is playing or not
        if((app_bt_device.a2dp_connected_stream[anotherDevice]) &&
            (btif_a2dp_get_stream_state(app_bt_device.a2dp_connected_stream[anotherDevice])  != BTIF_AVDTP_STRM_STATE_STREAMING))
        {
            app_bt_device.a2dp_play_pause_flag = 0;
                app_a2dp_hold_mute();
        }
#endif
        }

#ifdef __BT_ONE_BRING_TWO__
        a2dp_dual_slave_handling_refresh();
#endif
        app_bt_profile_connect_manager_a2dp(stream_id_flag.id, Stream, info);
        break;
#if defined(__BTIF_BT_RECONNECT__)
        case BTIF_A2DP_EVENT_CODEC_INFO:
            TRACE("::A2DP_EVENT_CODEC_INFO %d\n", Info->event);
            setconfig_codec.codecType = Info->p.codec->codecType;
            setconfig_codec.discoverable = Info->p.codec->discoverable;
            setconfig_codec.elemLen = Info->p.codec->elemLen;
            setconfig_codec.elements = tmp_element;
            memset(tmp_element, 0, sizeof(tmp_element));
            DUMP8("%02x ", (setconfig_codec.elements), 8);
            if(Info->p.codec->codecType == BTIF_AVDTP_CODEC_TYPE_SBC){
                setconfig_codec.elements[0] = (Info->p.codec->elements[0]) & (a2dp_codec_elements[0]);
                setconfig_codec.elements[1] = (Info->p.codec->elements[1]) & (a2dp_codec_elements[1]);

                if(Info->p.codec->elements[2] <= a2dp_codec_elements[2])
                    setconfig_codec.elements[2] = a2dp_codec_elements[2];////[2]:MIN_BITPOOL
                else
                    setconfig_codec.elements[2] = Info->p.codec->elements[2];

                if(Info->p.codec->elements[3] >= a2dp_codec_elements[3])
                    setconfig_codec.elements[3] = a2dp_codec_elements[3];////[3]:MAX_BITPOOL
                else
                    setconfig_codec.elements[3] = Info->p.codec->elements[3];

                ///////null set situation:
                if(setconfig_codec.elements[3] < a2dp_codec_elements[2]){
                    setconfig_codec.elements[2] = a2dp_codec_elements[2];
                    setconfig_codec.elements[3] = a2dp_codec_elements[3];
                }
                else if(setconfig_codec.elements[2] > a2dp_codec_elements[3]){
                    setconfig_codec.elements[2] = a2dp_codec_elements[3];
                    setconfig_codec.elements[3] = a2dp_codec_elements[3];
                }
                TRACE("!!!setconfig_codec.elements[2]:%d,setconfig_codec.elements[3]:%d\n",setconfig_codec.elements[2],setconfig_codec.elements[3]);

                setconfig_codec.elements[0] = get_valid_bit(setconfig_codec.elements[0],A2D_SBC_IE_SAMP_FREQ_MSK);
                setconfig_codec.elements[0] = get_valid_bit(setconfig_codec.elements[0],A2D_SBC_IE_CH_MD_MSK);
                setconfig_codec.elements[1] = get_valid_bit(setconfig_codec.elements[1],A2D_SBC_IE_BLOCKS_MSK);
                setconfig_codec.elements[1] = get_valid_bit(setconfig_codec.elements[1],A2D_SBC_IE_SUBBAND_MSK);
                setconfig_codec.elements[1] = get_valid_bit(setconfig_codec.elements[1],A2D_SBC_IE_ALLOC_MD_MSK);
            }
#if defined(A2DP_LHDC_ON)
            else if(Info->p.codec->codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
                //Vendor ID setting
                setconfig_codec.elements[0] = a2dp_codec_lhdc_elements[0];
                setconfig_codec.elements[1] = a2dp_codec_lhdc_elements[1];
                setconfig_codec.elements[2] = a2dp_codec_lhdc_elements[2];
                setconfig_codec.elements[3] = a2dp_codec_lhdc_elements[3];
                //Codec ID setting
                setconfig_codec.elements[4] = a2dp_codec_lhdc_elements[4];
                setconfig_codec.elements[5] = a2dp_codec_lhdc_elements[5];

                //Audio format setting
                //(A2DP_LHDC_SR_96000|A2DP_LHDC_SR_48000 |A2DP_LHDC_SR_44100) | (A2DP_LHDC_FMT_16),
                if (Info->p.codec->elements[6] & BTIF_A2DP_LHDC_SR_96000) {
                    setconfig_codec.elements[6] |= BTIF_A2DP_LHDC_SR_96000;
                }else if (Info->p.codec->elements[6] & BTIF_A2DP_LHDC_SR_48000) {
                    setconfig_codec.elements[6] |= BTIF_A2DP_LHDC_SR_48000;
                }else if (Info->p.codec->elements[6] & BTIF_A2DP_LHDC_SR_44100) {
                    setconfig_codec.elements[6] |= BTIF_A2DP_LHDC_SR_44100;
                }

                if (Info->p.codec->elements[6] & BTIF_A2DP_LHDC_FMT_16) {
                    setconfig_codec.elements[6] |= BTIF_A2DP_LHDC_FMT_16;
                }else if (Info->p.codec->elements[6] & BTIF_A2DP_LHDC_FMT_24) {
                    setconfig_codec.elements[6] |= BTIF_A2DP_LHDC_FMT_24;
                }


                TRACE("setconfig_codec.elemLen = %d", setconfig_codec.elemLen);
                TRACE("setconfig_codec.elements[6] = 0x%02x", setconfig_codec.elements[6]);
            }
#endif
#if defined(A2DP_AAC_ON)
            else if(Info->p.codec->codecType == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC){
                setconfig_codec.elements[0] = a2dp_codec_aac_elements[0];
                if(Info->p.codec->elements[1] & BTIF_A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100)
                    setconfig_codec.elements[1] |= BTIF_A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100;
                else if(Info->p.codec->elements[2] & BTIF_A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000)
                    setconfig_codec.elements[2] |= BTIF_A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000;

                if(Info->p.codec->elements[2] & BTIF_A2DP_AAC_OCTET2_CHANNELS_2)
                    setconfig_codec.elements[2] |= BTIF_A2DP_AAC_OCTET2_CHANNELS_2;
                else if(Info->p.codec->elements[2] & BTIF_A2DP_AAC_OCTET2_CHANNELS_1)
                    setconfig_codec.elements[2] |= BTIF_A2DP_AAC_OCTET2_CHANNELS_1;
            }
#endif
#if defined(A2DP_SCALABLE_ON)
            else if(Info->p.codec->codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
                setconfig_codec.elements = (U8*)&a2dp_codec_scalable_elements;
            }
#endif

#if defined(A2DP_LDAC_ON)
        else if(Info->p.codec->codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
//
//                0x2d, 0x01, 0x00, 0x00, //Vendor ID
//                0xaa, 0x00,     //Codec ID
               if(Info->p.codec->elements[0]==0x2d&&
				   Info->p.codec->elements[1]==0x01&&
				   Info->p.codec->elements[2]==0x00&&
				   Info->p.codec->elements[3]==0x00&&
                   Info->p.codec->elements[4]==0xaa&&
                   Info->p.codec->elements[5]==0x00){
                   memcpy(&setconfig_codec.elements[0], &a2dp_codec_ldac_elements[0], 6);
                    
                    DUMP8("%02x ", (setconfig_codec.elements), 8);
                    DUMP8("%02x ", &(Info->p.codec->elements[0]), 8);
							//Audio format setting
                //3c 03
				//34 07								  
				if (Info->p.codec->elements[6] & A2DP_LDAC_SR_96000) {
					setconfig_codec.elements[6] |= A2DP_LDAC_SR_96000;
				}else if (Info->p.codec->elements[6] & A2DP_LDAC_SR_48000) {
					setconfig_codec.elements[6] |= A2DP_LDAC_SR_48000;
				}else if (Info->p.codec->elements[6] & A2DP_LDAC_SR_44100) {
					setconfig_codec.elements[6] |= A2DP_LDAC_SR_44100;
                }
//				}else if (Info->p.codec->elements[6] & A2DP_LDAC_SR_88200) {
//					setconfig_codec.elements[6] |= A2DP_LDAC_SR_88200;
//				}
									
				if (Info->p.codec->elements[7] & A2DP_LDAC_CM_MONO) {
					setconfig_codec.elements[7] |= A2DP_LDAC_CM_MONO;
				}else if (Info->p.codec->elements[7] & A2DP_LDAC_CM_DUAL) {
					setconfig_codec.elements[7] |= A2DP_LDAC_CM_DUAL;
				}else if (Info->p.codec->elements[7] & A2DP_LDAC_CM_STEREO) {
					setconfig_codec.elements[7] |= A2DP_LDAC_CM_STEREO;
				}
									
				TRACE("setconfig_codec.elemLen = %d", setconfig_codec.elemLen);
				TRACE("setconfig_codec.elements[7] = 0x%02x", setconfig_codec.elements[7]);
                
                DUMP8("%02x ", (setconfig_codec.elements), 8);
				}		
		}
#endif
            break;

        case BTIF_A2DP_EVENT_GET_CONFIG_IND:
            TRACE("::A2DP_EVENT_GET_CONFIG_IND %d\n", Info->event);
#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
            //I2S or LineIn for 44.1K
            setconfig_codec.elements[0] = (setconfig_codec.elements[0]&0xef) |  A2D_SBC_IE_SAMP_FREQ_44;
#endif
#ifdef __A2DP_AVDTP_CP__
            if(Info->p.capability->type & BTIF_AVDTP_SRV_CAT_CONTENT_PROTECTION||Info->p.cp->cpType!=0){
                TRACE("support CONTENT_PROTECTION\n");
                A2DP_SetStreamConfig(Stream, &setconfig_codec, &setconfig_cp[stream_id_flag.id]);
            }else{
                TRACE("no CONTENT_PROTECTION\n");
                A2DP_SetStreamConfig(Stream, &setconfig_codec, NULL);
            }
#else
            btif_a2dp_set_stream_config(Stream, &setconfig_codec, NULL);
#endif
            break;
        case BTIF_A2DP_EVENT_STREAM_RECONFIG_IND:
            TRACE("::A2DP_EVENT_STREAM_RECONFIG_IND %d\n", Info->event);
#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
            SOURCE_DBLOG("::BTIF_A2DP_EVENT_STREAM_RECONFIG_IND %d\n", Info->event);
#else
#if defined(A2DP_SCALABLE_ON)
            a2dp_scalable_config(&(Info->p.configReq->codec.elements[0]));
#endif
#if defined(A2DP_LHDC_ON)
            a2dp_lhdc_config(&(Info->p.configReq->codec.elements[0]));
#endif
#if defined(A2DP_LDAC_ON)
			TRACE("::##LDAC A2DP_EVENT_STREAM_RECONFIG_IND %d\n", Info->event);
			a2dp_ldac_config(&(Info->p.configReq->codec.elements[0]));
#endif
#endif
            btif_a2dp_reconfig_stream_rsp(Stream,BTIF_A2DP_ERR_NO_ERROR,0);
            break;
        case BTIF_A2DP_EVENT_STREAM_RECONFIG_CNF:
            TRACE("::A2DP_EVENT_STREAM_RECONFIG_CNF %d,sample rate=%x\n", 
                                    Info->event,btif_a2dp_get_stream_codecCfg( Stream)->elements[0], Info->p.codec->elements[0]);
            TRACE("codecType 0x%d\n", Info->p.configReq->codec.codecType);
#if defined(A2DP_LHDC_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                TRACE("##codecType: LHDC Codec, Element length = %d, AVDTP_MAX_CODEC_ELEM_SIZE = %d\n", Info->p.configReq->codec.elemLen, BTIF_AVDTP_MAX_CODEC_ELEM_SIZE);
                app_bt_device.codec_type[stream_id_flag.id] = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
                a2dp_channel_num[stream_id_flag.id] = 2;
                a2dp_lhdc_config(&(Info->p.configReq->codec.elements[0]));
            }else
#endif
#if defined(A2DP_AAC_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
#ifdef __A2DP_AVDTP_CP__
                A2DP_SecurityControlReq(Stream,(U8 *)&a2dp_avdtpCp_aac_securityData[stream_id_flag.id][0],1);
#endif
                TRACE("::A2DP_EVENT_STREAM_OPEN stream_id:%d, aac sample_rate codec.elements 0x%x\n", stream_id_flag.id,Info->p.configReq->codec.elements[1]);
                app_bt_device.codec_type[stream_id_flag.id] = BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC;
                app_bt_device.sample_bit[stream_id_flag.id] = 16;
                // convert aac sample_rate to sbc sample_rate format
                if (Info->p.configReq->codec.elements[1] & BTIF_A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100) {
                    TRACE("::A2DP_EVENT_STREAM_OPEN stream_id:%d, aac sample_rate 44100\n", stream_id_flag.id);
                    app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_44;
                }
                else if (Info->p.configReq->codec.elements[2] & BTIF_A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000) {
                    TRACE("::A2DP_EVENT_STREAM_OPEN stream_id:%d, aac sample_rate 48000\n", stream_id_flag.id);
                    app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_48;
                }
                else {
                    TRACE("::A2DP_EVENT_STREAM_OPEN stream_id:%d, aac sample_rate not 48000 or 44100, set to 44100\n", stream_id_flag.id);
                    app_bt_device.sample_rate[stream_id_flag.id] = A2D_SBC_IE_SAMP_FREQ_44;
                }

                if (Info->p.configReq->codec.elements[2] & BTIF_A2DP_AAC_OCTET2_CHANNELS_1){
                    a2dp_channel_num[stream_id_flag.id] = 1;
                }else{
                    a2dp_channel_num[stream_id_flag.id] = 2;
                }         
            }
            else
#endif
#if defined(A2DP_SCALABLE_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                TRACE("##codecType scalable\n");
                app_bt_device.codec_type[stream_id_flag.id] = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
                //0x75 0x00 0x00 0x00Vid
                //0x03 0x01   Codec id
                if(Info->p.codec->elements[0]==0x75&&
                   Info->p.codec->elements[4]==0x03&&
                   Info->p.codec->elements[5]==0x01
                 ){
                    setconfig_codec.elements = (U8*)&a2dp_codec_scalable_elements;
                 }
                else{
                    if(Info->p.codec->pstreamflags!=NULL)
                    Info->p.codec->pstreamflags[0] &= ~APP_A2DP_STRM_FLAG_QUERY_CODEC;
                    else{
                        ASSERT(false, "pstreamflags not init ..");
                    }
                }

                a2dp_channel_num[stream_id_flag.id] = 2;
            }else
#endif
            {
                TRACE("app_bt_device.sample_rate::elements[0] %d BITPOOL:%d/%d %02x/%02x\n", Info->p.codec->elements[0], 
                                                                                             Info->p.codec->elements[2],
                                                                                             Info->p.codec->elements[3],
                                                                                             Info->p.codec->elements[2],
                                                                                             Info->p.codec->elements[3]);

                app_bt_device.codec_type[stream_id_flag.id] = BTIF_AVDTP_CODEC_TYPE_SBC;
                app_bt_device.sample_bit[stream_id_flag.id] = 16;
                app_bt_device.sample_rate[stream_id_flag.id] = (Info->p.configReq->codec.elements[0] & A2D_SBC_IE_SAMP_FREQ_MSK);
                
                if(Info->p.configReq->codec.elements[0] & A2D_SBC_IE_CH_MD_MONO)
                    a2dp_channel_num[stream_id_flag.id]  = 1;
                else
                    a2dp_channel_num[stream_id_flag.id] = 2;                              
            }            
#ifdef __A2DP_AVDTP_CP__
            if(Info->p.configReq->cp.cpType==BTIF_AVDTP_CP_TYPE_SCMS_T)
            {
                app_bt_device.avdtp_cp[stream_id_flag.id] = 1;
            }
#endif
#endif
            break;
#ifdef __A2DP_AVDTP_CP__
        case BTIF_A2DP_EVENT_CP_INFO:
            TRACE("::A2DP_EVENT_CP_INFO %d\n", Info->event);
            TRACE("info = 0x%x  cpType:%x\n",Info,Info->p.cp->cpType);
            setconfig_cp[stream_id_flag.id].cpType = Info->p.cp->cpType;
            setconfig_cp[stream_id_flag.id].data = Info->p.cp->data;
            setconfig_cp[stream_id_flag.id].dataLen = Info->p.cp->dataLen;
            break;
        case BTIF_A2DP_EVENT_STREAM_SECURITY_IND:
            TRACE("::A2DP_EVENT_STREAM_SECURITY_IND %d\n", Info->event);
            DUMP8("%x ",Info->p.data,Info->len);
             A2DP_SecurityControlRsp(Stream,&Info->p.data[1],Info->len-1,Info->error);
            break;
        case BTIF_A2DP_EVENT_STREAM_SECURITY_CNF:
            app_bt_device.avdtp_cp[stream_id_flag.id]=1;
            TRACE("::A2DP_EVENT_STREAM_SECURITY_CNF %d\n", Info->event);
            break;
#endif

#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
		case BTIF_A2DP_EVENT_STREAM_SBC_PACKET_SENT:
			//TRACE("@sbc be send succ");
			if(app_bt_device.src_or_snk==BT_DEVICE_SRC)
			{
				a2dp_source_notify_send();
			}
			break;
#endif

#ifdef __A2DP_AVDTP_CP__
        case A2DP_EVENT_STREAM_SECURITY_IND:
            TRACE("::A2DP_EVENT_STREAM_SECURITY_IND %d\n", Info->event);
            DUMP8("%x ",Info->p.data,Info->len);
            A2DP_SecurityControlRsp(Stream,&Info->p.data[1],Info->len-1,Info->error);
            break;
        case A2DP_EVENT_STREAM_SECURITY_CNF:
            TRACE("::A2DP_EVENT_STREAM_SECURITY_CNF %d\n", Info->event);
            break;
#endif
    }
#if defined(IBRT)
        app_tws_ibrt_profile_callback(IBRT_A2DP_PROFILE,(void *)Stream, (void *)info);
#endif
}

#if defined(IBRT)
a2dp_stream_t * app_bt_get_mobile_a2dp_stream(uint32_t deviceId);
int app_bt_stream_ibrt_audio_mismatch_stopaudio(void);

int a2dp_ibrt_sync_get_status(ibrt_a2dp_status_t *a2dp_status)
{
    a2dp_status->volume = a2dp_volume_get(BT_DEVICE_ID_1);    
    a2dp_status->state  = btif_a2dp_get_stream_state(app_bt_get_mobile_a2dp_stream(BTIF_DEVICE_ID_1));
    return 0;
}

int a2dp_ibrt_sync_set_status(ibrt_a2dp_status_t *a2dp_status)
{
    a2dp_stream_t *Stream = app_bt_get_mobile_a2dp_stream(BTIF_DEVICE_ID_1);
    btif_avdtp_stream_state_t old_avdtp_stream_state = btif_a2dp_get_stream_state(Stream);
    struct mock_a2dp_callback_parms_t{
        btif_a2dp_event_t event;
    } info;

    TRACE("%s,stream_state:[%d]->[%d], codec_type:%d volume:%d", __func__, old_avdtp_stream_state, a2dp_status->state, a2dp_status->codec.codec_type, a2dp_status->volume);

    btif_a2dp_set_codec_info(BTIF_DEVICE_ID_1, a2dp_status->codec.codec_type, (uint8_t)a2dp_status->codec.sample_rate, a2dp_status->codec.sample_bit);
    btif_a2dp_set_stream_state(Stream, a2dp_status->state);    
    app_bt_stream_volume_ptr_update((uint8_t *) btif_me_get_remote_device_bdaddr(btif_a2dp_get_stream_conn_remDev(Stream)));
    a2dp_volume_set(BT_DEVICE_ID_1, a2dp_status->volume);

    if (a2dp_status->state != old_avdtp_stream_state){
        switch(a2dp_status->state) {
            case BTIF_AVDTP_STRM_STATE_STREAMING:
                app_bt_clear_connecting_profiles_state(BTIF_DEVICE_ID_1);                
                a2dp_timestamp_parser_init();
                info.event = BTIF_A2DP_EVENT_STREAM_STARTED;
                a2dp_callback(Stream, &info);
#if defined(IBRT_FORCE_AUDIO_RETRIGGER)
                app_ibrt_if_force_audio_retrigger()
#else
                app_bt_stream_ibrt_audio_mismatch_stopaudio();
#endif
                break;
        }
    }
    return 0;
}

int a2dp_ibrt_stream_open_mock(void)
{
    a2dp_stream_t *Stream = app_bt_get_mobile_a2dp_stream(BTIF_DEVICE_ID_1);

    app_bt_clear_connecting_profiles_state(BTIF_DEVICE_ID_1);                
    a2dp_timestamp_parser_init();
    app_bt_stream_volume_ptr_update((uint8_t *) btif_me_get_remote_device_bdaddr(btif_a2dp_get_stream_conn_remDev(Stream)));
    a2dp_set_cur_stream(stream_id_flag.id);

    return 0;
}
#endif
void a2dp_suspend_music_force(void)
{
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,BT_DEVICE_ID_1,0);
#ifdef __BT_ONE_BRING_TWO__
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,BT_DEVICE_ID_2,0);
#endif
}

 int a2dp_volume_get(enum BT_DEVICE_ID_T id)
{
    int vol = TGT_VOLUME_LEVEL_15;
#ifndef __BT_ONE_BRING_TWO__
    vol = app_bt_stream_volume_get_ptr()->a2dp_vol;
#else
    if (app_audio_manager_a2dp_is_active(id)){
        vol = app_bt_stream_volume_get_ptr()->a2dp_vol;
    }else{
        btif_remote_device_t *remDev = NULL;
        nvrec_btdevicerecord *record = NULL;
        remDev = btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[id]);
        if (remDev){
            if (!nv_record_btdevicerecord_find(btif_me_get_remote_device_bdaddr(remDev),&record)){
                vol = record->device_vol.a2dp_vol;
            }
        }else{
            vol = TGT_VOLUME_LEVEL_15;
        }
    }
#endif

    if (vol == TGT_VOLUME_LEVEL_MUTE){
        vol = 0;
    }else{
        if (vol >= TGT_VOLUME_LEVEL_15){
            vol = TGT_VOLUME_LEVEL_15;
        }else if (vol <= TGT_VOLUME_LEVEL_0){
            vol = TGT_VOLUME_LEVEL_0;
        }
        vol = 8*(vol-1);
    }

    if (vol > 0x7f)
        vol = 0x7f;

    TRACE("get vol raw:%d %d/127", app_bt_stream_volume_get_ptr()->a2dp_vol, vol);

    return (vol);
}


void a2dp_volume_local_set(int8_t vol)
{
    if(app_bt_stream_volume_get_ptr()->a2dp_vol != vol)
    {
        app_bt_stream_volume_get_ptr()->a2dp_vol = vol;
#ifndef FPGA
        nv_record_touch_cause_flush();
#endif
    }
}

static int a2dp_volume_set(enum BT_DEVICE_ID_T id, uint8_t vol)
{
    int dest_vol;

    if (!vol){
        dest_vol = TGT_VOLUME_LEVEL_MUTE;
    }else{
        if (0x7f == vol)
        {
            dest_vol = TGT_VOLUME_LEVEL_15;
        }
        else
        {
            dest_vol = (((int)vol&0x7f)<<4)/128 + 1;   
        }
        if (dest_vol > TGT_VOLUME_LEVEL_15){
            dest_vol = TGT_VOLUME_LEVEL_15;
        }else if (dest_vol < TGT_VOLUME_LEVEL_0){
            dest_vol = TGT_VOLUME_LEVEL_0;
        }
    }

#ifndef __BT_ONE_BRING_TWO__
    a2dp_volume_local_set(dest_vol);
    app_bt_stream_volumeset(dest_vol);
#else
    if (app_audio_manager_a2dp_is_active(id)){
        a2dp_volume_local_set(dest_vol);
        app_bt_stream_volumeset(dest_vol);
    }else{
        btif_remote_device_t *remDev = NULL;
        nvrec_btdevicerecord *record = NULL;
        remDev = btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[id]);
        if (remDev){
            if (!nv_record_btdevicerecord_find(btif_me_get_remote_device_bdaddr(remDev),&record)){
                record->device_vol.a2dp_vol = dest_vol;
            }
        }
    }
#endif
    TRACE("put vol raw:%d/127 %d", vol, dest_vol);

    return (vol);
}

bool a2dp_is_music_ongoing(void)
{
#ifdef __BT_ONE_BRING_TWO__
    return app_bt_device.a2dp_streamming[BT_DEVICE_ID_1] ||
        app_bt_device.a2dp_streamming[BT_DEVICE_ID_2];
#else
    return app_bt_device.a2dp_streamming[BT_DEVICE_ID_1];
#endif

}

void btapp_a2dp_report_speak_gain(void)
{
#ifdef BTIF_AVRCP_ADVANCED_CONTROLLER
    uint8_t i;
    int vol;
    btif_remote_device_t *remDev = NULL;
    btif_link_mode_t mode = BTIF_BLM_SNIFF_MODE;

    for(i=0; i<BT_DEVICE_NUM; i++){
    /*
        TRACE("btapp_a2dp_report_speak_gain transId:%d a2dp_state:%d streamming:%d report:%02x\n",
        btif_get_avrcp_adv_notify(const avrcp_callback_parms_t * parms)(app_bt_device.avrcp_notify_rsp[i])->transId,
        app_bt_device.a2dp_state[i],
        app_bt_device.a2dp_streamming[i],
        app_bt_device.volume_report[i]);
       */

        osapi_lock_stack();
        if((vol_ctrl_done_flag[i]==TX_DONE_FLAG_INIT)||(vol_ctrl_done_flag[i]==TX_DONE_FLAG_SUCCESS)){
            remDev = btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[i]);
            if (remDev){
                mode = btif_me_get_current_mode(remDev);
            }else{
                mode = BTIF_BLM_SNIFF_MODE;
            }
            if ((app_bt_device.a2dp_state[i] == 1) &&
                    (app_bt_device.a2dp_streamming[i] == 1) &&
                    (app_bt_device.volume_report[i] == BTIF_AVCTP_RESPONSE_INTERIM) &&
                    (mode == BTIF_BLM_ACTIVE_MODE)){
                app_bt_device.volume_report[i] = BTIF_AVCTP_RESPONSE_CHANGED;
                //  TRACE("btapp_a2dp_report_speak_gain transId:%d\n", app_bt_device.avrcp_notify_rsp[i]->transId);
                if (app_bt_device.avrcp_notify_rsp[i] != NULL){

                    btif_set_app_bt_device_avrcp_notify_rsp_ctype(app_bt_device.avrcp_notify_rsp[i],  BTIF_AVCTP_RESPONSE_CHANGED);
                    vol = a2dp_volume_get((enum BT_DEVICE_ID_T)i);
                    btif_avrcp_ct_get_absolute_volume_rsp(app_bt_device.avrcp_channel[i], app_bt_device.avrcp_notify_rsp[i], vol);
                    vol_ctrl_done_flag[i] = TX_DONE_FLAG_TXING;
                }
            }
        }
        osapi_unlock_stack();
    }
#endif
}





extern "C" bool avdtp_Get_aacEnable_Flag( btif_remote_device_t* remDev, btif_avdtp_stream_t *strm)
{
    TRACE("%s,version=%x", btif_me_get_remote_device_version(remDev)[1] );
#if 0//defined(A2DP_AAC_ON)
    if(((remDev->remoteVersion[1] == 0x0f) && (remDev->remoteVersion[2] == 0)) || strm->codec->codecType !=AVDTP_CODEC_TYPE_MPEG2_4_AAC){
        return TRUE;
    }else
        return FALSE;
#else
    return TRUE;
#endif


}



#ifdef __TWS__
void app_AVRCP_SendCustomCmdToMobile(uint8_t* ptrData, uint32_t len)
{
    if (is_slave_tws_mode())
    {
        return;
    }

    btif_avrcp_send_custom_cmd_generic(app_bt_device.avrcp_channel[BT_DEVICE_ID_1], ptrData, len);
}


void app_AVRCP_SendCustomCmdToTws(uint8_t* ptrData, uint32_t len)
{
    if (is_slave_tws_mode())
    {
        btif_avrcp_send_custom_cmd_generic(app_bt_device.avrcp_channel[BT_DEVICE_ID_1], ptrData, len);
    }
    else if (is_master_tws_mode())
    {
        btif_avrcp_channel_t* chnl =  btif_get_avrcp_channel(tws_get_avrcp_channel_hdl());
        btif_avrcp_send_custom_cmd_generic(chnl, ptrData, len);
    }
}

#endif

static void app_AVRCP_sendCustomCmdRsp(uint8_t device_id, btif_avrcp_channel_t *chnl, uint8_t isAccept, uint8_t transId)
{
    if (app_bt_device.avrcp_control_rsp[device_id] == NULL)
    {
        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_control_rsp[device_id]);
    }


    btif_avrcp_set_control_rsp_cmd(app_bt_device.avrcp_control_rsp[device_id],transId, BTIF_AVCTP_RESPONSE_ACCEPTED);

    btif_avrcp_ct_accept_custom_cmd_rsp(chnl, app_bt_device.avrcp_control_rsp[device_id], isAccept);
}

static void app_AVRCP_CustomCmd_Received(uint8_t* ptrData, uint32_t len)
{
    TRACE("AVRCP Custom Command Received %d bytes data:", len);
    DUMP8("0x%02x ", ptrData, len);
}



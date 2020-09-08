/*
 *  Copyright (c) 2020 The wl project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifdef WL_VAD

/*
 *  Copyright (c) 
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */



#include "webrtc_vad.h"
#include "hal_trace.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>  // size_t
#include <stdlib.h>
#include <string.h>

static pVadInst phandle = NULL;


void wl_vad_init(void)
{
	int flag;

	flag = wl_WebRtcVad_Create(&phandle);

	TRACE("vad flag:%d ",flag);

	flag = wl_WebRtcVad_Init(phandle);
	TRACE("vad aa flag:%d ",flag);

	flag = wl_WebRtcVad_set_mode(phandle, 2);

	TRACE("vad init success :flag:%d, phandle:0x%x",flag, phandle);
}


void wl_vad_process_frame(short *buffer, int len)
{
    int vad_state = 0;

    vad_state = wl_WebRtcVad_Process(phandle, 16000, buffer, len);

    if(vad_state)
    {
	    TRACE("man voice detected:%d  ",vad_state);
    }
}
void vad_deinit()
{
    ;
}


#endif
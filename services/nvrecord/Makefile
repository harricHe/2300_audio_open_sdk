cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += -Iutils/hexdump    \
        -Iapps/key  \
        -Iservices/bt_app   \
        -Iservices/bt_if/inc  \
        -Iutils/crc32   \
        -Iplatform/drivers/ana \
        -Iservices/nv_section/customparam_section \
        -Iservices/ble_stack/common/api \
        -Iservices/ble_stack/ble_ip \
        -Iutils/heap \
        -Iservices/norflash_api

CFLAGS_nvrecord.o += -DAUDIO_OUTPUT_VOLUME_DEFAULT=$(AUDIO_OUTPUT_VOLUME_DEFAULT)

ifeq ($(FLASH_SUSPEND),1)
CFLAGS_nvrecord.o += -DFLASH_SUSPEND
endif



add_if_exists = $(foreach d,$(1),$(if $(wildcard $(srctree)/$(d)),$(d) ,))

# -------------------------------------------
# Root Option Dependencies
# -------------------------------------------

ifeq ($(BT_ANC),1)
export ANC_APP ?= 1
endif

ifeq ($(BISTO),1)
BLE := 1
ifeq ($(CHIP),best1400)
export DUMP_CRASH_LOG ?= 0
else
export DUMP_CRASH_LOG ?= 1
endif
VOICE_DATAPATH_ENABLED ?= 1
export GFPS_ENABLE ?= 1
MIX_MIC_DURING_MUSIC_ENABLED ?= 0

KBUILD_CPPFLAGS += -DCFG_SW_KEY_LPRESS_THRESH_MS=1000

export VOICE_DATAPATH_TYPE ?= gsound
#export TRACE_DUMP2FLASH ?= 1

export FLASH_SUSPEND ?= 1
ifeq ($(FLASH_SUSPEND), 1)
KBUILD_CPPFLAGS += -DFLASH_SUSPEND
endif

KBUILD_CFLAGS += -DGSOUND_ENABLED
KBUILD_CPPFLAGS += -DDEBUG_BLE_DATAPATH=0
KBUILD_CPPFLAGS += -DGSOUND_OTA_ENABLED
export OTA_BASIC := 1
KBUILD_CFLAGS += -DCRC32_OF_IMAGE

#export OPUS_CODEC ?= 1
#ENCODING_ALGORITHM_OPUS                2
#ENCODING_ALGORITHM_SBC                 3
#KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=3

# As MIX_MIC_DURING_MUSIC uses the isolated audio stream, if define MIX_MIC_DURING_MUSIC, the isolated audio stream
# must be enabled
KBUILD_CPPFLAGS += -DISOLATED_AUDIO_STREAM_ENABLED=1

ASSERT_SHOW_FILE_FUNC ?= 1
#KBUILD_CPPFLAGS += -DSAVING_AUDIO_DATA_TO_SD_ENABLED=1

else 

export DUMP_CRASH_LOG ?= 0

endif # BISTO

export AI_VOICE ?= 0
ifeq ($(AI_VOICE),1)
KBUILD_CPPFLAGS += -D__AI_VOICE__
KBUILD_CPPFLAGS += -DNO_ENCODING=0
KBUILD_CPPFLAGS += -DENCODING_ALGORITHM_ADPCM=1
KBUILD_CPPFLAGS += -DENCODING_ALGORITHM_OPUS=2
KBUILD_CPPFLAGS += -DENCODING_ALGORITHM_SBC=3
ifeq ($(USE_KNOWLES),1)
KBUILD_CPPFLAGS += -D__KNOWLES \
                   -DDIG_MIC_WORKAROUND

KBUILD_CPPFLAGS += -DKNOWLES_UART_DATA
KBUILD_CPPFLAGS += -DIDLE_ALEXA_KWD
export THRIDPARTY_LIB ?= knowles_uart
endif
endif

export AMA_VOICE ?= 0
ifeq ($(AMA_VOICE),1)
BLE := 1

KBUILD_CPPFLAGS += -DBTIF_DIP_DEVICE
KBUILD_CPPFLAGS += -DKEYWORD_WAKEUP_ENABLED=0
KBUILD_CPPFLAGS += -DPUSH_AND_HOLD_ENABLED=0
KBUILD_CPPFLAGS += -DAI_32KBPS_VOICE=0
KBUILD_CPPFLAGS += -D__AMA_VOICE__
#KBUILD_CPPFLAGS += -DNO_LOCAL_START_TONE
ifeq ($(CHIP),best1400)
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=3
export OPUS_CODEC ?= 0
else
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=2
export OPUS_CODEC ?= 1
endif
endif

export DMA_VOICE ?= 0
ifeq ($(DMA_VOICE),1)
KBUILD_CPPFLAGS += -D__DMA_VOICE__
KBUILD_CPPFLAGS += -D__BES__
KBUILD_CPPFLAGS += -DBAIDU_DATA_SN_LEN=16
KBUILD_CPPFLAGS += -DFLOW_CONTROL_ON_UPLEVEL=1
KBUILD_CPPFLAGS += -DASSAM_PKT_ON_UPLEVEL=1
KBUILD_CPPFLAGS += -DBAIDU_DATA_RAND_LEN=8
KBUILD_CPPFLAGS += -DCLOSE_BLE_ADV_WHEN_VOICE_CALL=1
KBUILD_CPPFLAGS += -DCLOSE_BLE_ADV_WHEN_SPP_CONNECTED=1
KBUILD_CPPFLAGS += -DBAIDU_RFCOMM_DIRECT_CONN=1
KBUILD_CPPFLAGS += -DBYPASS_SLOW_ADV_MODE=1

KBUILD_CPPFLAGS += -DNVREC_BAIDU_DATA_SECTION=1
KBUILD_CPPFLAGS += -DFM_MIN_FREQ=875 -DFM_MAX_FREQ=1079
KBUILD_CPPFLAGS += -DBAIDU_DATA_DEF_FM_FREQ=893
KBUILD_CPPFLAGS += -DBAIDU_DATA_RAND_LEN=8
KBUILD_CPPFLAGS += -DBAIDU_DATA_SN_LEN=16

KBUILD_CPPFLAGS += -DKEYWORD_WAKEUP_ENABLED=0
KBUILD_CPPFLAGS += -DPUSH_AND_HOLD_ENABLED=0
KBUILD_CPPFLAGS += -DAI_32KBPS_VOICE=0
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=2
export OPUS_CODEC ?= 1
export LIBC_ROM := 0
ifeq ($(LIBC_ROM),1)
$(error LIBC_ROM should be 0 when DMA_VOICE=1)
endif
endif

export SMART_VOICE ?= 0
ifeq ($(SMART_VOICE),1)
KBUILD_CPPFLAGS += -D__SMART_VOICE__
KBUILD_CPPFLAGS += -DKEYWORD_WAKEUP_ENABLED=0
KBUILD_CPPFLAGS += -DPUSH_AND_HOLD_ENABLED=1
KBUILD_CPPFLAGS += -DAI_32KBPS_VOICE=0
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=2
export OPUS_CODEC ?= 1
#SPEECH_CODEC_CAPTURE_CHANNEL_NUM ?= 2
#KBUILD_CPPFLAGS += -DMCU_HIGH_PERFORMANCE_MODE
#KBUILD_CPPFLAGS += -DSPEECH_CAPTURE_TWO_CHANNEL
endif

export TENCENT_VOICE ?= 0
ifeq ($(TENCENT_VOICE),1)
KBUILD_CPPFLAGS += -D__TENCENT_VOICE__
KBUILD_CPPFLAGS += -DKEYWORD_WAKEUP_ENABLED=0
KBUILD_CPPFLAGS += -DPUSH_AND_HOLD_ENABLED=0
KBUILD_CPPFLAGS += -DAI_32KBPS_VOICE=0
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=2
export OPUS_CODEC ?= 1
endif

ifeq ($(AMA_VOICE), 1)
ifeq ($(BISTO),1)
IS_MULTI_AI_ENABLED ?= 1
else
IS_MULTI_AI_ENABLED ?= 0
endif
else
IS_MULTI_AI_ENABLED ?= 0
endif

export OTA_BASIC ?= 0
ifneq ($(filter 1,$(OTA_BASIC) $(BES_OTA_BASIC)),)
export OTA_CODE_OFFSET ?= 0x18000
KBUILD_CPPFLAGS += -D__APP_IMAGE_FLASH_OFFSET__=$(OTA_CODE_OFFSET)
KBUILD_CPPFLAGS += -DNEW_IMAGE_FLASH_OFFSET=0x200000
KBUILD_CPPFLAGS += -DFIRMWARE_REV
endif

ifneq ($(filter apps/ tests/speech_test/, $(core-y)),)
BT_APP ?= 1
FULL_APP_PROJECT ?= 1
endif

# -------------------------------------------
# CHIP selection
# -------------------------------------------

export CHIP

ifneq (,)
else ifeq ($(CHIP),best2300p)
KBUILD_CPPFLAGS += -DCHIP_BEST2300P
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_SDIO := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_I2C := 2
export CHIP_HAS_UART := 3
export CHIP_HAS_DMA := 2
export CHIP_HAS_I2S := 2
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_TRANSQ := 0
export CHIP_HAS_EXT_PMU := 1
export CHIP_HAS_CP := 1
export CHIP_HAS_AUDIO_CONST_ROM := 0
export CHIP_FLASH_CTRL_VER := 2
export CHIP_SPI_VER := 3
export CHIP_CACHE_VER := 2
else ifeq ($(CHIP),best1400)
KBUILD_CPPFLAGS += -DCHIP_BEST1400
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 0
export CHIP_HAS_SDMMC := 0
export CHIP_HAS_SDIO := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 0
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_I2C := 1
export CHIP_HAS_UART := 3
export CHIP_HAS_DMA := 1
export CHIP_HAS_SPDIF := 0
export CHIP_HAS_TRANSQ := 0
export CHIP_HAS_EXT_PMU := 0
export CHIP_HAS_AUDIO_CONST_ROM := 0
export CHIP_FLASH_CTRL_VER := 2
export CHIP_SPI_VER := 3
export BTDUMP_ENABLE ?= 1
else ifeq ($(CHIP),best3001)
KBUILD_CPPFLAGS += -DCHIP_BEST3001
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 0
export CHIP_HAS_SDIO := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 0
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_I2C := 1
export CHIP_HAS_UART := 2
export CHIP_HAS_DMA := 1
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_TRANSQ := 0
export CHIP_HAS_EXT_PMU := 0
export CHIP_HAS_AUDIO_CONST_ROM := 0
ifeq ($(CHIP_BEST3003),1)
KBUILD_CPPFLAGS += -DCHIP_BEST3003
export CHIP_CACHE_VER := 2
export CHIP_FLASH_CTRL_VER := 2
else
export CHIP_FLASH_CTRL_VER := 1
endif
export CHIP_SPI_VER := 3
else ifeq ($(CHIP),best2300)
KBUILD_CPPFLAGS += -DCHIP_BEST2300
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_SDIO := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_I2C := 2
export CHIP_HAS_UART := 3
export CHIP_HAS_DMA := 2
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_TRANSQ := 0
export CHIP_HAS_EXT_PMU := 1
export CHIP_HAS_AUDIO_CONST_ROM := 0
export CHIP_FLASH_CTRL_VER := 2
export CHIP_SPI_VER := 2
else ifeq ($(CHIP),best2000)
KBUILD_CPPFLAGS += -DCHIP_BEST2000
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_SDIO := 1
export CHIP_HAS_PSRAM := 1
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_I2C := 1
export CHIP_HAS_UART := 3
export CHIP_HAS_DMA := 2
export CHIP_HAS_SPDIF := 2
export CHIP_HAS_TRANSQ := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_HAS_AUDIO_CONST_ROM := 0
export CHIP_FLASH_CTRL_VER := 1
export CHIP_SPI_VER := 1
else
ifneq ($(CHIP),best1000)
$(error Invalid CHIP: $(CHIP))
endif
KBUILD_CPPFLAGS += -DCHIP_BEST1000
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 0
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_SDIO := 1
export CHIP_HAS_PSRAM := 1
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_I2C := 1
export CHIP_HAS_UART := 2
export CHIP_HAS_DMA := 2
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_TRANSQ := 0
export CHIP_HAS_EXT_PMU := 0
export CHIP_HAS_AUDIO_CONST_ROM := 1
export CHIP_FLASH_CTRL_VER := 1
export CHIP_SPI_VER := 1
endif

export FLASH_SIZE ?= 0x100000
export PSRAM_SIZE ?= 0x400000

KBUILD_CPPFLAGS += -DCHIP_HAS_UART=$(CHIP_HAS_UART)
KBUILD_CPPFLAGS += -DCHIP_HAS_I2C=$(CHIP_HAS_I2C)

ifeq ($(CHIP_HAS_USB),1)
KBUILD_CPPFLAGS += -DCHIP_HAS_USB
endif

ifneq ($(CHIP_HAS_I2S),)
KBUILD_CPPFLAGS += -DCHIP_HAS_I2S=$(CHIP_HAS_I2S)
endif

ifneq ($(CHIP_HAS_SPDIF),0)
KBUILD_CPPFLAGS += -DCHIP_HAS_SPDIF=$(CHIP_HAS_SPDIF)
endif

ifneq ($(CHIP_HAS_TRANSQ),0)
KBUILD_CPPFLAGS += -DCHIP_HAS_TRANSQ=$(CHIP_HAS_TRANSQ)
endif

ifeq ($(CHIP_HAS_CP),1)
KBUILD_CPPFLAGS += -DCHIP_HAS_CP
endif

ifeq ($(CHIP_HAS_AUDIO_CONST_ROM),1)
KBUILD_CPPFLAGS += -DCHIP_HAS_AUDIO_CONST_ROM
endif

ifeq ($(MCU_SLEEP_POWER_DOWN),1)
KBUILD_CPPFLAGS += -DMCU_SLEEP_POWER_DOWN
endif

ifeq ($(USB_AUDIO_APP),1)
ifneq ($(BTUSB_AUDIO_MODE),1)
NO_OVERLAY ?= 1
endif
endif
export NO_OVERLAY
ifeq ($(NO_OVERLAY),1)
KBUILD_CPPFLAGS +=  -DNO_OVERLAY
endif

ifneq ($(ROM_SIZE),)
KBUILD_CPPFLAGS += -DROM_SIZE=$(ROM_SIZE)
endif

ifneq ($(RAM_SIZE),)
KBUILD_CPPFLAGS += -DRAM_SIZE=$(RAM_SIZE)
endif

ifneq ($(OTA_BOOT_SIZE),)
CPPFLAGS_${LDS_FILE} += -DOTA_BOOT_SIZE=$(OTA_BOOT_SIZE)
endif
ifneq ($(OTA_CODE_OFFSET),)
CPPFLAGS_${LDS_FILE} += -DOTA_CODE_OFFSET=$(OTA_CODE_OFFSET)
endif
ifneq ($(FLASH_REGION_SIZE),)
CPPFLAGS_${LDS_FILE} += -DFLASH_REGION_SIZE=$(FLASH_REGION_SIZE)
endif

ifeq ($(AUDIO_SECTION_ENABLE),1)
KBUILD_CPPFLAGS += -DAUDIO_SECTION_ENABLE
# depend on length of (ANC + AUDIO + SPEECH) in aud_section.c
AUD_SECTION_SIZE ?= 0x8000
ifeq ($(ANC_APP),1)
$(error Can not enable AUDIO_SECTION_ENABLE and ANC_APP together)
endif
endif

ifeq ($(ANC_APP),1)
ifeq ($(CHIP),best1000)
AUD_SECTION_SIZE ?= 0x8000
else
AUD_SECTION_SIZE ?= 0x10000
endif
ifeq ($(ANC_FB_CHECK),1)
KBUILD_CPPFLAGS += -DANC_FB_CHECK
endif
else
AUD_SECTION_SIZE ?= 0
endif

ifeq ($(TWS),1)
LARGE_RAM ?= 1
endif

USERDATA_SECTION_SIZE ?= 0x1000

FACTORY_SECTION_SIZE ?= 0x1000

#CRASH_DUMP_SECTION_SIZE ?= 0

export DUMP_NORMAL_LOG ?= 0
ifeq ($(DUMP_NORMAL_LOG),1)
ifeq ($(FLASH_SIZE),0x40000) # 2M bits
LOG_DUMP_SECTION_SIZE ?= 0x4000
endif
ifeq ($(FLASH_SIZE),0x80000) # 4M bits
LOG_DUMP_SECTION_SIZE ?= 0x8000
endif
ifeq ($(FLASH_SIZE),0x100000) # 8M bits
LOG_DUMP_SECTION_SIZE ?= 0x10000
endif
ifeq ($(FLASH_SIZE),0x200000) # 16M bits
LOG_DUMP_SECTION_SIZE ?= 0x80000
endif
ifeq ($(FLASH_SIZE),0x400000) # 32M bits
LOG_DUMP_SECTION_SIZE ?= 0x200000
endif
ifeq ($(FLASH_SIZE),0x800000) # 64M bits
LOG_DUMP_SECTION_SIZE ?= 0x400000
endif
KBUILD_CPPFLAGS += -DDUMP_LOG_ENABLE
else
LOG_DUMP_SECTION_SIZE ?= 0
endif

ifeq ($(DUMP_CRASH_LOG),1)
CRASH_DUMP_SECTION_SIZE ?= 0x4000
KBUILD_CPPFLAGS += -DDUMP_CRASH_ENABLE
else
CRASH_DUMP_SECTION_SIZE ?= 0
endif

CUSTOM_PARAMETER_SECTION_SIZE ?= 0x1000

ifeq ($(OTA_BASIC),1)
OTA_UPGRADE_LOG_SIZE ?= 0x1000
else
OTA_UPGRADE_LOG_SIZE ?= 0
endif

export SLAVE_BIN_FLASH_OFFSET ?= 0

CPPFLAGS_${LDS_FILE} += \
	-DLINKER_SCRIPT \
	-DBUILD_INFO_MAGIC=0xBE57341D \
	-DFLASH_SIZE=$(FLASH_SIZE) \
	-DPSRAM_SIZE=$(PSRAM_SIZE) \
	-DSLAVE_BIN_FLASH_OFFSET=$(SLAVE_BIN_FLASH_OFFSET) \
	-DOTA_UPGRADE_LOG_SIZE=$(OTA_UPGRADE_LOG_SIZE) \
	-DLOG_DUMP_SECTION_SIZE=$(LOG_DUMP_SECTION_SIZE) \
	-DCRASH_DUMP_SECTION_SIZE=$(CRASH_DUMP_SECTION_SIZE) \
	-DCUSTOM_PARAMETER_SECTION_SIZE=$(CUSTOM_PARAMETER_SECTION_SIZE) \
	-DAUD_SECTION_SIZE=$(AUD_SECTION_SIZE) \
	-DUSERDATA_SECTION_SIZE=$(USERDATA_SECTION_SIZE) \
	-DFACTORY_SECTION_SIZE=$(FACTORY_SECTION_SIZE) \
	-Iplatform/hal

ifeq ($(FLASH_REMAP),1)
CPPFLAGS_${LDS_FILE} += -DFLASH_REMAP
endif

ifeq ($(LARGE_RAM),1)
KBUILD_CPPFLAGS += -DLARGE_RAM
endif

ifeq ($(CHIP_HAS_EXT_PMU),1)
export PMU_IRQ_UNIFIED ?= 1
endif

# -------------------------------------------
# Standard C library
# -------------------------------------------

export NOSTD
export LIBC_ROM

ifeq ($(NOSTD),1)

ifeq ($(MBED),1)
$(error Invalid configuration: MBED needs standard C library support)
endif
ifeq ($(RTOS),1)
$(error Invalid configuration: RTOS needs standard C library support)
endif

ifneq ($(NO_LIBC),1)
core-y += utils/libc/
endif

SPECS_CFLAGS :=

LIB_LDFLAGS := $(filter-out -lstdc++ -lsupc++ -lm -lc -lgcc -lnosys,$(LIB_LDFLAGS))

KBUILD_CPPFLAGS += -nostdinc -ffreestanding -Iutils/libc/inc

CFLAGS_IMAGE += -nostdlib

CPPFLAGS_${LDS_FILE} += -DNOSTD

else # NOSTD != 1

ifeq ($(LIBC_ROM),1)
core-y += utils/libc/
endif

SPECS_CFLAGS := --specs=nano.specs

LIB_LDFLAGS += -lm -lc -lgcc -lnosys

endif # NOSTD != 1

# -------------------------------------------
# RTOS library
# -------------------------------------------

export RTOS

KERNEL ?= RTX
export KERNEL

ifeq ($(RTOS),1)

core-y += rtos/

KBUILD_CPPFLAGS += -DRTOS -D__CORTEX_M4F
KBUILD_CPPFLAGS += -DKERNEL_$(KERNEL)

ifeq ($(KERNEL),RTX)
KBUILD_CPPFLAGS += \
	-Iinclude/rtos/rtx/
KBUILD_CPPFLAGS += -D__RTX_CPU_STATISTICS__=1
KBUILD_CPPFLAGS += -D_CPU_STATISTICS_PEROID_=6000
else #!rtx
ifeq ($(KERNEL),FREERTOS)
KBUILD_CPPFLAGS += \
    -Iinclude/rtos/freertos/
endif #freertos
endif #rtx

ifeq ($(TWS),1)
OS_TASKCNT ?= 12
OS_SCHEDULERSTKSIZE ?= 768
OS_IDLESTKSIZE ?= 512
else
OS_TASKCNT ?= 20
OS_SCHEDULERSTKSIZE ?= 512
OS_IDLESTKSIZE ?= 256
endif
OS_CLOCK_NOMINAL ?= 32000
OS_FIFOSZ ?= 24

export OS_TASKCNT
export OS_SCHEDULERSTKSIZE
export OS_IDLESTKSIZE
export OS_CLOCK_NOMINAL
export OS_FIFOSZ

endif

# -------------------------------------------
# MBED library
# -------------------------------------------

export MBED

ifeq ($(MBED),1)

core-y += mbed/

KBUILD_CPPFLAGS += -DMBED

KBUILD_CPPFLAGS += \
	-Imbed/api \
	-Imbed/common \

endif

# -------------------------------------------
# DEBUG functions
# -------------------------------------------

export DEBUG

KBUILD_CFLAGS	+= -O2
ifeq ($(DEBUG),1)

KBUILD_CPPFLAGS	+= -DDEBUG

else

KBUILD_CPPFLAGS	+= -DNDEBUG

REL_TRACE_ENABLE ?= 1
ifeq ($(REL_TRACE_ENABLE),1)
KBUILD_CPPFLAGS	+= -DREL_TRACE_ENABLE
endif

endif

ifeq ($(MERGE_CONST),1)
KBUILD_CPPFLAGS += -fmerge-constants -fmerge-all-constants
endif

# -------------------------------------------
# SIMU functions
# -------------------------------------------

export SIMU

ifeq ($(SIMU),1)

KBUILD_CPPFLAGS += -DSIMU

endif

# -------------------------------------------
# FPGA functions
# -------------------------------------------

export FPGA

ifeq ($(FPGA),1)

KBUILD_CPPFLAGS += -DFPGA

endif

# -------------------------------------------
# ROM_BUILD functions
# -------------------------------------------

export ROM_BUILD

ifeq ($(ROM_BUILD),1)

KBUILD_CPPFLAGS += -DROM_BUILD

endif

# Limit the length of REVISION_INFO if ROM_BUILD or using rom.lds
ifneq ($(filter 1,$(ROM_BUILD))$(filter rom.lds,$(LDS_FILE)),)
ifeq ($(CHIP),best1000)
REVISION_INFO := x
else
REVISION_INFO := $(GIT_REVISION)
endif
endif

# -------------------------------------------
# PROGRAMMER functions
# -------------------------------------------

export PROGRAMMER

ifeq ($(PROGRAMMER),1)

KBUILD_CPPFLAGS += -DPROGRAMMER

endif

# -------------------------------------------
# ROM_UTILS functions
# -------------------------------------------

export ROM_UTILS_ON ?= 0
ifeq ($(ROM_UTILS_ON),1)
KBUILD_CPPFLAGS += -DROM_UTILS_ON
core-y += utils/rom_utils/
endif

# -------------------------------------------
# Features
# -------------------------------------------

export DEBUG_PORT ?= 1

ifeq ($(CHIP),best1000)
export AUD_SECTION_STRUCT_VERSION ?= 1
else ifeq ($(CHIP),best2000)
export AUD_SECTION_STRUCT_VERSION ?= 1
else
export AUD_SECTION_STRUCT_VERSION ?= 2
endif

ifneq ($(AUD_SECTION_STRUCT_VERSION),)
KBUILD_CPPFLAGS += -DAUD_SECTION_STRUCT_VERSION=$(AUD_SECTION_STRUCT_VERSION)
endif

export FLASH_CHIP
ifneq ($(FLASH_CHIP),)
VALID_FLASH_CHIP_LIST := ALL \
	GD25LQ64C GD25LQ32C GD25LQ16C GD25Q32C GD25Q32D GD25Q80C GD25Q40C GD25Q20C \
	P25Q64L P25Q32L P25Q16L P25Q80H P25Q21H \
	XT25Q08B \
	EN25S80B
ifneq ($(filter-out $(VALID_FLASH_CHIP_LIST),$(FLASH_CHIP)),)
$(error Invalid FLASH_CHIP: $(filter-out $(VALID_FLASH_CHIP_LIST),$(FLASH_CHIP)))
endif
endif

NV_REC_DEV_VER ?= 2

export NO_SLEEP ?= 0

export FAULT_DUMP ?= 1

export USE_TRACE_ID ?= 0

export CRASH_BOOT ?= 0

export OSC_26M_X4_AUD2BB ?= 0
ifeq ($(OSC_26M_X4_AUD2BB),1)
export ANA_26M_X4_ENABLE ?= 1
export FLASH_LOW_SPEED ?= 0
endif

ifeq ($(CHIP),best1400)
export DIGX4_AS_HIGHSPEED_CLK ?= 0
export DIGX2_AS_52M_CLK ?= 0
ifeq ($(DIGX4_AS_HIGHSPEED_CLK),1)
KBUILD_CPPFLAGS += -DDIGX4_AS_HIGHSPEED_CLK
endif
ifeq ($(DIGX2_AS_52M_CLK),1)
KBUILD_CPPFLAGS += -DDIGX2_AS_52M_CLK
endif
endif

export AUDIO_CODEC_ASYNC_CLOSE ?= 0

# Enable the workaround for BEST1000 version C & earlier chips
export CODEC_PLAY_BEFORE_CAPTURE ?= 0

export AUDIO_INPUT_CAPLESSMODE ?= 0

export AUDIO_INPUT_LARGEGAIN ?= 0

export AUDIO_INPUT_MONO ?= 0

export AUDIO_OUTPUT_MONO ?= 0

export AUDIO_OUTPUT_VOLUME_DEFAULT ?= 10

export AUDIO_OUTPUT_INVERT_RIGHT_CHANNEL ?= 0

export AUDIO_OUTPUT_CALIB_GAIN_MISSMATCH ?= 0

export BTUSB_AUDIO_MODE ?= 0
ifeq ($(BTUSB_AUDIO_MODE),1)
KBUILD_CPPFLAGS += -DBTUSB_AUDIO_MODE
endif

export BT_USB_AUDIO_DUAL_MODE ?= 0
ifeq ($(BT_USB_AUDIO_DUAL_MODE),1)
KBUILD_CPPFLAGS += -DBT_USB_AUDIO_DUAL_MODE
endif
ifeq ($(CHIP),best1000)
AUDIO_OUTPUT_DIFF ?= 1
AUDIO_OUTPUT_DC_CALIB ?= $(AUDIO_OUTPUT_DIFF)
AUDIO_OUTPUT_SMALL_GAIN_ATTN ?= 1
AUDIO_OUTPUT_SW_GAIN ?= 1
ANC_L_R_MISALIGN_WORKAROUND ?= 1
else ifeq ($(CHIP),best2000)
ifeq ($(USB_AUDIO_APP),1)
export VCODEC_VOLT ?= 2.5V
else
export VCODEC_VOLT ?= 1.6V
endif
AUDIO_OUTPUT_DIFF ?= 0
ifeq ($(VCODEC_VOLT),2.5V)
AUDIO_OUTPUT_DC_CALIB ?= 0
AUDIO_OUTPUT_DC_CALIB_ANA ?= 1
else
AUDIO_OUTPUT_DC_CALIB ?= 1
AUDIO_OUTPUT_DC_CALIB_ANA ?= 0
endif
ifneq ($(AUDIO_OUTPUT_DIFF),1)
# Class-G module still needs improving
#DAC_CLASSG_ENABLE ?= 1
endif
else ifeq ($(CHIP),best2300)
AUDIO_OUTPUT_DC_CALIB ?= 0
AUDIO_OUTPUT_DC_CALIB_ANA ?= 1
else ifeq ($(CHIP),best1400)
AUDIO_OUTPUT_DC_CALIB ?= 0
AUDIO_OUTPUT_DC_CALIB_ANA ?= 1
endif

ifeq ($(AUDIO_OUTPUT_DC_CALIB)-$(AUDIO_OUTPUT_DC_CALIB_ANA),1-1)
$(error AUDIO_OUTPUT_DC_CALIB and AUDIO_OUTPUT_DC_CALIB_ANA cannot be enabled at the same time)
endif

export AUDIO_OUTPUT_DIFF

export AUDIO_OUTPUT_DC_CALIB

export AUDIO_OUTPUT_DC_CALIB_ANA

export AUDIO_OUTPUT_SMALL_GAIN_ATTN

export AUDIO_OUTPUT_SW_GAIN

export ANC_L_R_MISALIGN_WORKAROUND

export DAC_CLASSG_ENABLE

ifeq ($(ANC_APP),1)
export ANC_FF_ENABLED ?= 1
endif

ifeq ($(CHIP),best1400)
export AUDIO_RESAMPLE ?= 1
else
export AUDIO_RESAMPLE ?= 0
endif

ifeq ($(AUDIO_RESAMPLE),1)
ifeq ($(CHIP),best1000)
export SW_PLAYBACK_RESAMPLE ?= 1
export SW_CAPTURE_RESAMPLE ?= 1
export NO_SCO_RESAMPLE ?= 1
endif # CHIP is best1000
ifeq ($(CHIP),best2000)
export SW_CAPTURE_RESAMPLE ?= 1
export SW_SCO_RESAMPLE ?= 1
export NO_SCO_RESAMPLE ?= 0
endif # CHIP is best2000
ifeq ($(BT_ANC),1)
ifeq ($(NO_SCO_RESAMPLE),1)
$(error BT_ANC and NO_SCO_RESAMPLE cannot be enabled at the same time)
endif
endif # BT_ANC
endif # AUDIO_RESAMPLE

export HW_FIR_DSD_PROCESS ?= 0

export HW_FIR_EQ_PROCESS ?= 0

export SW_IIR_EQ_PROCESS ?= 0

export HW_IIR_EQ_PROCESS ?= 0

export HW_DAC_IIR_EQ_PROCESS ?= 0

export AUDIO_DRC ?= 0

export AUDIO_DRC2 ?= 0

export HW_DC_FILTER_WITH_IIR ?= 0
ifeq ($(HW_DC_FILTER_WITH_IIR),1)
KBUILD_CPPFLAGS += -DHW_DC_FILTER_WITH_IIR
export HW_FILTER_CODEC_IIR ?= 1
endif

ifeq ($(USB_AUDIO_APP),1)
export ANDROID_ACCESSORY_SPEC ?= 1
export FIXED_CODEC_ADC_VOL ?= 0

ifneq ($(BTUSB_AUDIO_MODE),1)
NO_PWRKEY ?= 1
NO_GROUPKEY ?= 1
endif
endif

export NO_PWRKEY

export NO_GROUPKEY

ifneq ($(CHIP),best1000)
ifneq ($(CHIP)-$(TWS),best2000-1)
# For bt
export A2DP_EQ_24BIT ?= 1
# For usb audio
export AUDIO_PLAYBACK_24BIT ?= 1
endif
endif

ifeq ($(CHIP),best1000)

ifeq ($(AUD_PLL_DOUBLE),1)
KBUILD_CPPFLAGS += -DAUD_PLL_DOUBLE
endif

ifeq ($(DUAL_AUX_MIC),1)
ifeq ($(AUDIO_INPUT_MONO),1)
$(error Invalid talk mic configuration)
endif
KBUILD_CPPFLAGS += -D_DUAL_AUX_MIC_
endif

endif # best1000

ifeq ($(CAPTURE_ANC_DATA),1)
KBUILD_CPPFLAGS += -DCAPTURE_ANC_DATA
endif

ifeq ($(AUDIO_ANC_FB_MC),1)
KBUILD_CPPFLAGS += -DAUDIO_ANC_FB_MC
endif

ifeq ($(BT_ANC),1)
KBUILD_CPPFLAGS += -D__BT_ANC__
endif

ifeq ($(WATCHER_DOG),1)
KBUILD_CPPFLAGS += -D__WATCHER_DOG_RESET__
endif

export ULTRA_LOW_POWER ?= 0
ifeq ($(ULTRA_LOW_POWER),1)
export FLASH_LOW_SPEED ?= 1
export PSRAM_LOW_SPEED ?= 1
endif

export USB_HIGH_SPEED ?= 0
ifeq ($(CHIP),best2000)
ifeq ($(USB_HIGH_SPEED),1)
export AUDIO_USE_BBPLL ?= 1
endif
ifeq ($(AUDIO_USE_BBPLL),1)
ifeq ($(MCU_HIGH_PERFORMANCE_MODE),1)
$(error MCU_HIGH_PERFORMANCE_MODE conflicts with AUDIO_USE_BBPLL)
endif
else # !AUDIO_USE_BBPLL
ifeq ($(USB_HIGH_SPEED),1)
$(error AUDIO_USE_BBPLL must be used with USB_HIGH_SPEED)
endif
endif # !AUDIO_USE_BBPLL
endif # best2000

ifeq ($(SIMPLE_TASK_SWITCH),1)
KBUILD_CPPFLAGS += -DSIMPLE_TASK_SWITCH
endif

ifeq ($(ASSERT_SHOW_FILE_FUNC),1)
KBUILD_CPPFLAGS += -DASSERT_SHOW_FILE_FUNC
else
ifeq ($(ASSERT_SHOW_FILE),1)
KBUILD_CPPFLAGS += -DASSERT_SHOW_FILE
else
ifeq ($(ASSERT_SHOW_FUNC),1)
KBUILD_CPPFLAGS += -DASSERT_SHOW_FUNC
endif
endif
endif

ifeq ($(CALIB_SLOW_TIMER),1)
KBUILD_CPPFLAGS += -DCALIB_SLOW_TIMER
endif

ifeq ($(INT_LOCK_EXCEPTION),1)
KBUILD_CPPFLAGS += -DINT_LOCK_EXCEPTION
endif

ifeq ($(TRACE_STR_SECTION),1)
KBUILD_CPPFLAGS += -DTRACE_STR_SECTION
endif

USE_THIRDPARTY ?= 0
ifeq ($(USE_THIRDPARTY),1)
KBUILD_CPPFLAGS += -D__THIRDPARTY
core-y += thirdparty/
endif

export PC_CMD_UART ?= 0
ifeq ($(PC_CMD_UART),1)
KBUILD_CPPFLAGS += -D__PC_CMD_UART__
endif

export AUTO_TEST ?= 0
ifeq ($(AUTO_TEST),1)
KBUILD_CFLAGS += -D_AUTO_TEST_
endif

ifeq ($(RB_CODEC),1)
CPPFLAGS_${LDS_FILE} += -DRB_CODEC
endif

ifneq ($(DATA_BUF_START),)
CPPFLAGS_${LDS_FILE} += -DDATA_BUF_START=$(DATA_BUF_START)
endif

ifeq ($(USER_SECURE_BOOT),1)
core-y += utils/user_secure_boot/ \
               utils/system_info/
endif

ifeq ($(MAX_DAC_OUTPUT),-60db)
MAX_DAC_OUTPUT_FLAGS := -DMAX_DAC_OUTPUT_M60DB
else
ifeq ($(MAX_DAC_OUTPUT),3.75mw)
MAX_DAC_OUTPUT_FLAGS := -DMAX_DAC_OUTPUT_3P75MW
else
ifeq ($(MAX_DAC_OUTPUT),5mw)
MAX_DAC_OUTPUT_FLAGS := -DMAX_DAC_OUTPUT_5MW
else
ifeq ($(MAX_DAC_OUTPUT),10mw)
MAX_DAC_OUTPUT_FLAGS := -DMAX_DAC_OUTPUT_10MW
else
ifneq ($(MAX_DAC_OUTPUT),30mw)
ifneq ($(MAX_DAC_OUTPUT),)
$(error Invalid MAX_DAC_OUTPUT value: $(MAX_DAC_OUTPUT) (MUST be one of: -60db 3.75mw 5mw 10mw 30mw))
endif
endif
endif
endif
endif
endif
export MAX_DAC_OUTPUT_FLAGS

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# BT features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(BT_APP),1)

ifneq ($(filter-out 2M 3M,$(BT_RF_PREFER)),)
$(error Invalid BT_RF_PREFER=$(BT_RF_PREFER))
endif
ifneq ($(BT_RF_PREFER),)
RF_PREFER := $(subst .,P,$(BT_RF_PREFER))
KBUILD_CPPFLAGS += -D__$(RF_PREFER)_PACK__
endif

export AUDIO_SCO_BTPCM_CHANNEL ?= 1
ifeq ($(AUDIO_SCO_BTPCM_CHANNEL),1)
KBUILD_CPPFLAGS += -D_SCO_BTPCM_CHANNEL_
endif

export BT_ONE_BRING_TWO ?= 0
ifeq ($(BT_ONE_BRING_TWO),1)
KBUILD_CPPFLAGS += -D__BT_ONE_BRING_TWO__
endif

export BT_SELECT_PROF_DEVICE_ID ?= 0
ifeq ($(BT_ONE_BRING_TWO),1)
ifeq ($(BT_SELECT_PROF_DEVICE_ID),1)
KBUILD_CPPFLAGS += -D__BT_SELECT_PROF_DEVICE_ID__
endif
endif

export SBC_FUNC_IN_ROM ?= 0
ifeq ($(SBC_FUNC_IN_ROM),1)

KBUILD_CPPFLAGS += -D__SBC_FUNC_IN_ROM__

ifeq ($(CHIP),best2000)
UNALIGNED_ACCESS ?= 1
KBUILD_CPPFLAGS += -D__SBC_FUNC_IN_ROM_VBEST2000_ONLYSBC__
KBUILD_CPPFLAGS += -D__SBC_FUNC_IN_ROM_VBEST2000__
endif
endif

export HFP_1_6_ENABLE ?= 0
ifeq ($(HFP_1_6_ENABLE),1)
KBUILD_CPPFLAGS += -DHFP_1_6_ENABLE
ifeq ($(MSBC_16K_SAMPLE_RATE),0)
KBUILD_CPPFLAGS += -DMSBC_8K_SAMPLE_RATE
export DSP_LIB ?= 1
endif
endif

export A2DP_AAC_ON ?= 0
ifeq ($(A2DP_AAC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_AAC_ON
KBUILD_CPPFLAGS += -D__ACC_FRAGMENT_COMPATIBLE__
endif

export A2DP_LHDC_ON ?= 0
ifeq ($(A2DP_LHDC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LHDC_ON
core-y += thirdparty/audio_codec_lib/liblhdc-dec/
endif
ifeq ($(USER_SECURE_BOOT),1)
KBUILD_CPPFLAGS += -DUSER_SECURE_BOOT
endif

export A2DP_SCALABLE_ON ?= 0
ifeq ($(A2DP_SCALABLE_ON),1)
KBUILD_CPPFLAGS += -DA2DP_SCALABLE_ON
core-y += thirdparty/audio_codec_lib/scalable/
endif

export A2DP_LDAC_ON ?= 0
ifeq ($(A2DP_LDAC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LDAC_ON
core-y += thirdparty/audio_codec_lib/ldac/
endif

export A2DP_CP_ACCEL ?= 0
ifeq ($(A2DP_CP_ACCEL),1)
KBUILD_CPPFLAGS += -DA2DP_CP_ACCEL
endif

ifeq ($(BT_XTAL_SYNC),1)
KBUILD_CPPFLAGS += -DBT_XTAL_SYNC_NEW_METHOD
KBUILD_CPPFLAGS += -DFIXED_BIT_OFFSET_TARGET
endif

ifeq ($(APP_LINEIN_A2DP_SOURCE),1)
KBUILD_CPPFLAGS += -DAPP_LINEIN_A2DP_SOURCE
endif

ifeq ($(HSP_ENABLE),1)
KBUILD_CPPFLAGS += -D__HSP_ENABLE__
endif

ifeq ($(APP_I2S_A2DP_SOURCE),1)
KBUILD_CPPFLAGS += -DAPP_I2S_A2DP_SOURCE
endif

export TX_RX_PCM_MASK ?= 0
ifeq ($(TX_RX_PCM_MASK),1)
KBUILD_CPPFLAGS += -DTX_RX_PCM_MASK
endif

export PCM_FAST_MODE ?= 0
ifeq ($(PCM_FAST_MODE),1)
KBUILD_CPPFLAGS += -DPCM_FAST_MODE
endif

export LOW_DELAY_SCO ?= 0
ifeq ($(LOW_DELAY_SCO),1)
KBUILD_CPPFLAGS += -DLOW_DELAY_SCO
endif

export CVSD_BYPASS ?= 0
ifeq ($(CVSD_BYPASS),1)
KBUILD_CPPFLAGS += -DCVSD_BYPASS
endif

export SCO_DMA_SNAPSHOT ?= 0
ifeq ($(SCO_DMA_SNAPSHOT),1)
KBUILD_CPPFLAGS += -DSCO_DMA_SNAPSHOT
endif

export SCO_OPTIMIZE_FOR_RAM ?= 0
ifeq ($(SCO_OPTIMIZE_FOR_RAM),1)
KBUILD_CPPFLAGS += -DSCO_OPTIMIZE_FOR_RAM
endif

export AAC_TEXT_PARTIAL_IN_FLASH ?= 0
ifeq ($(AAC_TEXT_PARTIAL_IN_FLASH),1)
KBUILD_CPPFLAGS += -DAAC_TEXT_PARTIAL_IN_FLASH
endif

ifeq ($(BLE),1)
ifeq ($(A2DP_LHDC_ON),1)
export INTERSYS_NO_THREAD ?= 1
else
export INTERSYS_NO_THREAD ?= 0
endif
else
export INTERSYS_NO_THREAD ?= 1
endif

ifeq ($(SUPPORT_BATTERY_REPORT),1)
KBUILD_CPPFLAGS += -DSUPPORT_BATTERY_REPORT
endif

ifeq ($(SUPPORT_HF_INDICATORS),1)
KBUILD_CPPFLAGS += -DSUPPORT_HF_INDICATORS
endif

ifeq ($(SUPPORT_SIRI),1)
KBUILD_CPPFLAGS += -DSUPPORT_SIRI
endif

export BQB_PROFILE_TEST ?= 0
ifeq ($(BQB_PROFILE_TEST),1)
KBUILD_CPPFLAGS += -D__BQB_PROFILE_TEST__
endif

export AUDIO_SPECTRUM ?= 0
ifeq ($(AUDIO_SPECTRUM),1)
KBUILD_CPPFLAGS += -D__AUDIO_SPECTRUM__
endif

export INTERCONNECTION ?= 0
ifeq ($(INTERCONNECTION),1)
KBUILD_CPPFLAGS += -D__INTERCONNECTION__
BES_OTA_BASIC := 1
endif

ifeq ($(SUSPEND_ANOTHER_DEV_A2DP_STREAMING_WHEN_CALL_IS_COMING),1)
KBUILD_CPPFLAGS += -DSUSPEND_ANOTHER_DEV_A2DP_STREAMING_WHEN_CALL_IS_COMING
endif

export LBRT ?= 0
ifeq ($(LBRT),1)
KBUILD_CPPFLAGS += -DLBRT
endif

export IBRT ?= 0
ifeq ($(IBRT),1)
KBUILD_CPPFLAGS += -DIBRT
KBUILD_CPPFLAGS += -DIBRT_BLOCKED
KBUILD_CPPFLAGS += -DIBRT_NOT_USE
endif

export BES_AUD ?= 0
ifeq ($(BES_AUD),1)
KBUILD_CPPFLAGS += -DBES_AUD
endif

export IBRT_SEARCH_UI ?= 0
ifeq ($(IBRT_SEARCH_UI),1)
KBUILD_CPPFLAGS += -DIBRT_SEARCH_UI
endif

endif # BT_APP

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# BLE features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
export BLE ?= 0
ifeq ($(BLE),1)

KBUILD_CPPFLAGS += -D__IAG_BLE_INCLUDE__

IS_USE_BLE_DUAL_CONNECTION ?= 1
ifeq ($(IS_USE_BLE_DUAL_CONNECTION),1)
KBUILD_CPPFLAGS += -DBLE_CONNECTION_MAX=2
else
KBUILD_CPPFLAGS += -DBLE_CONNECTION_MAX=1
endif

ifeq ($(IS_ENABLE_DEUGGING_MODE),1)
KBUILD_CPPFLAGS += -DIS_ENABLE_DEUGGING_MODE
endif

export SMARTVOICE ?= 0
ifeq ($(SMARTVOICE),1)
KBUILD_CPPFLAGS += -D__SMARTVOICE__
endif

IS_MULTI_AI_ENABLED ?= 0
ifeq ($(IS_MULTI_AI_ENABLED),1)
KBUILD_CPPFLAGS += -DIS_MULTI_AI_ENABLED
endif

export VOICE_DATAPATH_ENABLED ?= 0
ifeq ($(VOICE_DATAPATH_ENABLED),1)
KBUILD_CPPFLAGS += -DVOICE_DATAPATH
endif

ifeq ($(GFPS_ENABLE),1)
KBUILD_CPPFLAGS += -D_GFPS_
endif

ifeq ($(MIX_MIC_DURING_MUSIC_ENABLED),1)
KBUILD_CPPFLAGS += -DMIX_MIC_DURING_MUSIC
endif

endif # BLE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Speech features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
export SPEECH_TX_DC_FILTER ?= 0
ifeq ($(SPEECH_TX_DC_FILTER),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_DC_FILTER
endif

export SPEECH_TX_MIC_CALIBRATION ?= 0
ifeq ($(SPEECH_TX_MIC_CALIBRATION),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_MIC_CALIBRATION
endif

export SPEECH_TX_MIC_FIR_CALIBRATION ?= 0
ifeq ($(SPEECH_TX_MIC_FIR_CALIBRATION),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_MIC_FIR_CALIBRATION
endif

export SPEECH_TX_AEC ?= 0
ifeq ($(SPEECH_TX_AEC),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_AEC
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_AEC2 ?= 0
ifeq ($(SPEECH_TX_AEC2),1)
$(error SPEECH_TX_AEC2 is not supported now, use SPEECH_TX_AEC2FLOAT instead)
KBUILD_CPPFLAGS += -DSPEECH_TX_AEC2
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_AEC2FLOAT ?= 0
ifeq ($(SPEECH_TX_AEC2FLOAT),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_AEC2FLOAT
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export DSP_LIB ?= 1
endif

export SPEECH_TX_NS ?= 0
ifeq ($(SPEECH_TX_NS),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_NS
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_NS2 ?= 0
ifeq ($(SPEECH_TX_NS2),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_NS2
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
KBUILD_CPPFLAGS += -DLC_MMSE_FRAME_LENGTH=$(LC_MMSE_FRAME_LENGTH)
endif

export SPEECH_TX_NS2FLOAT ?= 0
ifeq ($(SPEECH_TX_NS2FLOAT),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_NS2FLOAT
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export DSP_LIB ?= 1
endif

export SPEECH_TX_NS3 ?= 0
ifeq ($(SPEECH_TX_NS3),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_NS3
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_WNR ?= 0
ifeq ($(SPEECH_TX_WNR),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_WNR
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_CODEC_CAPTURE_CHANNEL_NUM ?= 1

export SPEECH_TX_2MIC_NS ?= 0
ifeq ($(SPEECH_TX_2MIC_NS),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

export SPEECH_TX_2MIC_NS2 ?= 0
ifeq ($(SPEECH_TX_2MIC_NS2),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS2
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
KBUILD_CPPFLAGS += -DCOH_FRAME_LENGTH=$(COH_FRAME_LENGTH)
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

export SPEECH_TX_2MIC_NS3 ?= 0
ifeq ($(SPEECH_TX_2MIC_NS3),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS3
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

export SPEECH_TX_2MIC_NS4 ?= 0
ifeq ($(SPEECH_TX_2MIC_NS4),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS4
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

export SPEECH_TX_2MIC_NS5 ?= 0
ifeq ($(SPEECH_TX_2MIC_NS5),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS5
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

export SPEECH_TX_3MIC_NS ?= 0
ifeq ($(SPEECH_TX_3MIC_NS),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_3MIC_NS
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
# Get 1 channel from sensor
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 3
endif

ifeq ($(APP_LINEIN_SOURCE),1)
KBUILD_CPPFLAGS += -DAPP_LINEIN_SOURCE
endif

ifeq ($(WL_NSX),1)
KBUILD_CPPFLAGS += -DWL_NSX
KBUILD_CPPFLAGS += -DSTATIC_MEM
endif

ifeq ($(WL_AEC),1)
KBUILD_CPPFLAGS += -DWL_AEC
endif

ifeq ($(WL_AGC),1)
KBUILD_CPPFLAGS += -DWEBRTC_AGC
endif

ifeq ($(WL_AGC_32K),1)
KBUILD_CPPFLAGS += -DWEBRTC_AGC_32K
endif

export WL_NSX_5MS ?= 0
ifeq ($(WL_NSX_5MS),1)
KBUILD_CPPFLAGS += -DWL_NSX_5MS
endif

export WL_FIR_FILTER ?= 0
ifeq ($(WL_FIR_FILTER),1)
KBUILD_CPPFLAGS += -DWL_FIR_FILTER
endif

export SPEECH_CODEC_CAPTURE_SAMPLE = 16000
ifeq ($(WL_HIGH_SAMPLE),1)
KBUILD_CPPFLAGS += -DWL_HIGH_SAMPLE
export SPEECH_CODEC_CAPTURE_SAMPLE = 32000
endif

ifeq ($(WL_VAD),1)
KBUILD_CPPFLAGS += -DWL_VAD
KBUILD_CPPFLAGS += -DSTATIC_MEM
endif

export SPEECH_TX_NOISE_GATE ?= 0
ifeq ($(SPEECH_TX_NOISE_GATE),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_NOISE_GATE
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_COMPEXP ?= 0
ifeq ($(SPEECH_TX_COMPEXP),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_COMPEXP
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_AGC ?= 0
ifeq ($(SPEECH_TX_AGC),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_AGC
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_EQ ?= 0
ifeq ($(SPEECH_TX_EQ),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_EQ
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export DSP_LIB ?= 1
endif

export SPEECH_TX_POST_GAIN ?= 0
ifeq ($(SPEECH_TX_POST_GAIN),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_POST_GAIN
endif

export SPEECH_RX_NS ?= 0
ifeq ($(SPEECH_RX_NS),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_NS
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_RX_NS2 ?= 0
ifeq ($(SPEECH_RX_NS2),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_NS2
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_RX_NS2FLOAT ?= 0
ifeq ($(SPEECH_RX_NS2FLOAT),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_NS2FLOAT
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export DSP_LIB ?= 1
endif

export SPEECH_RX_NS3 ?= 0
ifeq ($(SPEECH_RX_NS3),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_NS3
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_RX_AGC ?= 0
ifeq ($(SPEECH_RX_AGC),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_AGC
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export AUDIO_LOOPBACK ?= 0
ifeq ($(AUDIO_LOOPBACK),1)
KBUILD_CPPFLAGS += -DAUDIO_LOOPBACK
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 1
endif

export MIC_16K_LOOPBACK ?= 0
ifeq ($(MIC_16K_LOOPBACK),1)
KBUILD_CPPFLAGS += -DMIC_16K_LOOPBACK
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 1
endif

export MIC_32K_LOOPBACK ?= 0
ifeq ($(MIC_32K_LOOPBACK),1)
KBUILD_CPPFLAGS += -DMIC_32K_LOOPBACK
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
export SPEECH_CODEC_CAPTURE_SAMPLE = 32000
endif

export LINGJI_CODEC ?=0
ifeq ($(LINGJI_CODEC),1)
KBUILD_CPPFLAGS += -DLINGJI_CODEC
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

export OPUS_LOOPBACK ?= 1
ifeq ($(OPUS_LOOPBACK),1)
KBUILD_CPPFLAGS += -DOPUS_LOOPBACK
endif


export WL_GPIO_SWITCH ?= 0
ifeq ($(WL_GPIO_SWITCH),1)
KBUILD_CPPFLAGS += -DWL_GPIO_SWITCH
endif

export WL_LED_ZG_SWITCH ?= 0
ifeq ($(WL_LED_ZG_SWITCH),1)
KBUILD_CPPFLAGS += -DWL_LED_ZG_SWITCH
endif

export AUDIO_DEBUG ?= 0
ifeq ($(AUDIO_DEBUG),1)
KBUILD_CPPFLAGS += -DAUDIO_DEBUG
endif

export WL_STEREO_AUDIO ?= 0
ifeq ($(WL_STEREO_AUDIO),1)
KBUILD_CPPFLAGS += -DWL_STEREO_AUDIO
endif

export WL_DEBUG_MODE ?= 0
ifeq ($(WL_DEBUG_MODE),1)
KBUILD_CPPFLAGS += -DWL_DEBUG_MODE
endif

export GCC_PLAT ?= 0
ifeq ($(GCC_PLAT),1)
KBUILD_CPPFLAGS += -DGCC_PLAT
endif

export NOTCH_FILTER ?= 0
ifeq ($(NOTCH_FILTER),1)
KBUILD_CPPFLAGS += -DNOTCH_FILTER
endif

export WL_TEL_DENOISE ?= 0
ifeq ($(WL_TEL_DENOISE),1)
KBUILD_CPPFLAGS += -DWL_TEL_DENOISE
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

export SPEECH_RX_EQ ?= 0
ifeq ($(SPEECH_RX_EQ),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_EQ
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export DSP_LIB ?= 1
endif

export SPEECH_RX_POST_GAIN ?= 0
ifeq ($(SPEECH_RX_POST_GAIN),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_POST_GAIN
endif

export SPEECH_PROCESS_FRAME_MS 	?= 16

export SPEECH_SCO_FRAME_MS 		?= 15

export SPEECH_SIDETONE ?= 0
ifeq ($(SPEECH_SIDETONE),1)
KBUILD_CPPFLAGS += -DSPEECH_SIDETONE
ifeq ($(CHIP),best2000)
# Disable SCO resample
export SW_SCO_RESAMPLE := 0
export NO_SCO_RESAMPLE := 1
endif
endif

ifeq ($(THRIDPARTY_LIB),aispeech)
export DSP_LIB ?= 1
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Features for full application projects
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(FULL_APP_PROJECT),1)

export SPEECH_LIB ?= 1

export APP_TEST_AUDIO ?= 0

export APP_TEST_MODE ?= 0
ifeq ($(APP_TEST_MODE),1)
KBUILD_CPPFLAGS += -DAPP_TEST_MODE
endif

export VOICE_PROMPT ?= 1

export AUDIO_QUEUE_SUPPORT ?= 1

export VOICE_RECOGNITION ?= 0

export ENGINEER_MODE ?= 1
ifeq ($(ENGINEER_MODE),1)
FACTORY_MODE := 1
endif
ifeq ($(FACTORY_MODE),1)
KBUILD_CPPFLAGS += -D__FACTORY_MODE_SUPPORT__
endif

ifeq ($(HEAR_THRU_PEAK_DET),1)
KBUILD_CPPFLAGS += -D__HEAR_THRU_PEAK_DET__
endif

KBUILD_CPPFLAGS += -DSPEECH_PROCESS_FRAME_MS=$(SPEECH_PROCESS_FRAME_MS)
KBUILD_CPPFLAGS += -DSPEECH_SCO_FRAME_MS=$(SPEECH_SCO_FRAME_MS)

KBUILD_CPPFLAGS += -DMULTIPOINT_DUAL_SLAVE

endif # FULL_APP_PROJECT

ifeq ($(SPEECH_LIB),1)

export DSP_LIB ?= 1

ifeq ($(USB_AUDIO_APP),1)
ifneq ($(USB_AUDIO_SEND_CHAN),$(SPEECH_CODEC_CAPTURE_CHANNEL_NUM))
$(info )
$(info CAUTION: Change USB_AUDIO_SEND_CHAN($(USB_AUDIO_SEND_CHAN)) to SPEECH_CODEC_CAPTURE_CHANNEL_NUM($(SPEECH_CODEC_CAPTURE_CHANNEL_NUM)))
$(info )
export USB_AUDIO_SEND_CHAN := $(SPEECH_CODEC_CAPTURE_CHANNEL_NUM)
ifneq ($(USB_AUDIO_SEND_CHAN),$(SPEECH_CODEC_CAPTURE_CHANNEL_NUM))
$(error ERROR: Failed to change USB_AUDIO_SEND_CHAN($(USB_AUDIO_SEND_CHAN)))
endif
endif
endif

KBUILD_CPPFLAGS += -DSPEECH_CODEC_CAPTURE_CHANNEL_NUM=$(SPEECH_CODEC_CAPTURE_CHANNEL_NUM)

KBUILD_CPPFLAGS += -DSPEECH_CODEC_CAPTURE_SAMPLE=$(SPEECH_CODEC_CAPTURE_SAMPLE)

endif # SPEECH_LIB

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Put customized features above
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Obsoleted features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
OBSOLETED_FEATURE_LIST := EQ_PROCESS RB_CODEC AUDIO_EQ_PROCESS MEDIA_PLAYER_RBCODEC
USED_OBSOLETED_FEATURE := $(strip $(foreach f,$(OBSOLETED_FEATURE_LIST),$(if $(filter 1,$($f)),$f)))
ifneq ($(USED_OBSOLETED_FEATURE),)
$(error Obsoleted features: $(USED_OBSOLETED_FEATURE))
endif

# -------------------------------------------
# General
# -------------------------------------------

ifneq ($(NO_CONFIG),1)
core-y += config/
endif

ifneq ($(NO_BOOT_STRUCT),1)
core-y += $(call add_if_exists,utils/boot_struct/)
endif

export DEFAULT_CFG_SRC ?= _default_cfg_src_

ifneq ($(wildcard $(srctree)/config/$(T)/tgt_hardware.h $(srctree)/config/$(T)/res/),)
KBUILD_CPPFLAGS += -Iconfig/$(T)
endif
KBUILD_CPPFLAGS += -Iconfig/$(DEFAULT_CFG_SRC)

CPU_CFLAGS := -mcpu=cortex-m4 -mthumb

export UNALIGNED_ACCESS ?= 1
ifeq ($(UNALIGNED_ACCESS),1)
KBUILD_CPPFLAGS += -DUNALIGNED_ACCESS
else
CPU_CFLAGS += -mno-unaligned-access
endif

ifeq ($(CHIP_HAS_FPU),1)
CPU_CFLAGS += -mfpu=fpv4-sp-d16
ifeq ($(SOFT_FLOAT_ABI),1)
CPU_CFLAGS += -mfloat-abi=softfp
else
CPU_CFLAGS += -mfloat-abi=hard
endif
else
CPU_CFLAGS += -mfloat-abi=soft
endif

ifneq ($(ALLOW_WARNING),1)
KBUILD_CPPFLAGS += -Werror
endif

ifeq ($(PIE),1)
ifneq ($(NOSTD),1)
$(error PIE can only work when NOSTD=1)
endif
KBUILD_CPPFLAGS += -fPIE -msingle-pic-base
# -pie option will generate .dynamic section
#LDFLAGS += -pie
#LDFLAGS += -z relro -z now
endif

KBUILD_CPPFLAGS += $(CPU_CFLAGS) $(SPECS_CFLAGS) -DTOOLCHAIN_GCC -DTARGET_BEST1000
LINK_CFLAGS += $(CPU_CFLAGS) $(SPECS_CFLAGS)
CFLAGS_IMAGE += $(CPU_CFLAGS) $(SPECS_CFLAGS)

# Save 100+ bytes by filling less alignment holes
# TODO: Array alignment?
#LDFLAGS += --sort-common --sort-section=alignment

ifeq ($(CTYPE_PTR_DEF),1)
LDFLAGS_IMAGE += --defsym __ctype_ptr__=0
endif



# __LINUX_KERNEL__ is used in OS-independent header files
MV_DEFINE := -DBERLIN -DSoC_Galois -D__LINUX_KERNEL__ -D__CODE_LINK__=0 -DCPUINDEX=0
MV_DEFINE += -DBERLIN_B_0=0x1000 \
             -DBERLIN_C_0=0x2000 \
             -DBERLIN_BG2=0x3000 \
             -DBERLIN_BG2_Z2=0x3010 \
             -DBERLIN_BG2_A0=0x3020 \
             -DBERLIN_BG2_CD=0x3030 \
             -DBERLIN_BG2_CDP=0x3040 \
             -DBERLIN_BG3_CD=0x3040 \
             -DBERLIN_BG2_CT=0x3050 \
             -DBERLIN_BG2_Q=0x3060 \
             -DBERLIN_BG2Q_A1=0x3070 \
             -DBERLIN_BG2_DTV=0x3080 \
             -DBERLIN_BG2_DTV_A0=0x3090 \
             -DBERLIN_BG4_CD=0x30A0 \
             -DBERLIN_BG4_CT=0x30B0 \
             -DBERLIN_BG4_DTV=0x30C0 \
             -DBERLIN_BG4_CT_A0=0x30D0 \
             -DBERLIN_BG4_CDP=0x30E0 \
             -DBERLIN_BG4_CDP_A0=0x30F0

MV_DEFINE += -DSOC_MINOR_REV_Z1=0x1  \
             -DSOC_MINOR_REV_Z2=0x2  \
             -DSOC_MINOR_REV_A0=0x4  \
             -DSOC_MINOR_REV_A1=0x5

MV_DEFINE += -DBERLIN_BG4CDP_A0_EXT=0xA0
###############################################################################
#
# AMP kernel module configuration
#
###############################################################################

###############################################################################
# Here defines the default AMP kernel module configuration settings
###############################################################################
CONFIG_FASTLOGO=y

# enable or disable AVIF driver
CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE=y

# enable or disable AIP driver
CONFIG_MV_AMP_COMPONENT_AIP_ENABLE=y

# enable or disable VIP driver
CONFIG_MV_AMP_COMPONENT_VIP_ENABLE=n

# enable or disable HDMI RX driver
CONFIG_MV_AMP_COMPONENT_HDMIRX_ENABLE=y

# enable or disable AUDIO LoRo
CONFIG_MV_AMP_AUDIO_PATH_20_ENABLE=y

# enable or disable AUDIO SPDIF
CONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE=y

# enable or disable AUDIO HDMI PASSTHROUGH
CONFIG_MV_AMP_AUDIO_PATH_HDMI_ENABLE=y

# enable or disable HW APP module
CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE=y

# enable or disable ZSP module
CONFIG_MV_AMP_COMPONENT_ZSP_ENABLE=y

# enable or disable VMETA
CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE=y

# enable or disable V2G
CONFIG_MV_AMP_COMPONENT_V2G_ENABLE=y

# enable or disable CPM
CONFIG_MV_AMP_COMPONENT_CPM_ENABLE=n

#enable or disable OVP
CONFIG_MV_AMP_COMPONENT_OVP_ENABLE=n

###############################################################################
# Setup the AMP kernel module configuration accordingly
###############################################################################
ifeq ($(MV_AMP_COMPONENT_AVIN_ENABLE), false)
CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE=n
CONFIG_MV_AMP_COMPONENT_AIP_ENABLE=n
CONFIG_MV_AMP_COMPONENT_VIP_ENABLE=n
CONFIG_MV_AMP_COMPONENT_HDMIRX_ENABLE=n
endif

ifeq ($(MV_AMP_AUDIO_SNDSRV_PATH_71_DISABLE), true)
CONFIG_MV_AMP_AUDIO_PATH_71_ENABLE=n
endif

ifeq ($(MV_AMP_AUDIO_SNDSRV_PATH_20_DISABLE), true)
CONFIG_MV_AMP_AUDIO_PATH_20_ENABLE=n
endif

ifeq ($(MV_AMP_AUDIO_SNDSRV_PATH_SPDIF_DISABLE), true)
CONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE=n
endif

ifeq ($(MV_AMP_AUDIO_SNDSRV_PATH_HDMI_DISABLE), true)
CONFIG_MV_AMP_AUDIO_PATH_HDMI_ENABLE=n
endif

###############################################################################
#
# Common AMP kernel module objects
#
###############################################################################
amp_core-objs := \
	amp_driver.o \
	drv_msg.o \
	drv_aout.o \
	drv_vpu.o \
	drv_vpp.o

###############################################################################
#
# Chip specific section
#
###############################################################################

ifeq ($(MV_BERLIN_CHIP_VERSION), BERLIN_BG4_CT_A0)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG4_CT_A0
        MV_DEFINE += -DAMP_ACTIVE_STANDBY
        FIRMWARE_INC_DIR_LOCAL = $(srctree)/drivers/soc/berlin/modules/amp/inc/Firmware_Berlin_BG4_CT_A0
        CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_AIP_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_VIP_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_HDMIRX_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_V2G_ENABLE=y
        CONFIG_MV_AMP_COMPONENT_H1_ENABLE=y
        CONFIG_MV_AMP_COMPONENT_CPM_ENABLE=y
        CONFIG_MV_AMP_COMPONENT_OVP_ENABLE=y
        CONFIG_MV_AMP_COMPONENT_SM_CEC_ENABLE=y
endif


ifeq ($(MV_BERLIN_CHIP_VERSION), BERLIN_BG4_DTV)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG4_DTV
        FIRMWARE_INC_DIR_LOCAL = $(srctree)/drivers/soc/berlin/modules/amp/inc/Firmware_Berlin_BG4_DTV
        CONFIG_MV_AMP_COMPONENT_VIP_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_HDMIRX_ENABLE=n
        CONFIG_MV_AMP_AUDIO_PATH_20_ENABLE=y
        CONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE=y
        CONFIG_MV_AMP_AUDIO_PATH_HDMI_ENABLE=y
        CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_ZSP_ENABLE=y
        CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_V2G_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_CPM_ENABLE=y
endif


ifeq ($(CONFIG_AMP_BERLIN_BG4CDP),y)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG4_CDP
        FIRMWARE_INC_DIR_LOCAL = $(srctree)/drivers/soc/berlin/modules/amp/inc/Firmware_Berlin_BG4_CDP
        CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_AIP_ENABLE=y
        CONFIG_MV_AMP_COMPONENT_VIP_ENABLE=y
        CONFIG_MV_AMP_COMPONENT_HDMIRX_ENABLE=n
        CONFIG_MV_AMP_AUDIO_PATH_20_ENABLE=n
        CONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE=n
        CONFIG_MV_AMP_AUDIO_PATH_HDMI_ENABLE=y
        CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_ZSP_ENABLE=y
        CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_V2G_ENABLE=y
        CONFIG_MV_AMP_COMPONENT_CPM_ENABLE=n
        CONFIG_MV_AMP_COMPONENT_SM_CEC_ENABLE=y
        CONFIG_MV_AMP_COMPONENT_OVP_ENABLE=n
        CONFIG_FASTLOGO=n
endif

###############################################################################
#
# Collect the configurations and generate the build rules
#
###############################################################################

ifeq ($(CONFIG_MV_AMP_COMPONENT_SM_CEC_ENABLE), y)
        MV_DEFINE += -DCONFIG_MV_AMP_COMPONENT_SM_CEC_ENABLE
endif

ifeq ($(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE), y)
        MV_DEFINE += -DCONFIG_MV_AMP_COMPONENT_AVIN_ENABLE
endif

ifeq ($(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE), y)
        MV_DEFINE += -DCONFIG_MV_AMP_COMPONENT_AIP_ENABLE
endif

ifeq ($(CONFIG_MV_AMP_COMPONENT_VIP_ENABLE), y)
        MV_DEFINE += -DCONFIG_MV_AMP_COMPONENT_VIP_ENABLE
endif


ifeq ($(CONFIG_MV_AMP_COMPONENT_HDMIRX_ENABLE), y)
        MV_DEFINE += -DCONFIG_MV_AMP_COMPONENT_HDMIRX_ENABLE
endif

ifeq ($(CONFIG_MV_AMP_AUDIO_PATH_20_ENABLE), y)
        MV_DEFINE += -DKCONFIG_MV_AMP_AUDIO_PATH_20_ENABLE
endif

ifeq ($(CONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE), y)
        MV_DEFINE += -DKCONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE
endif

ifeq ($(CONFIG_MV_AMP_AUDIO_PATH_HDMI_ENABLE), y)
        MV_DEFINE += -DKCONFIG_MV_AMP_AUDIO_PATH_HDMI_ENABLE
endif

ifeq ($(CONFIG_MV_AMP_COMPONENT_ZSP_ENABLE), y)
        MV_DEFINE += -DKCONFIG_MV_AMP_COMPONENT_ZSP_ENABLE
endif

ifeq ($(CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE), y)
        MV_DEFINE += -DCONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE
endif

ifeq ($(CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE), y)
        MV_DEFINE += -DCONFIG_MV_AMP_COMPONENT_VMETA_ENABLE
endif

ifeq ($(CONFIG_MV_AMP_COMPONENT_V2G_ENABLE), y)
        MV_DEFINE += -DCONFIG_MV_AMP_COMPONENT_V2G_ENABLE
endif

ifeq ($(CONFIG_MV_AMP_COMPONENT_CPM_ENABLE), y)
        MV_DEFINE += -DCONFIG_MV_AMP_COMPONENT_CPM_ENABLE
endif

ifeq ($(CONFIG_MV_AMP_COMPONENT_OVP_ENABLE), y)
        MV_DEFINE += -DCONFIG_MV_AMP_COMPONENT_OVP_ENABLE
        MV_DEFINE += -DOVP_IN_TZ
endif

MV_DEFINE += -I$(srctree)/drivers/soc/berlin/modules/tzd/tee/client_api \
             -I$(srctree)/drivers/soc/berlin/modules/tzd/tee/common \
             -I$(srctree)/drivers/soc/berlin/modules/tzd/include

###############################################################################
#
# Common CPM kernel module objects
#
###############################################################################
ifeq ($(CONFIG_MV_AMP_COMPONENT_CPM_ENABLE), y)
CPM_ABS_PATH := $(srctree)/drivers/soc/berlin/modules/amp/cpm
CPM_API_OBJS := cpm/source/cpm_driver.o

CPM_INC_PATH := -I$(CPM_ABS_PATH)/include

ifeq ($(MV_BERLIN_CHIP_VERSION), BERLIN_BG4_CT)
        CPM_API_OBJS += cpm/source/cpm_bg4ct.o
endif

ifeq ($(MV_BERLIN_CHIP_VERSION), BERLIN_BG4_CT_A0)
        CPM_API_OBJS += cpm/source/cpm_bg4ct.o
        MV_DEFINE += -DGFX3D_V4G_CHECK
endif

ifeq ($(MV_BERLIN_CHIP_VERSION), BERLIN_BG4_DTV)
        CPM_API_OBJS += cpm/source/cpm_bg4dtv.o
endif
endif

###############################################################################
#
# Common OVP kernel module objects
#
###############################################################################

ifeq ($(CONFIG_MV_AMP_COMPONENT_OVP_ENABLE), y)
OVP_ABS_PATH := $(srctree)/drivers/soc/berlin/modules/amp/ovp
OVP_API_OBJS := ovp/source/ovp_api_wrap.o
OVP_CA_OBJS  := ovp/ca/ovp_dev_ca.o
OVP_INC_PATH := -I$(OVP_ABS_PATH)/include
endif
###############################################################################
#
# Common DHUB kernel module objects
#
###############################################################################
DHUB_ABS_PATH := $(srctree)/drivers/soc/berlin/modules/amp/dhub
DHUB_API_OBJS := dhub/source/avio_dhub_drv.o    \
                 dhub/source/hal_dhub_wrap.o    \
                 dhub/source/hal_dhub.o         \
                 dhub/ca/tee_ca_dhub.o

DHUB_INC_PATH := -I$(DHUB_ABS_PATH)/include
DHUB_CFG_DIR  := dhub/configs
####  add soc specific dhub cfg###############################################
ifeq ($(MV_BERLIN_CHIP_VERSION), BERLIN_BG4_CD)
DHUB_CFG_OBJS := $(DHUB_CFG_DIR)/bg4cd/avio_dhub_cfg.o
ifeq ($(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE), y)
DHUB_CFG_OBJS += $(DHUB_CFG_DIR)/bg4cd/avif_dhub_cfg.o
endif
DHUB_INC_PATH += -I$(DHUB_ABS_PATH)/configs/bg4cd \
        -I$(FIRMWARE_INC_DIR_LOCAL)
endif

ifeq ($(MV_BERLIN_CHIP_VERSION), BERLIN_BG4_CT)
DHUB_CFG_OBJS := $(DHUB_CFG_DIR)/bg4ct/avio_dhub_cfg.o
ifeq ($(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE), y)
DHUB_CFG_OBJS += $(DHUB_CFG_DIR)/bg4ct/avif_dhub_cfg.o
endif
DHUB_INC_PATH += -I$(DHUB_ABS_PATH)/configs/bg4ct \
        -I$(FIRMWARE_INC_DIR_LOCAL)
endif

ifeq ($(MV_BERLIN_CHIP_VERSION), BERLIN_BG4_CT_A0)
DHUB_CFG_OBJS := $(DHUB_CFG_DIR)/bg4ct/avio_dhub_cfg.o
ifeq ($(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE), y)
DHUB_CFG_OBJS += $(DHUB_CFG_DIR)/bg4ct/avif_dhub_cfg.o
endif
DHUB_INC_PATH += -I$(DHUB_ABS_PATH)/configs/bg4ct \
        -I$(FIRMWARE_INC_DIR_LOCAL)
endif

ifeq ($(MV_BERLIN_CHIP_VERSION), BERLIN_BG4_DTV)
DHUB_CFG_OBJS := $(DHUB_CFG_DIR)/bg4dtv/avio_dhub_cfg.o
ifeq ($(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE), y)
DHUB_CFG_OBJS += $(DHUB_CFG_DIR)/bg4dtv/avif_dhub_cfg.o
endif
DHUB_INC_PATH += -I$(DHUB_ABS_PATH)/configs/bg4dtv \
        -I$(FIRMWARE_INC_DIR_LOCAL)
endif

ifeq ($(CONFIG_AMP_BERLIN_BG4CDP),y)
DHUB_CFG_OBJS := $(DHUB_CFG_DIR)/bg4cdp/avio_dhub_cfg.o
DHUB_INC_PATH += -I$(DHUB_ABS_PATH)/configs/bg4cdp \
        -I$(FIRMWARE_INC_DIR_LOCAL)
endif

DHUB_ALL_OBJS := $(DHUB_API_OBJS)   \
                 $(DHUB_CFG_OBJS)

CPM_ALL_OBJS := $(CPM_API_OBJS)

ifeq ($(CONFIG_MV_AMP_COMPONENT_OVP_ENABLE), y)
OVP_ALL_OBJS :=  $(OVP_API_OBJS) \
                $(OVP_CA_OBJS)
endif

amp_core-objs += \
    $(DHUB_ALL_OBJS)\
    $(CPM_ALL_OBJS)

ifeq ($(CONFIG_MV_AMP_COMPONENT_OVP_ENABLE), y)
    amp_core-objs += $(OVP_ALL_OBJS)
endif

###############################################################################
#
# Configurable kernel module objects
#
###############################################################################
ifeq ($(CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE), y)
amp_core-objs += drv_app.o
endif
ifeq ($(CONFIG_MV_AMP_COMPONENT_ZSP_ENABLE), y)
amp_core-objs += drv_zsp.o
endif
ifeq ($(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE), y)
amp_core-objs += drv_avif.o
else
ifeq ($(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE), y)
amp_core-objs += drv_avif.o
endif
endif
ifeq ($(CONFIG_MV_AMP_COMPONENT_OVP_ENABLE), y)
amp_core-objs += drv_ovp.o
endif
ifeq ($(CONFIG_MV_AMP_COMPONENT_VIP_ENABLE), y)
amp_core-objs += drv_vip.o
endif
amp_core-objs += drv_tsp.o

HEADER_PATH +=  \
    -I$(srctree)/drivers/soc/berlin/modules/amp/inc \
    -I$(FIRMWARE_INC_DIR_LOCAL)  \
    $(DHUB_INC_PATH)\
    $(CPM_INC_PATH)

ifeq ($(CONFIG_MV_AMP_COMPONENT_OVP_ENABLE), y)
HEADER_PATH += $(OVP_INC_PATH)
endif

ccflags-y += $(MV_DEFINE) $(HEADER_PATH)

ccflags-y += -DBERLIN_SINGLE_CPU

ifeq ($(CONFIG_FASTLOGO),y)
ccflags-y += -DBERLIN_BOOTLOGO
endif

obj-$(CONFIG_BERLIN_AMP) += amp_core.o

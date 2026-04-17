CONFIG_TC956X_NET ?= m
CONFIG_TC956X_PF ?= y
#CONFIG_TC956X_PCIE_GEN3_SETTING ?= y
CONFIG_TC956X_IOCTL_REG_RD_WR_ENABLE ?= y
CONFIG_TC956X_PLATFORM_SUPPORT ?= y

# Common CFLAGS for all files in this directory
ccflags-y := -DTC956X -Wmissing-prototypes

# Add CFLAGS based on Kconfig options
#ccflags-$(CONFIG_TC956X_PCIE_GEN3_SETTING)   += -DTC956X_PCIE_GEN3_SETTING
ccflags-$(CONFIG_TC956X_IOCTL_REG_RD_WR_ENABLE) += -DTC956X_IOCTL_REG_RD_WR_ENABLE
ccflags-$(CONFIG_TC956X_PLATFORM_SUPPORT) += -DCONFIG_TC956X_PLATFORM_SUPPORT
ccflags-$(CONFIG_TC956X_MAGIC_PACKET_WOL_GPIO) += -DTC956X_MAGIC_PACKET_WOL_GPIO
ccflags-$(CONFIG_TC956X_MAGIC_PACKET_WOL_CONF) += -DTC956X_MAGIC_PACKET_WOL_CONF

ccflags-$(CONFIG_TC956X_PF) += -DTC956X_SRIOV_PF
ccflags-$(CONFIG_TC956X_VF) += -DTC956X_SRIOV_VF

# Define the object files for the PF driver composite module
tc956x_pcie_eth-y := \
	tc956xmac_main.o \
	tc956xmac_ethtool.o \
	tc956xmac_mdio.o \
	mmc_core.o \
	tc956xmac_hwtstamp.o \
	tc956xmac_ptp.o \
	tc956x_xpcs.o \
	tc956x_pma.o \
	hwif.o \
	tc956xmac_tc.o \
	dwxgmac2_core.o \
	dwxgmac2_descs.o \
	dwxgmac2_dma.o \
	tc956x_pci.o \
	tc956x_msigen.o \
	tc956x_pcie_logstat.o \
	tc956x_pf_rsc_mng.o \
	tc956x_pf_mbx_wrapper.o \
	tc956x_pf_mbx.o \
	tc956x_qcom.o \
	tc956x_gpio.o

# Define the object files for the VF driver composite module
tc956x_vf_pcie_eth-y := \
	tc956xmac_main.o \
	tc956xmac_ethtool.o \
	mmc_core.o \
	tc956xmac_hwtstamp.o \
	tc956xmac_ptp.o \
	hwif.o \
	tc956xmac_tc.o \
	dwxgmac2_core.o \
	dwxgmac2_descs.o \
	dwxgmac2_dma.o \
	tc956x_pci.o \
	tc956x_vf_mbx_wrapper.o \
	tc956x_msigen.o \
	tc956x_vf_rsc_mng.o \
	tc956x_vf_mbx.o \
	tc956x_pcie_logstat.o

ifeq ($(CONFIG_TC956X_PF), y)
obj-$(CONFIG_TC956X_NET) += tc956x_pcie_eth.o
endif

ifeq ($(CONFIG_TC956X_VF), y)
obj-$(CONFIG_TC956X_NET) += tc956x_vf_pcie_eth.o
endif

ifndef KERNELRELEASE
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) \
		CONFIG_TC956X_NET=m \
		CONFIG_TC956X_PF=y \
		CONFIG_TC956X_IOCTL_REG_RD_WR_ENABLE=y \
		CONFIG_TC956X_PLATFORM_SUPPORT=y \
		modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
endif

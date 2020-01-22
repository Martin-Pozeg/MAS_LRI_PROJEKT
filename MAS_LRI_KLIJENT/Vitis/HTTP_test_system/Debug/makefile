#
# This file was created by Xilinx Vitis tool.
#


#
# tools
#

RM = rm -f
RMDIR = rm -rf

#
# variables
#

SDCARD_GEN = sdcard_gen
GENERATE_BIF = ::scw::generate_bif

XPFM_PATH = D:/VitisWorkspace/b_d_wrapper/export/b_d_wrapper/b_d_wrapper.xpfm
SYS_CONFIG = b_d_wrapper
DOMAINS = standalone_domain
BIF_PATH = D:/VitisWorkspace/HTTP_test_system/Debug/system.bif

BITSTREAM_PATH = D:/VitisWorkspace/HTTP_test/_ide/bitstream/b_d_wrapper.bit
SD_FILES = --elf D:/VitisWorkspace/HTTP_test/Debug/HTTP_test.elf,ps7_cortexa9_0

#
# primary build targets
#

all: sd_card

sd_card:
	-@echo Generating bif file for the system project
	generate_system_bif.bat 61102 ${XPFM_PATH} ${DOMAINS} ${BIF_PATH}
	${SDCARD_GEN} --xpfm ${XPFM_PATH} --sys_config ${SYS_CONFIG} --bif ${BIF_PATH} --bitstream ${BITSTREAM_PATH} ${SD_FILES}

clean:
	$(RMDIR) ./_sds
	${RMDIR} sd_card
	${RM} system.bif

.PHONY: all sd_card clean



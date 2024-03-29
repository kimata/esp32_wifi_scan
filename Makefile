#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp32_wifi_scan
REACT_DIR    := ./react

include $(IDF_PATH)/make/project.mk

ota: all
ifeq ($(strip $(IP_ADDR)),)
	@echo "\nERROR: Please specify IP_ADDR."
else
	echo -n "\nFirmware: "
	du -h build/$(PROJECT_NAME).bin
	echo ""
	curl $(IP_ADDR)/ota/ --write-out '\nElapsed Time: %{time_total}s (speed: %{speed_upload} bytes/sec)\n' \
		--no-buffer --data-binary @- < build/$(PROJECT_NAME).bin
endif

component-main-build: $(REACT_DIR)/build/index.html

$(REACT_DIR)/build/index.html:
	$(MAKE) -C $(REACT_DIR)

.PHONY: react

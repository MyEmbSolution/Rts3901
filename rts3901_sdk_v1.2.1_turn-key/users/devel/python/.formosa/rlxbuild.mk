#
# Realtek Semiconductor Corp.
#
# tristan_fei (tristan_fei@realsil.com.cn)
# Dec 21, 2015

BUILD = $(shell pwd)/.formosa/build


CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

.PHONY: all romfs clean

all: python

python:
	mkdir -p $(BUILD)
	mkdir -p $(BUILD)/Parser
	cp $(BUILD)/../../Parser/host_pgen $(BUILD)/Parser/
	cd $(BUILD);\
	echo ac_cv_file__dev_ptmx=no > config.site;\
	echo ac_cv_file__dev_ptc=no >> config.site;\
	CONFIG_SITE=config.site ../../configure --host=mips-linux --build=x86_64 --prefix=$(BUILD) --disable-ipv6;\
	$(MAKE);\
	$(MAKE)  -i install
	rm -rf `find  $(BUILD)/lib -name "*.a"`

romfs:
	for x in $(shell cd $(BUILD)/bin; ls *); do \
		if [ -L $(BUILD)/bin/$$x ]; then \
			dest=`readlink $(BUILD)/bin/$$x`; \
			$(ROMFSINST) -s $$dest /bin/$$x; \
		else \
			$(ROMFSINST) $(BUILD)/bin/$$x /bin/$$x; \
		fi; \
	done
	for x in $(shell cd $(BUILD)/lib/python2.7; ls *.py); do \
			$(ROMFSINST) $(BUILD)/lib/python2.7/$$x /lib/python2.7/$$x; \
	done

clean:
	rm -rf $(BUILD)


BUILD = $(shell pwd)/.formosa/build

all:tmpfs

tmpfs:
	$(TMPFSINST) $(BUILD)/include /include
	$(TMPFSINST) $(BUILD)/lib /lib

romfs:
	#$(ROMFSINST) $(BUILD)/bin /usr/bin
	for x in $(shell cd $(BUILD)/lib; ls *.so*); do \
		if [ -L $(BUILD)/lib/$$x ]; then \
			dest=`readlink $(BUILD)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(BUILD)/lib/$$x /lib; \
		fi; \
	done


clean:


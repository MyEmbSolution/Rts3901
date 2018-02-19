define INST_SO
	for x in $(shell cd $1; ls *.so*); do \
		if [ -L $1/$$x ]; then \
			dest=`readlink $1/$$x`; \
			$3 -s $$dest $2/$$x; \
		else \
			$3 $1/$$x $2/$$x; \
		fi; \
	done
endef

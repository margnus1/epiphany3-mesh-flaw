TARGETS=legacy.elf legacy.srec fast.elf fast.srec arm

all: $(TARGETS)

clean:
	rm -rf $(TARGETS)

legacy.elf: epiphany.c *.h
	e-gcc -O2 -o $@ $< -Tlegacy_but_local_stack.ldf \
		-le-lib --std=c11 -g

fast.elf: epiphany.c *.h
	e-gcc -O2 -o $@ $< -T$(EPIPHANY_HOME)/bsps/current/fast.ldf \
		-le-lib --std=c11 -g

%.srec: %.elf
	e-objcopy --srec-forceS3 --output-target srec $< $@

arm: arm.c *.h
	gcc -o $@ $< -L$(EPIPHANY_HOME)/tools/host/lib -le-hal -le-loader \
		-I$(EPIPHANY_HOME)/tools/host/include --std=c11 -O2 -g

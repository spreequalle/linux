ifneq ($(filter y,$(CONFIG_MACH_BERLIN_BG2) $(CONFIG_MACH_BERLIN_BG2CD) $(CONFIG_MACH_BERLIN_BG2Q)),)
obj-y += berlin2-avpll.o berlin2-pll.o berlin2-div.o
else
obj-y += pll.o clk.o gate.o pll-berlin2cdp.o pll-berlin4cdp.o
obj-y += clk-berlin4cdp.o clk-berlin4ct.o clk-berlin4cd.o
endif
obj-$(CONFIG_MACH_BERLIN_BG2)	+= bg2.o
obj-$(CONFIG_MACH_BERLIN_BG2CD)	+= bg2.o
obj-$(CONFIG_MACH_BERLIN_BG2Q)	+= bg2q.o

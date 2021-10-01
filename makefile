PACKAGE    ?= $(NAME)
URL        ?= github.com/jcog/fp
CC          = mips64-gcc
LD          = mips64-g++
AS          = mips64-gcc -x assembler-with-cpp
OBJCOPY     = mips64-objcopy
ARMIPS      = armips
GRU         = gru
GRC         = grc
GENHOOKS    = CPPFLAGS='$(subst ','\'',$(CPPFLAGS))' ./genhooks
RESDESC     = $(RESDIR)/resources.json
LUAFILE     = crc.lua
SRCDIR      = src
OBJDIR      = obj
BINDIR      = bin
LIBDIR      = lib
RESDIR      = res
CFILES      = *.c
SFILES      = *.s
FP_VERSIONS = PM64J PM64U
NAME        = fp
NDEBUG     ?= 0

ADDRESS     = 0x80400060
ADDRESS_LDR	= 0x8004AA2C
CFLAGS      = -c -MMD -MP -std=gnu11 -Wall -ffunction-sections -fdata-sections -O1 -fno-reorder-blocks 
CPPFLAGS    = -DPACKAGE=$(PACKAGE) -DURL=$(URL) -DF3DEX_GBI_2
ALL_LDFLAGS = -T gl-n64.ld -L$(LIBDIR) -nostartfiles -specs=nosys.specs -Wl,--gc-sections $(LDFLAGS)
ALL_LIBS    = $(LIBS)

ifeq ($(NDEBUG),1)
  CFLAGS += -DNDEBUG
  CPPFLAGS += -DNDEBUG
endif

FP          = $(foreach v,$(FP_VERSIONS),fp-$(v))
LDR         = $(foreach v,$(FP_VERSIONS),ldr-fp-$(v))

ELF-FP		= $(ELF-patch-fp-PM64J) $(ELF-fp-PM64U)
FP-ALL      = $(FP-PM64J) $(FP-PM64U)
FP-PM64J    = $(OBJ-fp-PM64J) $(ELF-fp-PM64J) $(HOOKS-fp-PM64J)
FP-PM64U    = $(OBJ-fp-PM64U) $(ELF-fp-PM64U) $(HOOKS-fp-PM64U)

all         : $(FP)
us          : $(FP-PM64U)
jp          : $(FP-PM64J)
clean       :
	rm -rf $(OBJDIR) $(BINDIR)
crc         :
	@find ./rom/ -name "fp-*.z64" -type f -printf "$(GRU) $(LUAFILE) %f\n" -exec $(GRU) $(LUAFILE) {} \;

.PHONY: clean all crc us jp

define bin_template
SRCDIR-$(1)      = $(5)
RESDIR-$(1)      = $(6)
OBJDIR-$(1)      = obj/$(2)
BINDIR-$(1)      = bin/$(2)
HOOKSDIR-$(1)    = patch/$(2)
NAME-$(1)        = $(1)
ADDRESS-$(1)     = $(4)
CPPFLAGS-$(1)    = -DPM64_VERSION=$(2) $(CPPFLAGS)
CSRC-$(1)       := $$(foreach s,$$(CFILES),$$(wildcard $$(SRCDIR-$(1))/$$(s)))
COBJ-$(1)        = $$(patsubst $$(SRCDIR-$(1))/%,$$(OBJDIR-$(1))/%.o,$$(CSRC-$(1)))
SSRC-$(1)       := $$(foreach s,$$(SFILES),$$(wildcard $$(SRCDIR-$(1))/$$(s)))
SOBJ-$(1)        = $$(patsubst $$(SRCDIR-$(1))/%,$$(OBJDIR-$(1))/%.o,$$(SSRC-$(1)))
RESSRC-$(1)     := $$(wildcard $$(RESDIR-$(1))/*)
RESOBJ-$(1)      = $$(patsubst $$(RESDIR-$(1))/%,$$(OBJDIR-$(1))/$$(RESDIR)/%.o,$$(RESSRC-$(1)))
ELF-$(1)         = $$(BINDIR-$(1))/$(3).elf
BIN-$(1)         = $$(BINDIR-$(1))/$(3).bin
OUTDIR-$(1)      = $$(OBJDIR-$(1)) $$(OBJDIR-$(1))/$$(RESDIR) $$(BINDIR-$(1)) $$(HOOKSDIR-$(1))
HOOKS-$(1)       = patch/$(2)/hooks.gsc
BUILD-$(1)       = $(1)
CLEAN-$(1)       = clean-$(1)
$$(ELF-$(1))     : ALL_LDFLAGS += -Wl,--defsym,start=$$(ADDRESS-$(1))
$$(BUILD-$(1))   : $$(BIN-$(1))
$$(CLEAN-$(1))   : rm -rf $$(OUTDIR-$(1))

$$(COBJ-$(1))     : $$(OBJDIR-$(1))/%.o: $$(SRCDIR-$(1))/% | $$(OBJDIR-$(1))
	$(CC) $$(CPPFLAGS-$(1)) $$(CFLAGS) $$< -o $$@
$$(SOBJ-$(1))     : $$(OBJDIR-$(1))/%.o: $$(SRCDIR-$(1))/% | $$(OBJDIR-$(1))
	$(AS) -c -MMD -MP -DF3DEX_GBI_2 $$< -o $$@
$$(ELF-$(1))      : $$(COBJ-$(1)) $$(SOBJ-$(1)) $$(RESOBJ-$(1)) | $$(BINDIR-$(1))
	$(LD) $$(ALL_LDFLAGS) $$^ $$(ALL_LIBS) -o $$@
$$(BIN-$(1))      : $$(ELF-$(1)) | $$(BINDIR-$(1))
	$(OBJCOPY) -S -O binary $$< $$@
$$(RESOBJ-$(1))   : $$(OBJDIR-$(1))/$$(RESDIR)/%.o: $$(RESDIR-$(1))/% $$(RESDESC) | $$(OBJDIR-$(1))/$$(RESDIR)
	$$(GRC) $$< -d $$(RESDESC) -o $$@
$$(HOOKS-$(1))    :   $$(ELF-$(1)) $$(HOOKSDIR-$(2))
	$$(GENHOOKS) $$< >$$@
$$(OUTDIR-$(1))   : 
	mkdir -p $$@
endef

$(foreach v,$(FP_VERSIONS),$(eval $(call bin_template,fp-$(v),$(v),fp,$(ADDRESS),src,res/fp)))
#$(foreach v,$(FP_VERSIONS),$(eval $(call bin_template,ldr-fp-$(v),$(v),ldr,$(ADDRESS_LDR),ldr,ldr/res)))

$(FP-PM64U)	:	LDFLAGS	+=	-Wl,-Map=bin/PM64U/fp-u.map
$(FP-PM64J)	:	LDFLAGS	+=	-Wl,-Map=bin/PM64J/fp-j.map

$(FP-PM64U)	:	LIBS	:=	-lpm-us
$(FP-PM64J)	:	LIBS	:=	-lpm-jp

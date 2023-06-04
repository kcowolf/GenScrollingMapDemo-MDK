# Project meta configuration
PROJECT_NAME := rom
MDKROOT := mdk/mdk

# Project directories.
SRCDIR := src
RESDIR := res
OBJDIR := obj
OUTDIR := out

# Sources.
SOURCES_C= $(wildcard $(SRCDIR)/*.c)
SOURCES_C+= $(wildcard $(SRCDIR)/*/*.c)
SOURCES_C+= $(wildcard $(SRCDIR)/*/*/*.c)

SOURCES_ASM= $(wildcard $(SRCDIR)/*.s)
SOURCES_ASM+= $(wildcard $(SRCDIR)/*/*.s)
SOURCES_ASM+= $(wildcard $(SRCDIR)/*/*/*.s)

# Target configuration.
# TARGET_SYSTEM:
#   * MDK_TARGET_MD  ; Default. Use this for megadrive
#   * MDK_TARGET_C2  ; Use this to target the Sega System C/C2 PCB.
# In addition, you may define MDK_C2_ADPCM with the filename of the ADPCM ROM.
TARGET_SYSTEM := MDK_TARGET_MD

# Any additional CFLAGS/CPPFLAGS/ASFLAGS additions or build targets may go here.
# Please read about EXTERNAL_DEPS and EXTERNAL_ARTIFACTS in readme.md.

include $(MDKROOT)/md-rules.mk

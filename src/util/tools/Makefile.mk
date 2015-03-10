C_FILE += $(SROUCE)/util/tools/dos_endian.c \
			$(SROUCE)/util/tools/dos_string.c \
			$(SROUCE)/util/tools/dos_vargs.c \
			$(SROUCE)/util/tools/dos_py.c

C_OBJ_FILE += dos_endian.$(SUFFIX) \
				dos_string.$(SUFFIX) \
				dos_vargs.$(SUFFIX) \
				dos_py.$(SUFFIX)

dos_endian.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/tools/dos_endian.c
	
dos_string.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/tools/dos_string.c

dos_vargs.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/tools/dos_vargs.c
	
dos_py.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/util/tools/dos_py.c
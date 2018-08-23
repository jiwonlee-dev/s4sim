


lib			:=libdrivers.a
OBJS 		:=kernel.o


all : $(lib)

 
$(lib):  $(OBJS) ; 
	@echo  "  AR: $@"
	@$(AR) -r $@ $(OBJS)


kernel.o : kernel.s hw_drivers.h ;
	@echo  "   $@"
	@arm-none-eabi-as --warn -EL kernel.s -o $@   


clean :
	rm -f $(OBJS) $(lib)






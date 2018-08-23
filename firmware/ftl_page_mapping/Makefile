

lib				:=$(BUILD_FOLDER)/$(config)_ftl.a
objs 			:=ftl 
OBJS 			=$(map_tables) 
OBJS 			+=$(foreach OBJ,$(objs), ${BUILD_FOLDER}/$(config)_$(OBJ).o)
map_tables  	:= ${BUILD_FOLDER}/$(config)_map_tables.o


all : $(lib)

 
$(lib):  $(OBJS)  ; 
	@$(AR) -r $@ $(OBJS)  


$(BUILD_FOLDER)/$(config)_%.o : %.c    ;
	@echo "   $(notdir $@)"
	@$(CC) -g -c $(CFLAGS) $(CONFIG_FLAGS) $(VERBOSE_FLAGS) $< -o $@ 

$(map_tables):
	@echo  "   Generate mapping tables "
	@g++ $(CONFIG_FLAGS) gen_mapping_tables.cpp -o $(basename $@)_generator  
	@$(basename $@)_generator page_mapping $(basename $@).c
	@$(CC) -c $(CFLAGS) $(CONFIG_FLAGS) -include ftl.h $(basename $@).c  -o $@


clean :
	@echo "rm -f ftl.o ftl.a"
	@rm -f $(OBJS) $(lib)






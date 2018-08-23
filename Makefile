
include Makefile.nandchip.config
include Makefile.storage.config
include Makefile.firmware.config

# select configuration functions
get_config   =$(strip $(filter-out  --$(1),$(subst =, , $(filter  --$(1)=%,$(all_config)))))

CROSS_COMPILE_DIR =
ARCH          	?= arm
CROSS_COMPILE 	?= arm-none-eabi-
AS           	= $(CROSS_COMPILE_DIR)$(CROSS_COMPILE)as 
LD           	= $(CROSS_COMPILE_DIR)$(CROSS_COMPILE)ld
CC           	= $(CROSS_COMPILE_DIR)$(CROSS_COMPILE)gcc 
CXX          	= $(CROSS_COMPILE_DIR)$(CROSS_COMPILE)g++
CPP          	= $(CROSS_COMPILE_DIR)$(CROSS_COMPILE)cpp
AR           	= $(CROSS_COMPILE_DIR)$(CROSS_COMPILE)ar
NM           	= $(CROSS_COMPILE_DIR)$(CROSS_COMPILE)nm
STRIP        	= $(CROSS_COMPILE_DIR)$(CROSS_COMPILE)strip
OBJCOPY      	= $(CROSS_COMPILE_DIR)$(CROSS_COMPILE)objcopy
OBJDUMP      	= $(CROSS_COMPILE_DIR)$(CROSS_COMPILE)objdump
CROSS_FLAGS  	= -target arm-none-eabi -mfloat-abi=soft -I/usr/lib/gcc/arm-none-eabi/4.9.3/include -I/usr/lib/gcc/arm-none-eabi/4.9.3/include-fixed -I/usr/lib/gcc/arm-none-eabi/4.9.3/../../../arm-none-eabi/include -fshort-enums
CFLAGS 			= -I . -I$(SRC_FOLDER)/../include -I $(hw_driver_path) -include $(SRC_FOLDER)/global.h -std=gnu11 -Wall -Werror
CXXFLAGS     	= -lunistd -I . -fno-exceptions -std=gnu11 -Wall
ASFLAGS      	= -EL 


BUILD_FOLDER 	:=$(PWD)/build
SRC_FOLDER	 	:=$(PWD)/firmware

# select a default configuration if not speicfied in make command
ifeq ($(config),) 
    config :=H4k_N4k_256mb
endif
all_config   	:=$($(config)) 
CONFIG_FLAGS 	:=$(subst --,-D, $(all_config)) $(VERBOSE_EXECUTION_FLAGS)
buff_manager 	:=$(call get_config ,write_back_cache_mgt)
ftl 		 	:=$(call get_config ,mapping_scheme)
hardware 		:=$(call get_config ,hardware)

hw_driver_path	:=$(SRC_FOLDER)/hw_$(hardware)
hw_driver_lib	:=$(hw_driver_path)/libdrivers.a
ftl_module_path :=$(SRC_FOLDER)/ftl_$(ftl)
ftl_module_lib 	:=$(BUILD_FOLDER)/$(config)_ftl.a 
firmware	 	:=$(BUILD_FOLDER)/$(config)_firmware.elf 
kernel_objs	 	:= issd_kernel issd_kernel_mem_alloc 
util_objs	 	:= util.memcp util.uart_print util.containers
base_objs 	 	:= nvme_driver host_dma buffer_mgt_$(buff_manager) low_level_nand_io isp
OBJS 		 	=
OBJS 			+=$(foreach OBJ,$(kernel_objs), ${BUILD_FOLDER}/$(config)_$(OBJ).o)
OBJS 		 	+=$(foreach OBJ,$(util_objs), ${BUILD_FOLDER}/$(config)_$(OBJ).o)
OBJS 		 	+=$(foreach OBJ,$(base_objs), ${BUILD_FOLDER}/$(config)_$(OBJ).o)
linker_script 	:=$(SRC_FOLDER)/linkerS.ld


all: show_config directory ftl_module_lib $(firmware)

show_config:
	@#echo $(config): { $(all_config) }  
	

.PHONY: directory
directory: ${BUILD_FOLDER}
${BUILD_FOLDER}  :
	mkdir -p ${BUILD_FOLDER}


$(firmware): $(hw_driver_lib) $(ftl_module_lib) $(OBJS) 
	@echo  "  LD: $@"
	@$(CC) -nostartfiles -Xlinker -T$(linker_script) $(map_tables) $(OBJS) $(hw_driver_lib) $(ftl_module_lib) -o $@

$(hw_driver_lib) :
	@cd $(hw_driver_path) ; make 

ftl_module_lib :	
	@cd $(ftl_module_path) ; \
	make CC=$(CC) CFLAGS="$(CFLAGS)" AR=$(AR) \
	BUILD_FOLDER="$(BUILD_FOLDER)" SRC_FOLDER="$(SRC_FOLDER)" \
	config=$(config) CONFIG_FLAGS="$(CONFIG_FLAGS)" VERBOSE_FLAGS="$(VERBOSE_FLAGS)" 

 
${BUILD_FOLDER}/$(config)_%.o : $(SRC_FOLDER)/%.c   ;
	@echo "   $(notdir $@)"
	@$(CC) -g -c $(CFLAGS) $(CONFIG_FLAGS) $(VERBOSE_FLAGS) $< -o $@ 


clean: 
	rm -f -r $(BUILD_FOLDER)/$(config)_*
 
cleanall : clean ;
	@cd $(hw_driver_path) ; make clean 


 
# running the simulation

ifeq ($(cpu),o3)
    cpu_mode:=--cpu-type=arm_detailed
else
	cpu_mode:=--cpu-type=timing
endif

cpu_cores		:=--num-cpus=4
cache_config	:=--caches --l2cache --l1d_size=32kB --l1i_size=32kB --l1d_assoc=4  --l1i_assoc=4 --l2_size=512kB --l2_assoc=8 --cacheline_size=64 
dram_config		:=--mem-size=512MB
run_cmd 		:=SSD_hw_platforms/gem5/gem5-stable_2015_09_03/build/ARM/gem5.fast --outdir=m5out SSD_hw_platforms/gem5/issd_fs_mcore.py $(cpu_cores) $(cpu_mode) $(cache_config) $(dram_config)
gem5_config 	:= $(filter  --nand_%,$(all_config))
HOST_FILE_PID 	:=simulators/qemu_pid

run : directory  $(BootLoader) $(firmware) ;
	#rm -f ${HOST_FILE_PID} ; sync 
	#export HOST_FILE_PID=${HOST_FILE_PID} ; ./starthost.sh 
	export M5_PATH="~/share/tom/isp_issd/simulators/gem5_dep" ; $(run_cmd) --kernel=$(firmware) --issd_bootloader=SSD_hw_platforms/gem5/bootloader.mp_cpu $(gem5_config)
	#./stophost.sh 

	

 
	


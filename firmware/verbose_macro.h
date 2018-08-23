

#ifndef VERBOSE_MACRO
#define VERBOSE_MACRO

#define DPRINTF_MORE(args...)
#define VPRINTF_NM(args...)
#define VPRINTF_NM_WL(args...)
#define VPRINTF_CONFIGQ(args...)
#define DPRINTF_LESS(args...)
#define VPRINTF(args...)	snprintf_(get_buff() , printbuff_size , args ) ; queue_print () ;

//#define VPRINTF_LINE VPRINTF(" **** %s:%u **** " , __FUNCTION__ , __LINE__ ) ;
#define VPRINTF_LINE 
#define VPRINTF_ERROR_LINE(error_msg) VPRINTF(" *** Error @ {%s:%u} Msg:%s *** " , __FUNCTION__ , __LINE__ , error_msg ) ;

#ifdef VERBOSE_ERROR
#define VPRINTF_ERROR VPRINTF
#else
#define VPRINTF_ERROR(args...)
#endif

#ifdef VERBOSE_STARTUP_SHUTDOWN
#define VPRINTF_CONFIG VPRINTF
#define VPRINTF_STARTUP VPRINTF
#define VPRINTF_SHUTDOWN VPRINTF
#else
#define VPRINTF_CONFIG(args...)
#define VPRINTF_STARTUP(args...)
#define VPRINTF_SHUTDOWN(args...)
#endif

#ifdef VERBOSE_CPU_SCHEDULE
#define VPRINTF_CPU_SCHEDULE VPRINTF
#else
#define VPRINTF_CPU_SCHEDULE(args...)
#endif


#ifdef VERBOSE_NVME_ADMIN
#define VPRINTF_NVME_ADMIN VPRINTF
#else
#define VPRINTF_NVME_ADMIN(args...)
#endif

#ifdef VERBOSE_NVME_DOOR_BELL
#define VPRINTF_NVME_DB VPRINTF
#else
#define VPRINTF_NVME_DB(args...)
#endif

#ifdef VERBOSE_NVME_IO
#define VPRINTF_NVME_IO VPRINTF
#else
#define VPRINTF_NVME_IO(args...)
#endif

#ifdef VERBOSE_NVME_IO_DETAILED
#define VPRINTF_NVME_IO_DETAILED VPRINTF
#else
#define VPRINTF_NVME_IO_DETAILED(args...)
#endif

#ifdef VERBOSE_PAGE_BUFFER_MGT
#define VPRINTF_BUFF_MGT VPRINTF
#else
#define VPRINTF_BUFF_MGT(args...)
#endif

#ifdef VERBOSE_FTL
#define VPRINTF_FTL VPRINTF
#else
#define VPRINTF_FTL(args...)
#endif

#ifdef VERBOSE_FTL_DETAILED
#define VPRINTF_FTL_DETAILED VPRINTF
#else
#define VPRINTF_FTL_DETAILED(args...)
#endif

#ifdef VERBOSE_TRIM
#define VPRINTF_TRIM VPRINTF
#else
#define VPRINTF_TRIM(args...)
#endif

#ifdef VERBOSE_GC
#define VPRINTF_GC VPRINTF
#else
#define VPRINTF_GC(args...)
#endif

#ifdef VERBOSE_WEAR_LEVELING
#define VPRINTF_WEAR_LEVELING VPRINTF
#else
#define VPRINTF_WEAR_LEVELING(args...)
#endif

#ifdef VERBOSE_HOST_DMA
#define VPRINTF_HOST_DMA VPRINTF
#else
#define VPRINTF_HOST_DMA(args...)
#endif

#ifdef VERBOSE_HOST_DMA_DETAILED
#define VPRINTF_HOST_DMA_DETAILED VPRINTF
#else
#define VPRINTF_HOST_DMA_DETAILED(args...)
#endif

#ifdef VERBOSE_NAND_IO
#define VPRINTF_NAND_IO VPRINTF
#else
#define VPRINTF_NAND_IO(args...)
#endif

#ifdef VERBOSE_NAND_IO_DETAILED
#define VPRINTF_NAND_IO_DETAILED VPRINTF
#else
#define VPRINTF_NAND_IO_DETAILED(args...)
#endif


#ifdef VERBOSE_ISP_BIN_DOWNLOAD
#define VPRINTF_ISP_BIN_DOWNLOAD VPRINTF
#else
#define VPRINTF_ISP_BIN_DOWNLOAD(args...)
#endif

#ifdef VERBOSE_ISP_IO
#define VPRINTF_ISP_IO VPRINTF
#else
#define VPRINTF_ISP_IO(args...)
#endif

#ifdef VERBOSE_ISP_IO_DETAILED
#define VPRINTF_ISP_IO_DETAILED VPRINTF
#else
#define VPRINTF_ISP_IO_DETAILED(args...)
#endif


#endif





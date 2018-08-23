 
#include <hw_drivers.h>
#include <stdarg.h>

volatile u32 *SimEventLog = (u32*)0x2D419000 ;


typedef struct printing_buffer{
	int			state;
	uint64_t	time_stamp ;
	char		buffer [0x10000]; 
}printing_buffer;

char  *cpu_string[4]			= {	"CORE 0","CORE 1","CORE 2","CORE 3"};
char  *io_opcode_string[11]	= { "FLUSH IO" , "WRITE IO" , "READ IO" ,"-","-","-","-","-","-","DSM","Not Set"};

printing_buffer printbuff[4];

//DEFINE_REQ_STRINGS ;
//DEFINE_REQ_STAGES_STRINGS ;

#define DEV_IS_FREE 0xf
unsigned int *print_request;


char * request_type_string[32];
char *request_stage_string[32]; 
char not_set_string[16] = "Not Set" ;

void init_printing(){
	
	print_request  = (unsigned int *)get_uncacheable_ram(sizeof(unsigned int) * 4);
	*print_request = DEV_IS_FREE ;

	for(uint32_t i = 0 ; i < 32 ; i++){
		request_type_string[i] = not_set_string;
		request_stage_string[i]= not_set_string;
	}

	request_type_string[ADMIN_REQUEST ] = "ADMIN_REQUEST";
	request_type_string[READ_IO_REQUEST] = "READ_IO_REQUEST";
	request_type_string[WRITE_IO_REQUEST ] = "WRITE_IO_REQUEST";
	request_type_string[FLUSH_IO_REQUEST ] = "FLUSH_IO_REQUEST";
	request_type_string[TRIM_IO_REQUEST ] = "TRIM_IO_REQUEST";
	request_type_string[ISP_IO_REQUEST ] = "ISP_IO_REQUEST";
	request_type_string[WBC_FLUSH_REQUEST ] = "WBC_FLUSH_REQUEST";
	request_type_string[ISP_READ_PAGES_REQUEST ] = "ISP_READ_PAGES_REQUEST";
	request_type_string[ISP_PROGRAM_PAGES_REQUEST ] = "ISP_PROGRAM_PAGES_REQUEST";
	request_type_string[ISP_RESERVE_PAGES_REQUEST ] = "ISP_RESERVE_PAGES_REQUEST";
	request_type_string[ISP_HOST_DMA_REQUEST ] = "ISP_HOST_DMA_REQUEST";

	request_stage_string[REQ_STAGE_FREE ] = "REQ_STAGE_FREE" ;
	request_stage_string[REQ_STAGE_INUSE ] =  "REQ_STAGE_INUSE";
	request_stage_string[REQ_STAGE_FTL ] =  "REQ_STAGE_FTL";
	request_stage_string[REQ_STAGE_READING_FLASH ] = "REQ_STAGE_READING_FLASH" ;
	request_stage_string[REQ_STAGE_PROGRAMMING_FLASH ] = "REQ_STAGE_PROGRAMMING_FLASH" ;
	request_stage_string[REQ_STAGE_DMA ] = "REQ_STAGE_DMA" ;
	request_stage_string[PENDING_CCMD_POST ] = "PENDING_CCMD_POST" ;
	request_stage_string[REQ_COMPLETED ] = "REQ_COMPLETED" ;

	request_stage_string[ISP_DATA_TRANSFERING] = "ISP_DATA_TRANSFERING" ;
	request_stage_string[ISP_CTRL_PENDING_MSG] = "ISP_CTRL_PENDING_MSG";
	request_stage_string[ISP_EXEC_PENDING_POST] = "ISP_EXEC_PENDING_POST";

	return ;
}

char * get_buff(){
	unsigned int cpu = get_cpu_id();
	printing_buffer * buf = &printbuff[cpu];
	return buf->buffer ;
}

void _sync_print_(char * string_buffer){
	unsigned int cpu = get_cpu_id();
	int loop_count = 0 ;
	
_keep_waiting_:
	
	for(; loop_count < 20000 ; loop_count ++){
		
		if (*print_request == DEV_IS_FREE ){
			
			// try locking the device
			*print_request = cpu ;
			
			// ensure it is locked 
			for(int loop_count_2 = 0 ; loop_count_2 < 10 ; loop_count_2 ++){
				
				if (*print_request != cpu ){
					// some other core has re-locked the device , start over
					goto _keep_waiting_;	
				}
			}
			
			// this core still has access
			goto _use_device_ ;
			
		}else{
			// some other core is printing , keep waiting
		}
	}
	
	// device is busy for too long , abort the printing
	printToDev("\n ***ABORT*** \n");
	return ;
	
_use_device_: // now use the print device
	
	//buf = & printbuff[cpu];
	printToDev ( "\n ");
	printToDev ( cpu_string[cpu] );
	printToDev ( " : ");
	printToDev( string_buffer ) ;
	
	// release the print device 
	*print_request = DEV_IS_FREE ;
	return ;
}

void queue_print(){
	unsigned int cpu = get_cpu_id();
	_sync_print_( printbuff[cpu].buffer ) ;
}
 

static void simple_outputchar(char **str, char c)
{
	if (str) {
		**str = c;
		++(*str);
	} 
}

enum flags {
	PAD_ZERO	= 1,
	PAD_RIGHT	= 2,
};

static int prints(char **out, const char *string, int width, int flags)
{
	int pc = 0, padchar = ' ';
	
	if (width > 0) {
		int len = 0;
		const char *ptr;
		for (ptr = string; *ptr; ++ptr) ++len;
		if (len >= width) width = 0;
		else width -= len;
		if (flags & PAD_ZERO)
		padchar = '0';
	}
	if (!(flags & PAD_RIGHT)) {
		for ( ; width > 0; --width) {
			simple_outputchar(out, padchar);
			++pc;
		}
	}
	for ( ; *string ; ++string) {
		simple_outputchar(out, *string);
		++pc;
	}
	for ( ; width > 0; --width) {
		simple_outputchar(out, padchar);
		++pc;
	}
	
	return pc;
}


#define PRINT_BUF_LEN 64
static int simple_outputi(char **out, int i, int base, int sign, int width, int flags, int letbase)
{
	char print_buf[PRINT_BUF_LEN];
	char *s;
	int t, neg = 0, pc = 0;
	unsigned int u = i;
	
	if (i == 0) {
		print_buf[0] = '0';
		print_buf[1] = '\0';
		return prints(out, print_buf, width, flags);
	}
	
	if (sign && base == 10 && i < 0) {
		neg = 1;
		u = -i;
	}
	
	s = print_buf + PRINT_BUF_LEN-1;
	*s = '\0';
	
	while (u) {
		t = u % base;
		if( t >= 10 )
		t += letbase - '0' - 10;
		*--s = t + '0';
		u /= base;
	}
	
	if (neg) {
		if( width && (flags & PAD_ZERO) ) {
			simple_outputchar (out, '-');
			++pc;
			--width;
		}
		else {
			*--s = '-';
		}
	}
	
	return pc + prints (out, s, width, flags);
}

static int simple_vsprintf(char **out, char *format, va_list ap)
{
	int width, flags;
	int pc = 0;
	char scr[2];
	union {
		char c;
		char *s;
		int i;
		unsigned int u;
		void *p;
	} u;
	
	for (; *format != 0; ++format) {
		if (*format == '%') {
			++format;
			width = flags = 0;
			if (*format == '\0')
			break;
			if (*format == '%')
			goto out;
			if (*format == '-') {
				++format;
				flags = PAD_RIGHT;
			}
			while (*format == '0') {
				++format;
				flags |= PAD_ZERO;
			}
			if (*format == '*') {
				width = va_arg(ap, int);
				format++;
			} else {
				for ( ; *format >= '0' && *format <= '9'; ++format) {
					width *= 10;
					width += *format - '0';
				}
			}
			switch (*format) {
					case('d'):
					u.i = va_arg(ap, int);
					pc += simple_outputi(out, u.i, 10, 1, width, flags, 'a');
					break;
					
					case('u'):
					u.u = va_arg(ap, unsigned int);
					pc += simple_outputi(out, u.u, 10, 0, width, flags, 'a');
					break;
					
					case('x'):
					u.u = va_arg(ap, unsigned int);
					pc += simple_outputi(out, u.u, 16, 0, width, flags, 'a');
					break;
					
					case('X'):
					u.u = va_arg(ap, unsigned int);
					pc += simple_outputi(out, u.u, 16, 0, width, flags, 'A');
					break;
					
					case('c'):
					u.c = va_arg(ap, int);
					scr[0] = u.c;
					scr[1] = '\0';
					pc += prints(out, scr, width, flags);
					break;
					
					case('s'):
					u.s = va_arg(ap, char *);
					pc += prints(out, u.s ? u.s : "(null)", width, flags);
					break;
				default:
					break;
			}
		}
		else {
		out:
			simple_outputchar (out, *format);
			++pc;
		}
	}
	if (out) **out = '\0';
	return pc;
}

int simple_snprintf(char *buf, size_t len , char *fmt, ...)
{
	va_list ap;
	int r;
	va_start(ap, fmt);
	r = simple_vsprintf(&buf, fmt, ap);
	va_end(ap);
	return r;
}

char *simple_strcat(char *dest, const char *src)
{
	while (*dest != '\0') dest++;
	while ((*dest++ = *src++) != '\0');
	return --dest;
}
 
int _strnlen(const char *s, int max_len)
{
	int i;	
	for(i = 0; i < max_len; i++) {
		if (s[i] == '\0') {
			break;
		}
	}
	return i;
}

void strpadcpy(char *buf, int buf_size, const char *str, char pad)
{
	int len = _strnlen(str, buf_size);
	memcpy8((uint8_t*)buf,(uint8_t*) str, len);
	memset8((uint8_t*)buf + len, pad, buf_size - len);
}



  






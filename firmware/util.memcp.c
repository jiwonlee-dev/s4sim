 

void memset32(uint32_t * ptr  , uint32_t value , size_t count ){
	for (size_t ix = 0 ; ix < count ; ix++ ){
		ptr[ix] = value ;
	}
}

void memset8(uint8_t * ptr  , uint8_t value , size_t count ){
	for (size_t ix = 0 ; ix < count ; ix++ ){
		ptr[ix] = value ;
	}
}
 
void memcpy_sysbitw(unsigned char* dst, unsigned char* src, size_t count ){
	sysPtr * dst_ptr = (sysPtr*)dst;
	sysPtr * src_ptr = (sysPtr*)src ;
	for (size_t ix = 0 ; ix < count ; ix++ ){
		dst_ptr[ix] = src_ptr[ix] ;
	}
}

void memcpy8(uint8_t * dst, uint8_t * src, size_t count ){
	for (size_t ix = 0 ; ix < count ; ix++ ){
		dst[ix] = src[ix] ;
	}
}

  

  






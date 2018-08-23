 

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/shm.h>
#include <string>

using namespace std ;

#include <stdint.h>
#include <stdio.h>

#define NUM_CHANNELS			nand_num_chn 
#define PKG_PER_CHANNEL			nand_pkgs_per_chn	
#define DIES_PER_PKG			nand_dies_per_pkg 
#define PLANES_PER_DIE 			nand_planes_per_die	
#define BLOCKS_PER_PLANE		nand_blocks_per_plane 
#define PAGES_PER_BLOCK 		nand_pages_per_block	
#define NAND_DS 				nand_page_size 
#define NAND_PAGE_SIZE 			(1<<nand_page_size)
#define DS 						host_page_size  
#define HOST_PAGE_SIZE 			(1<<host_page_size)
#define NUM_NAMESPACES 			num_namespaces 
#define OVP 					over_provision_factor 

#define TOTAL_NUM_OF_PKGS		( NUM_CHANNELS * PKG_PER_CHANNEL )
#define TOTAL_NUM_OF_DIES		( TOTAL_NUM_OF_PKGS * DIES_PER_PKG )
#define TOTAL_NUM_OF_PLANES		( TOTAL_NUM_OF_DIES * PLANES_PER_DIE )
#define TOTAL_NUM_OF_BLOCK		( TOTAL_NUM_OF_PLANES * BLOCKS_PER_PLANE )
#define TOTAL_NUM_OF_PAGES		( TOTAL_NUM_OF_BLOCK * PAGES_PER_BLOCK )
#define TOTAL_FLASH_SIZE		( TOTAL_NUM_OF_PAGES * NAND_PAGE_SIZE )


int main( int argc , char ** argv) {
	
	FILE * out_file = fopen(argv[2] , "w");
	
	std::string array_to_make(argv[1]);
	
	int DIES_PER_CHANNEL = DIES_PER_PKG * PKG_PER_CHANNEL;
	 
	if (array_to_make == "page_mapping"){
		
		//printf ("\n generating ppa_table to %s \n" , argv[2] );
		
		fprintf(out_file , "\n\n");
		fprintf(out_file , "__attribute__ ((section(\"mapping_tables\")))  ppa_t lpa_table[TOTAL_NUM_OF_PAGES] = {");
		
		int lpa = 0;
		int cr_count_down = 50 ;
		//fprintf (out_file , "\n\t{");
		fprintf (out_file , "\n\t\t");
		while (1) {
			
			uint16_t	page ;
			uint16_t	block ;
			uint8_t		plane;
			uint8_t		die ;
			uint8_t		chn ;
			uint8_t		resv;
			
			fprintf (out_file , "{0x00FF000000000000}");
			lpa++;
			if ( lpa >= TOTAL_NUM_OF_PAGES )break;
			fprintf (out_file , ",");
			
			cr_count_down--;
			if(!cr_count_down){
				cr_count_down = 50 ;	
				fprintf (out_file , "\n\t\t");
			}
		}
		fprintf (out_file , "\n};");
		
		
		
		fprintf(out_file , "\n\n");
		fprintf(out_file , "__attribute__ ((section(\"mapping_tables\")))  lpn_t ppa_table[NUM_CHANNELS][DIES_PER_CHANNEL][PLANES_PER_DIE][BLOCKS_PER_PLANE][PAGES_PER_BLOCK] = {");
		
		int chn = 0;
		while (1) {
			fprintf (out_file , "\n\n\t{// CHN %d " , chn );
			
			
			int die = 0;
			while (1) {
				fprintf (out_file , "\n\t\t{// DIE %d " , die );
				
				
				int plane = 0;
				while (1) {				
					fprintf (out_file , "\n\t\t\t{// plane %d" , plane );
					
					
					int block = 0 ;
					while(1){	
					
						int page = 0;
						fprintf (out_file , "\n\t\t\t\t{");
						while (1) {
							fprintf (out_file , "FREE_PPA");
							page++;
							if ( page >= PAGES_PER_BLOCK )break;
							fprintf (out_file , ",");
							
						}
						
						
						fprintf (out_file , "}");
						block++;
						if ( block >= BLOCKS_PER_PLANE )break;
						fprintf (out_file , ",");
					}
					
					
					fprintf (out_file , "\n\t\t\t}");
					plane++;
					if (plane >= PLANES_PER_DIE )break;
					fprintf (out_file , ",");
				}
				
				
				fprintf (out_file , "\n\t\t}");
				die++;
				if ( die >= DIES_PER_CHANNEL )break;
				fprintf (out_file , ",");		
			}

			fprintf (out_file , "\n\t}");
			chn++;
			if ( chn >= NUM_CHANNELS )break;
			fprintf (out_file , ",");

		
		}
	
		fprintf (out_file , "};");
		
		
		
	}
	else if (array_to_make == "block_mapping"){
		
		
		fprintf(out_file , "\n\n");
		fprintf(out_file , "__attribute__ ((section(\"mapping_tables\")))  pba_t lba_table[TOTAL_NUM_OF_BLOCK] = {");
		
		int lba = 0;
		int cr_count_down = 50 ;
		//fprintf (out_file , "\n\t{");
		fprintf (out_file , "\n\t\t");
		while (1) {
			
			uint32_t	block ;
			uint8_t		plane;
			uint8_t		die ;
			uint8_t		chn ;
			uint8_t		resv;
			
			fprintf (out_file , "{0x00FF000000000000}");
			lba++;
			if ( lba >= TOTAL_NUM_OF_BLOCK )break;
			fprintf (out_file , ",");
			
			cr_count_down--;
			if(!cr_count_down){
				cr_count_down = 50 ;	
				fprintf (out_file , "\n\t\t");
			}
		}
		fprintf (out_file , "\n};");
		
		
		
		fprintf(out_file , "\n\n");
		fprintf(out_file , "__attribute__ ((section(\"mapping_tables\")))  lba_t pba_table[NUM_CHANNELS][DIES_PER_CHANNEL][PLANES_PER_DIE][BLOCKS_PER_PLANE] = {");
		
		int chn = 0;
		while (1) {
			fprintf (out_file , "\n\n\t{// CHN %d " , chn );
			
			
			int die = 0;
			while (1) {
				fprintf (out_file , "\n\t\t{// DIE %d " , die );
				
				
				int plane = 0;
				while (1) {				
					fprintf (out_file , "\n\t\t\t{// plane %d" , plane );
					
					
					int block = 0 ;
					while(1){	
					
						fprintf (out_file , "FREE_PBA");
						
						block++;
						if ( block >= BLOCKS_PER_PLANE )break;
						fprintf (out_file , ",");
					}
					
					
					fprintf (out_file , "\n\t\t\t}");
					plane++;
					if (plane >= PLANES_PER_DIE )break;
					fprintf (out_file , ",");
				}
				
				
				fprintf (out_file , "\n\t\t}");
				die++;
				if ( die >= DIES_PER_CHANNEL )break;
				fprintf (out_file , ",");		
			}

			fprintf (out_file , "\n\t}");
			chn++;
			if ( chn >= NUM_CHANNELS )break;
			fprintf (out_file , ",");

		
		}
	
		fprintf (out_file , "};");
		
	}
	
	fclose(out_file);
}

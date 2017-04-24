/* IBM_PROLOG_BEGIN_TAG */
/* 
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 		 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

/******************************************************************************
 * PROGRAM_NAME: bufdisp.c                                  
 *                                                                           
 * This program will take two data files and compare them against each other.
 * If a miscompare is found it will mark the place it found the miscompare.
 * After reading all of the data it will then display the data on the screen.
 * It will mark the good lines of the data with an equal sign and will mark
 * the lines that have a miscompare with a # sign. It will also count the   
 * number of bytes in the file that are different. You will be able to scroll
 * through the data as well as go to a specific offset. Also you will be able
 * to display the offset in hexadecimal or decimal format. You will also be
 * able to change the block size of the data. The default blocksize is 512.
 ****************************************************************************/
#include <stdio.h>                         /* standard include declarations */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>     

#define DISP_SIZ 8       /* default info for data to be displayed on screen */
#define DEF_BLOCK_SIZ 512
#define SCREEN_SIZ 20
#define MAX_TEXT_MSG 256
#define OVERHEAD (48/2)
#define BUFSIG "MDHF"

#define LBA        0
#define TS         4
#define DEVICE     8
#define STANZA    16
#define HOST      24
#define MACHINEID 34
#define PATT      40
#define SIG       42
#define CKSUM     46
#define WBUF      34

char  ascii_check[6] = "READ";
int    miscom_offset = 0, smaller_size =0, larger_size = 0;
int    num_sub_blks_psect = 0; /* For unique_patterns   */
int    mis_count = 0;

main(int argc, char *argv[])
{
    int    *diff, data1_index, data2_index;
    int    loop_index, loop_index2, save_index, same_count, o_format;
    int    diff_range, diff_count, block_siz, new_block_siz, offset_assign;
    int    flag, error_status = 0;
    int    new_range, line_len, disp_line_start, range_begin, range_end, offset;
    int    mis_offset=0, k=0;
    char   msg[MAX_TEXT_MSG];
    char   msgtmp[1024];
    register long  i=0, j=0;
    unsigned short *sptr=0;
    int    sum = 0;
    int    *cksum = &sum;
    char    mis_option = 0;
    char    oper[20]; /* Holds the Stanza id */
    char    *tmp_str = NULL;


    long   file1_size, file2_size;
    FILE   *f1_ptr, *f2_ptr;
    char   *data1, *data2, *message1;
    char   ascii[17], command[20];

    char   *offset_format_d = "%.8d";
    char   *offset_format_h = "%.6X";
    char   *data_format_d = "%.3d";
    char   *data_format_h = "%.2X";
    char   option = 0;

    char   *data_format, *offset_format;    

    struct stat file1_status, file2_status;
    int    print_mode,print_size; /* print_mode 1 - prints the whole file */
    char   cmd_data[8]; /* store command line arguments */
    int    cmd_len;

    void help(int block, int o_format, int diff_range);

    if ( argc < 3 || argc > 5 ) {  /* check args for proper syntax */
      printf("Syntax: %s file1 file2 [block size] [-dx8ltp]\n", argv[0]);
      exit(-1);
    }
	cmd_len = 0;
	block_siz = 0;
	for ( i = 3; i < argc; i++) {
	   if ( argv[i][0] == '-' ) {
		  cmd_len = proc_cmdarg(argv[i],cmd_data,cmd_len,8);
		  if ( cmd_len == -1) {
             printf("Syntax: %s file1 file2 [block size] [-dx8ltp]\n", argv[0]);
             exit(-1);
		  }
	   } else { 
          block_siz = atoi(argv[i]);
	   }	
    } /* of for */

    if ( block_siz <= 0 )         /* assign defaul blksize */
       block_siz = DEF_BLOCK_SIZ;
	/*printf("cmd_len %d cmd_data %s blk_size %d \n",cmd_len,cmd_data,block_siz); */

    if ( (f1_ptr = fopen(argv[1], "r")) == NULL ) {            /* open files */
       printf("Cannot open file %s for reading !\n", argv[1]);
       error_status = 1;
    } else if ( stat(argv[1], &file1_status) == -1 ) {
       printf("Cannot fstat file %s !\n", argv[1]);
       error_status = 1;
    }     
    if ( (f2_ptr = fopen(argv[2], "r")) == NULL ) {
       printf("Cannot open file %s for reading !\n", argv[2]);
       error_status = 1;
    } else if ( stat(argv[2], &file2_status) == -1 ) {
       printf("Cannot fstat file %s !\n", argv[2]);
       error_status = 1;
    }

    file1_size = (int) file1_status.st_size;   /* get file size of each file */
    file2_size = (int) file2_status.st_size;
    if (file1_size <= file2_size) {                   /* assign smaller size */
       smaller_size = file1_size;
       larger_size  = file2_size ;
    } else {
       larger_size  = file1_size;
       smaller_size = file2_size;
    }
                                 /* dynamic memory allocation for each array */
    if ( (data1 = (char *)malloc(larger_size)) == NULL ) {
       printf("Cannot allocate memory to read file %s !\n", argv[1]);
       error_status = 1;
    }
    if ( (data2 = (char *)malloc(larger_size)) == NULL ) {
       printf("Cannot allocate memory to read file %s !\n", argv[2]);
       error_status = 1;
    }
    if ( (diff = (int *) malloc(larger_size * sizeof(int))) == NULL ) {
       printf("Cannot allocate memory for temporary array\n");
       error_status = 1;
    }
    if ( error_status )      /* if error found previously, then exit program */
      exit(-1);

    data1_index = 0;                                /* read data from file 1 */
    while ( fscanf(f1_ptr, "%c", &data1[data1_index]) == 1 )  
       data1_index++;
    fclose(f1_ptr);
    data2_index = 0;                                /* read data from file 2 */
    while ( fscanf(f2_ptr, "%c", &data2[data2_index]) == 1 )
       data2_index++;
    fclose(f2_ptr);

            /*****************************************************************/
            /* This section of code will compare the data files. The compare */
            /* starts with the beginning of the files and goes to the end of */
            /* the smallest file. If a miscompare is found then it will put  */
            /* a 1 into the array diff at the same index as the data arrays. */
            /* If one data array is larger than the other then the diff      */
            /* array will have 1 in it from the end of the smallest array to */
            /* the end of the larger array. Also the it will assign blanks   */
            /* to the smaller array to make both data arrays equal.          */
            /*****************************************************************/

	/*
	 * Commenting out the following code. Refer to Feature # 446206
	 * Code changes start, Nitin Gupta, HTX India, 22/04/2004
	 */

    /* for ( loop_index = 0; loop_index < smaller_size; loop_index++ ) 
       if (data1[loop_index] != data2[loop_index])
          diff[loop_index] = 1;
       else
          diff[loop_index] = 0;*/


        /* Check if unique_patterns used */
        if(((int)data1[38] == 0xa5) && ((int)data1[39] == 0xa5))
                num_sub_blks_psect = 3;
        else
                num_sub_blks_psect = 0;

  for ( i = 0; i < smaller_size; i += (DEF_BLOCK_SIZ/(num_sub_blks_psect +1)) )
  {
         strncpy(oper, &data1[i+16], 8);/* data1 is the wbuf */
         oper[16] = '\0'; /* Terminate the string */
         tmp_str = strstr(oper, "WRC");

          /* Calc the sum of 1st OVERHEAD shorts. Note: the  */
          /* sum was gen'd from an array of shorts so use a  */
          /* (short *) to index it for calculating the sum.  */
         *cksum = 0;
         sptr = (unsigned short *) &data2[i];
         for ( mis_offset = 0; mis_offset < OVERHEAD; mis_offset++ )
            *cksum += sptr[mis_offset];

           /* If sum is 0 then we don't want to compare the   */
           /* 1st OVERHEAD*2 bytes, else we want to compare   */
           /* the whole block. Note that wbuf is an array of  */
           /* chars so we have to double the OVERHEAD value   */
           /* (it's in shorts.) Also, if sum is 0 then the    */
           /* LBA fields should be compared as a final check  */
           /* to make sure the header can be ignored.        */

    if ( memcmp( data2 + i + SIG, BUFSIG, strlen( BUFSIG ) ) ||
         memcmp( data1 + i + SIG, BUFSIG, strlen( BUFSIG ) ) )
        {
                mis_offset = 0;
        } else if(tmp_str != NULL){ /* oper == WRC */
                mis_offset = 0;
        } else {
           mis_offset = OVERHEAD * 2;

                /*
                 * We are looking for the miscompares in the header
                 * separately in the following piece of code. So,
                 * compare only the data portion after the header
                 * in the while loop below.
                 */

                if( (*(unsigned *)data2 != *(unsigned *)data1)) {
                        for (k=(i+LBA); k<(i+LBA+4); k++){
                        if (data1[k] != data2[k])
                                diff[k] = 1; /* LBA */
                        else
                                diff[k] = 0; /* LBA */
                        }

                }else{
                        for (k=(i+LBA); k<(i+LBA+4); k++)
                        diff[k] = 0; /* LBA */
                }

                /* Ignore Time Stamp always */
                for (k=(i+TS); k<(i+TS+4); k++)
                diff[k] = 0; /* Time Stamp */

                 if ( memcmp( data1 + i + DEVICE, data2 + i + DEVICE, 8)  ){
                        /* Diskname does not match */
                        for (k=(i+DEVICE); k<(i+DEVICE+8); k++){
                        if ( data1[k] != data2[k] )
                                diff[k] = 1; /* Device */
                        else
                                diff[k] = 0; /* Device */
                        }
                 }else {
                        for (k=(i+DEVICE); k<(i+DEVICE+8); k++)
                        diff[k] = 0; /* Device */
                }

                /* Ignore Stanza-id always */
                for (k=(i+STANZA); k<(i+STANZA+8); k++)
                diff[k] = 0; /* Stanza ID */

                 if ( memcmp( data1 + i + HOST, data2 + i + HOST, 10)  ){
                        /* Hostname does not match */
                        for (k=(i+HOST); k<(i+HOST+10); k++){
                        if ( data1[k] != data2[k] )
                                diff[k] = 1; /* Host */
                        else
                                diff[k] = 0; /* Host */
                        }
                 }else {
                        for (k=(i+HOST); k<(i+HOST+10); k++)
                        diff[k] = 0; /* Host */
                }

                if (num_sub_blks_psect){
                         /* unique patterns used */
                        for (k=i+WBUF; k<(i+WBUF+6); k++)
                        diff[k] = 0; /* wbuf base addr and a5a5 */
                }else {
                         if ( memcmp( data1 + i + MACHINEID, data2 + i + MACHINEID, 6)  ){
                                /* machine-id does not match */
                                for (k=(i+MACHINEID); k<(i+MACHINEID + 6); k++){
                                if ( data1[k] != data2[k] )
                                        diff[k] = 1; /* Machine-id */
                                else
                                        diff[k] = 0; /* Machine-id */
                                }
                         }else {
                                for (k=(i+MACHINEID); k<(i+MACHINEID + 6); k++)
                                diff[k] = 0; /* Machine-id */
                        }
                }

                 if ( memcmp( data1 + i + PATT, data2 + i + PATT, 2)  ){
                        /* Pattern does not match */
                        for (k=(i+PATT); k<(i+PATT+2); k++){
                        if ( data1[k] != data2[k] )
                                diff[k] = 1; /* Pattern */
                        else
                                diff[k] = 0;
                        }
                 }else {
                        for (k=(i+PATT); k<(i+PATT+2); k++)
                        diff[k] = 0; /* Pattern */
                }

                if ( *cksum & 0xffff ) {
                        for (k=(i+CKSUM); k<(i+CKSUM+2); k++){
                        if ( data1[k] != data2[k] )
                                diff[k] = 1; /* Checksum */
                        else
                                diff[k] = 0;
                        }
                }else {
                        for (k=(i+CKSUM); k<(i+CKSUM+2); k++)
                        diff[k] = 0; /* Checksum */
                }

        } /* end else */

           if (mis_offset == 0)
                j = i;
           else
                j = i + (OVERHEAD * 2);

    while ( (mis_offset++) < (DEF_BLOCK_SIZ/(num_sub_blks_psect + 1)) ) {

      if ( data1[j] != data2[j] ){
        mis_count++;
        diff[j] = 1;
        } else{
         diff[j] = 0;
        }

        ++j;

    } /* end while */
        if (j>=smaller_size)
        break;

   } /* End for */

	/*
	 * Code changes end here, Refer to Feature number 446206
	 */

    for ( loop_index = smaller_size; loop_index < larger_size; loop_index++ ) {
       diff[loop_index] = 1;
       if ( data1_index <= data2_index )
          data1[loop_index]= ' ';
       else
          data2[loop_index]= ' ';
    }
           /******************************************************************/
           /* This section of code will assign the screen defaults and will  */
           /* print the two data arrays onto the screen.                     */
           /******************************************************************/
    data_format = data_format_h;                    /* default to hex format */
    offset_format = offset_format_h;                /* default to hex format */
    line_len = DISP_SIZ;                            /* default to DISP_SIZ   */
    disp_line_start = 0; 
    save_index = 0;         
    diff_range = 24;
    command[0]='0';                           /* default to the first screen */
	print_mode = 0;
	while (1) {                              /* begin loop to control screen data */
	  
	  if (cmd_len == 0) { /* there are command line commands*/
         if (command[0] != 'q')            /* clear screen before each write of */
	#ifdef __HTX_LINUX__
            system("clear");    /* screen unless user wants out of display */
	#else
            system("tput clear");    /* screen unless user wants out of display */
	#endif
	  } else {
		 command[0] = cmd_data[cmd_len -1];
		 cmd_len--;
	  }

      message1 = "";                    /* initialize message1 to NULL       */
      switch (command[0]) {
                        /*****************************************************/
                        /* user selected help. check what format the offsets */
                        /* are in and pass that along with the current block */
                        /* size to the help function.                        */
                        /*****************************************************/ 
        case '?':           
          if (offset_format == offset_format_h)
             o_format = 1;
          else
             o_format = 0; 
          help(block_siz, o_format, diff_range);
          break;
                        /*****************************************************/
                        /* user wants to change the offset format from       */
                        /* hexadecimal to decimal. Allow both lower or upper */
                        /* case d for the command.                           */ 
                        /*****************************************************/
        case 'd':
        case 'D':
          offset_format = offset_format_d;
          break;
                        /*****************************************************/
                        /* user wants to change the offset format from       */
                        /* decimal to hexadecimal. Allow both lower or upper */
                        /* case x for the command.                           */
                        /*****************************************************/
        case 'x': 
        case 'X':
          offset_format = offset_format_h;
          break;
                        /*****************************************************/
                        /* user wants to change the the ascii data being     */
                        /* displayed. He can only flip-flop from READ to     */
                        /* WRITE. Default is READ.                           */
                        /*****************************************************/
        case 't': 
        case 'T':
          if ( strcmp(ascii_check, "READ") == 0 )
             strcpy(ascii_check, "WRITE");
          else
             strcpy(ascii_check, "READ");
          break;
                        /*****************************************************/
                        /* user wants to change the screen display from 10   */
                        /* bytes displayed to 8 bytes displayed. Make sure   */
                        /* that the offset is set to about the same place as */
                        /* it was in the 10 byte display.                    */
                        /*****************************************************/
        case '8': 
          offset = (disp_line_start + 8) * line_len; 
          line_len = 8;     
          disp_line_start = offset / line_len - SCREEN_SIZ/2;
          if ( disp_line_start > (larger_size - 1) / line_len - SCREEN_SIZ+1) { 
             disp_line_start = (larger_size - 1) / line_len - SCREEN_SIZ + 1;
             message1 = "This is the last screen!";
          }
          if ( disp_line_start < 0 ) {
             disp_line_start = 0;
             message1 = "This is the first screen!";
          } 
          break;
                        /*****************************************************/
                        /* user wants to change the screen display from 8    */
                        /* bytes displayed to 10 bytes displayed. Make sure  */
                        /* that the offset stay about the same place as it   */
                        /* was in the 8 byte display. Allow both upper and   */
                        /* lower case l for the command.                     */
                        /*****************************************************/ 
        case 'l': 
        case 'L':     
          offset = (disp_line_start + 13) * line_len;
          line_len = 10 ; 
          disp_line_start = offset / line_len - SCREEN_SIZ / 2;
          if ( disp_line_start > (larger_size - 1) / line_len - SCREEN_SIZ+1) {
             disp_line_start = (larger_size - 1) / line_len - SCREEN_SIZ + 1;
             message1 = "This is the last screen!";
          }
          if ( disp_line_start < 0 ) {
             disp_line_start = 0;
             message1 = "This is the first screen!";
          } 
          break;
                         /****************************************************/
                         /* user hit the enter button and wants to scroll    */
                         /* forward one line. Make sure you don't go pass    */
                         /* the end of the file.                             */
                         /****************************************************/
        case ' ': 
        case '\0':
          disp_line_start = disp_line_start + 1 ;
	  if ( disp_line_start > (larger_size - 1) / line_len - SCREEN_SIZ+1 ) {
             disp_line_start = disp_line_start - 1;
             message1 = "This is the last screen!";
          }
          break;
                         /****************************************************/
                         /* user hit a dash and wants to scroll backward one */
                         /* line in the file. Make sure you do not go past   */
                         /* the beginning of the file.                       */
                         /****************************************************/
        case 'u': 
        case 'U': 
          disp_line_start = disp_line_start - 1;
          if ( disp_line_start < 0 ) {
             disp_line_start = 0;
             message1 = "This is the first screen!";
          }
          break;
                            /*************************************************/
                            /* user hit a zero and wants to position himself */
                            /* to the beginning of the file.                 */
                            /*************************************************/
        case '0': 
          disp_line_start = 0;
          break;
                            /*************************************************/
                            /* user input an + to scroll forward by one page.*/
                            /* Allow either an upper or lower case f.        */
                            /*************************************************/ 
        case '+':  
          disp_line_start = disp_line_start + SCREEN_SIZ;
	  if ( disp_line_start > (larger_size - 1) / line_len ) {
             disp_line_start = disp_line_start - SCREEN_SIZ;
             message1 = "This is the last screen!";
          }
          break;      
                  /***********************************************************/
                  /* user input a g which means he wants to position himself */
                  /* to the first occurrence of a miscompare. Allow the use  */
                  /* of either an upper or lower case f. Search the diff     */
                  /* array for a 1 and set the screen line start at that     */
                  /* position. This is done by taking the array element found*/
                  /* and dividing it by the line length being used.          */
                  /***********************************************************/
        case 'f':  
        case 'F':
          for( loop_index = 0; loop_index <= larger_size - 1; loop_index++ ) {
             if ( diff[loop_index] == 1 ) {
                disp_line_start = loop_index / line_len;
                save_index = loop_index;
                loop_index = larger_size;
             }
          }    
          if ( loop_index == larger_size )
             message1 = "There was no miscompare found!"; 
          break; 
                  /***********************************************************/
                  /* user input an n which means he wants to position        */
                  /* himself to the next occurrence of a miscompare. The     */
                  /* user should have used a g to find the first occurrence  */
                  /* of a miscompare and then this option will take him to   */
                  /* the next. Allow both upper or lower case "n".           */
                  /***********************************************************/
        case 'n':  
        case 'N':
          same_count = 0;
          flag = 0;
          for ( loop_index = save_index; loop_index <= larger_size - 1;
                loop_index++ ) {
             if ( diff[loop_index] == 0 ) 
                same_count++;
             else
                same_count = 0;
             if ( same_count == diff_range ) {
                for ( loop_index++; loop_index <= larger_size -1; 
                      loop_index++ ) {
                   if ( diff[loop_index] == 1 ) {
                      disp_line_start = loop_index / line_len;
                      save_index = loop_index;
                      loop_index = larger_size;
                      flag = 1;
                   }
                }
             }
          }    
          if ( loop_index >= larger_size && flag == 0 )
             message1 = "There were no other miscompares found!"; 
          break; 
                         /****************************************************/
                         /* user input an m which means he wants to position */
                         /* himself to the last occurrence of the current    */
                         /* miscompare.                                      */
                         /****************************************************/
        case 'e':  
        case 'E':
          same_count = 0;
          flag = 0;
          for ( loop_index = save_index; loop_index <= larger_size - 1;
                loop_index++ ) {
             if ( diff[loop_index] == 0 ) 
                same_count++;
             else
                same_count = 0;
             if ( same_count == diff_range ) {
                 disp_line_start = (loop_index - diff_range) / line_len;
                 loop_index = larger_size;
                 flag = 1;
             }
          }    
          if ( loop_index >= larger_size && flag == 0 ) {
	     disp_line_start = (larger_size - 1) / line_len - SCREEN_SIZ + 1;
	     if ( disp_line_start < 0 )
	        disp_line_start = 0;
             message1 = "The end of the miscompare is also the end of data!";
          }
          break;
                           /**************************************************/
                           /* user input a n which means he wants to reset   */
                           /* the range of good bytes between a miscompare.  */
                           /* Allow both upper or lower case "s".            */
                           /**************************************************/
        case 's':  
        case 'S':
          if ( sscanf(command, " %*c %d", &new_range) == 1 )
             diff_range = new_range;
          if ( diff_range < 1 ) 
             message1 = "Range < 1 is not allowed! Range reset to 24!\n";
          break;
                           /**************************************************/
                           /* user input a b which means he wants to go back */
                           /* one screen.                                    */
                           /**************************************************/
        case '-':  
          disp_line_start = disp_line_start - SCREEN_SIZ;
          if ( disp_line_start < 0 ) {
             disp_line_start = 0;
             message1 = "This is the first screen!";
          }
          break;
                           /**************************************************/
                           /* user input a $ which means he wants to jump to */
                           /* the last page of the data.                     */
                           /**************************************************/ 
	case '$':
	  disp_line_start = (larger_size - 1) / line_len - SCREEN_SIZ + 1;
	  if ( disp_line_start < 0 )
	     disp_line_start = 0;
	  break;
                           /**************************************************/
                           /* user input an s which means he would like to   */
                           /* change the blocking factor on the file. syntax */
                           /* for this command is s xxx where xxx is the new */
                           /* block size. allow for either upper or lower    */
                           /* b to be used.                                  */
                           /**************************************************/
	case 'b': 
	case 'B':
	  if ( sscanf(command, " %*c %d", &new_block_siz ) == 1 )
	     block_siz = new_block_siz;
	  else
	     message1 = "Can NOT assign the new block size !!!";
          break;

                           /**************************************************/
                           /* user input p which means he would like to   */
                           /* print the whole file.                       */
                           /* allow either an upper or lower case p to be used */
                           /**************************************************/
	case 'p': 
	case 'P':
	     print_mode = 1; /* print mode selected */
          break;
                         /****************************************************/
                         /* user input a q and wants to quit the program.    */
                         /* allow either an upper or lower case q to be used */
                         /****************************************************/
        case 'q':
        case 'Q':
          exit(0);     
          break;
                        /*****************************************************/
                        /* user input an offset and the screen should be     */
                        /* realigned to this new offset. the user just has   */
                        /* to type in a valid offset. it can be either in    */
                        /* hex or decimal.                                   */
                        /*****************************************************/
        default: 
          if ( strcmp(offset_format, offset_format_d) == 0 ) 
             offset_assign = sscanf(command, "%*c %d", &offset );
          else 
             offset_assign = sscanf(command, "%*c %x", &offset );
          if ( offset_assign == 1 ) {    /* able to assign a value to offset */
             disp_line_start = offset / line_len - SCREEN_SIZ / 2;
	     if (disp_line_start > (larger_size-1) / line_len - SCREEN_SIZ +1) {
	        disp_line_start = (larger_size - 1) / line_len - SCREEN_SIZ +1;
                message1 = "This is the last screen!";
             }
             if ( disp_line_start < 0 ) {
                disp_line_start = 0;
                message1 = "This is the first screen!";
             }
          }
      }

	  if ( cmd_len ) /* more commands to process */
		 continue; /* do not print */
                                           /* print header for offset in hex */
      if ( offset_format == offset_format_h ) {
         if ( line_len == 8 ) {   
           if ( strcmp(ascii_check, "READ") == 0 ) {
            printf("             DATA WRITTEN                DATA READ BACK");
            printf("           ASCII - READ\n");
            printf("OFFSET   0  1  2  3  4  5  6  7        0  1  2  3  4  5  ");
            printf("6  7       01234567\n");
           } else {
            printf("             DATA WRITTEN                DATA READ BACK");
            printf("           ASCII - WRITE\n");
            printf("OFFSET   0  1  2  3  4  5  6  7        0  1  2  3  4  5  ");
            printf("6  7       01234567\n");
           }
         } else {
            printf("             DATA WRITTEN                      ");
            printf("DATA READ BACK\n");
            printf("OFFSET   0  1  2  3  4  5  6  7  8  9        0  1  2  3");
            printf("  4  5  6  7  8  9\n");
         }
      }
                                           /* print header for offset in dec */
      if ( offset_format == offset_format_d ) {
         if ( line_len == 8 ) {     
           if ( strcmp(ascii_check, "READ") == 0 ) {
            printf("               DATA WRITTEN                DATA ");
            printf("READ BACK          ASCII - READ\n"); 
            printf("OFFSET     0  1  2  3  4  5  6  7        0  1  2  3  4");
            printf("  5  6  7       01234567\n");
           } else {
            printf("               DATA WRITTEN                DATA ");
            printf("READ BACK         ASCII - WRITE\n"); 
            printf("OFFSET     0  1  2  3  4  5  6  7        0  1  2  3  4");
            printf("  5  6  7       01234567\n");
           }
         } else {
            printf("               DATA WRITTEN                      DATA ");
            printf("READ BACK\n");
            printf("OFFSET     0  1  2  3  4  5  6  7  8  9        0  1  2");
            printf("  3  4  5  6  7  8  9\n");
         }
      }
        /* for each iteration of loop_index, print a line of data comparison */
      if ( print_mode )
		  print_size = larger_size/line_len - disp_line_start;
	  else
		  print_size = SCREEN_SIZ - 1;

      /* for ( loop_index = 0; loop_index <= SCREEN_SIZ - 1; loop_index++ ) { */
      for ( loop_index = 0; loop_index <= print_size; loop_index++ ) {
         range_begin = (disp_line_start + loop_index) * line_len;
         range_end   = range_begin + line_len-1;
                                              /* print offset as range_begin */
         printf(offset_format, range_begin);
                     /* check to see if a line contains the start of a block */
	                    /* and print > if it does                        */
	 if ( range_end < larger_size ) {
            if ( range_end%block_siz < line_len )
               printf("> ");
            else
               printf(": ");
         } else {
	    if ( (larger_size -1) % block_siz < line_len )
               printf("> ");
            else
              printf(": ");
         }
                                     /* print line_len # of data from array1 */
         for ( loop_index2 = range_begin; loop_index2 <= range_end; 
               loop_index2++ )
            if ( loop_index2 <= (data1_index - 1) ) {
               if ( loop_index2 % block_siz == 0 )
                  printf("_");
               else
                  printf(" ");
               printf(data_format, data1[loop_index2]);
            } else
               printf("   ");
                                           /* calculate diff_count and print */
         diff_count = 0 ;                  /* reinitialize diff_count        */
         for ( loop_index2 = range_begin; loop_index2 <= range_end; 
               loop_index2++ )
	    if ( loop_index2 <= larger_size - 1 ) 
               diff_count = diff_count + diff[loop_index2] ;
         if ( diff_count == 0 )
            printf("  ==  ", diff_count);
         else
            printf(" #%2d# ", diff_count);
                                     /* print line_len # of data from array2 */
         for ( loop_index2 = range_begin; loop_index2 <= range_end; 
               loop_index2++ )
            if ( loop_index2 <= data2_index - 1 ) {
               if ( loop_index2 % block_siz == 0 )
                  printf("_");
               else
                  printf(" ");
               printf(data_format, data2[loop_index2] );
            } else
               printf("   ");

         if ( line_len == 8 && strcmp(ascii_check, "READ") == 0 ) {
            for ( i = 0; i < 6; i++ )
               ascii[i] = ' ';
            i = 6;
            for ( loop_index2 = range_begin; loop_index2 <= range_end; 
                  loop_index2++ ) {
               ascii[i] = '\0';
               if ( loop_index2 <= (data2_index - 1) ) 
		{
                  if ( isprint(data2[loop_index2]) )
		   {
                     ascii[i] = data2[loop_index2];
		   } else {
                     ascii[i] = '.';
		   }
	        }
               i++;
            }
            printf("%s", ascii);
         } else if ( line_len == 8 && strcmp(ascii_check, "WRITE") == 0 ) {
            for ( i = 0; i < 6; i++ )
               ascii[i] = ' ';
            i = 6;
            for ( loop_index2 = range_begin; loop_index2 <= range_end; 
                  loop_index2++ ) {
               ascii[i] = '\0';
               if ( loop_index2 <= data1_index - 1 )
 		{
                  if ( isprint(data1[loop_index2]) )
                     ascii[i] = data1[loop_index2];
                  else
                     ascii[i] = '.';
		}
               i++;
            }
            printf("%s", ascii);
         } else {
            for ( i = 0; i < 17; i++ )
               ascii[i] = '\0';
         }
         printf("\n");
      }
      printf("%s\n",message1);
	  if (print_mode) {
		 exit(0);
	  }
      printf("Please enter command (? for help):");
      if ( gets(command) == NULL )
		break; /*while */
   } /* of while */
}

void help(int block, int format_o, int diff_range)
{  
  int i;

  for ( i = 1; i <= 12; i++ )
     printf("\n");    
  printf("NOTE:  The symbol > indicates a line that contains the start\n");
  printf("       of a new block. The symbol _ precedes the byte where\n");
  printf("       the new block begins.\n\n");   
  printf("0     : Jump to first page\n");
  printf("$     : Jump to last page\n");
  printf("+     : Scroll forward one page\n");     
  printf("enter : Scroll forward one line\n"); 
  printf("-     : Scroll backward one page\n");
  printf("u     : Scroll backward one line\n");
  printf("f     : Jump to the start of the first comparison error\n");    
  printf("n     : Jump to the start of the next comparison error\n");
  printf("e     : Jump to the end of the comparison error that was last\n");
  printf("        found by using the 'f' or 'n' command\n");
  printf("s xxx : Span of good bytes between miscompares (current = %d)\n",
          diff_range);
  printf("j xxxx: Jump to offset xxxx (xxxx is currently ");
  if ( format_o == 1 )
     printf("hexadecimal)\n");
  else  
     printf("decimal)\n");
  printf("d     : Display offset field in decimal\n");
  printf("x     : Display offset field in hexadecimal\n");
  printf("8     : Set display to 8 bytes/line (integral power-of-2)\n");
  printf("l     : Set display to 10 bytes/line (for more bytes/page)\n");
  printf("b xxx : Set blocksize to xxx in decimal (current = %d)\n",block);
  printf("p     : Display data up to the end of file & exit\n");
  printf("t     : Set ascii data to be displayed to Read or Write");
  printf(" (current = %s)\n", ascii_check);
  printf("q     : Exit this program.\n");
  printf("Hit enter to exit help:\n");
  getchar();
}

int proc_cmdarg(char * cmd, char *cmd_data,int cur_len, int max_size)
{
  int cmd_no,i;

  for (cmd_no = cur_len ; cmd[i] != NULL && cmd_no < max_size; i++) {
	 switch(cmd[i]) {
		case '-' :
			break;
		
		case 'd': case 'D':
		case 'x': case 'X':
		case '8':
		case 'l': case 'L':
		case 't': case 'T':
		case 'p': case 'P':
			cmd_data[cmd_no++] = cmd[i];
			break;
	 
        default :
			printf("Invalid argument: %c\n",cmd[i]);
			return -1;
	  }
   } /* of while */

   return cmd_no;
}


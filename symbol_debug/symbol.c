/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "symbol.h"

int find_sym_by_addr(unsigned long address, char* rom_map, char* symbol_name, char* file_name, char *type);
int find_sym_in_files(unsigned int address, char type,
                      int map_num, char* map_files[],
                      char* symbol_name, char* file_name);

void do_back_trace(int map_num, char *map_files[]);

unsigned int *debug_info;
int info_count = 0;
struct register_dump_s *dump = NULL;
unsigned int *stack_frame = NULL;

/*usages::*/
char *usage = "symbol <dump info> <map file1> <map file2> .... <map file n>\n";


/*arg1: error file name
 * arg2: map file 1 
 * arg3: map file 2
 * ....
 * argN: map file N-1
 */
int main(int argc, char* argv[])
{
    FILE *fp;
    char *line;
    size_t len = 0;
    ssize_t read;
    char *str_token;
    char *str_tok_save;
    unsigned int data;

    if(argc < 3)
    {
        printf("%s", usage);
        exit(EXIT_FAILURE);
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
        exit(EXIT_FAILURE);
    }
    
    line = (char*)malloc(256);
    if(!line){
        exit(EXIT_FAILURE);
    }
       
    debug_info = (unsigned int*)malloc(sizeof(struct register_dump_s) + 
                                       MAX_STACK_FRAM_DEPTH * sizeof(unsigned int));
    if(!debug_info) 
    {
        free(line);
        exit(EXIT_FAILURE);
    }

    memset(line, 0, sizeof(line));
    /*read debug info*/ 
    while ((read = getline(&line, &len, fp)) != -1)
    {
         if((line[0] != '0') || (line[1] != 'x'))
         {
             continue;
         }

         str_token = strtok_r(line, " \n", &str_tok_save);
         do{
             data = (unsigned int)strtoul(str_token, NULL, 16);
             if((errno != EINVAL) || (data != 0))
             {
                 debug_info[info_count] = data;
                 info_count++;
             }else{
                 printf("error info_count %d %d %x\n", info_count, errno, data);
             }
         }while((str_token = strtok_r(NULL, " \n", &str_tok_save)) != NULL);
    }

    dump  = (struct register_dump_s *)debug_info;

    if(info_count >= (MAX_REGDUMP_IN_WORD + MAX_STACK_FRAM_DEPTH))
        stack_frame = debug_info + MAX_REGDUMP_IN_WORD;

    /*print exception info*/
    do_back_trace(argc - 2, &argv[2]); 

    free(debug_info);
    free(line);
    fclose(fp);
    return 0;
}

/*
 * find address symbol from given map file. return buffer: symbol_name, file_name, type
 * function return TRUE/FALSE
 */
int find_sym_by_addr(unsigned long address, char* rom_map, char* symbol_name, char* file_name, char *type)
{
    FILE *fp;
    char *line = NULL;
    char *str_token = NULL;
    char *str_tok_save;
    size_t len = 0;
    ssize_t read;
    unsigned int num = 0;
    unsigned long cur_addr;
    unsigned long prev_addr;
    char prev_sym[MAX_SYMBOL_NAME_LEN];
    char prev_file[MAX_FILE_NAME_LEN];
    char prev_type;
    bool found = FALSE;

    prev_type = '0';
    memset(prev_sym, 0, sizeof(prev_sym));
    memset(prev_file, 0, sizeof(prev_file));

    if(!symbol_name || !file_name || !type)
    {
        printf("symbol, file or type buffer is null!\n");
        return 0;
    }

    fp = fopen(rom_map, "r");
    if (fp == NULL)
    {
        exit(EXIT_FAILURE);
    }

    /*mask address to strip unnecessary info (cache, uncache...)*/ 
    address &= ADDRESS_MASK;
    while ((read = getline(&line, &len, fp)) != -1) 
    {
         /*address*/
         str_token = strtok_r(line, " \t\n", &str_tok_save);
         cur_addr = strtoul(str_token, NULL, 16);
         /*see if we found*/
         if(cur_addr > address)
         {
             /*found*/
             found = TRUE;
             break;
         }
         prev_addr = cur_addr;

         /*type*/
         str_token = strtok_r(NULL, " \t\n", &str_tok_save);
         prev_type = str_token[0];

         /*symbol name*/
         str_token = strtok_r(NULL, " \t\n", &str_tok_save);
         strcpy(prev_sym, str_token);

         /*file line num*/
         str_token = strtok_r(NULL, " :\t\n", &str_tok_save);
         if(str_token)
         {
             strcpy(prev_file, str_token);
         }else
         {
             strcpy(prev_file, "\0");
         }
         num++;
    }

    if(found && prev_type != '0') /*if prev_type is '0' it means the address is invalid*/
    {
        strcpy(symbol_name, prev_sym);
        strcpy(file_name, prev_file);
        *type = prev_type;
    } 
    else
    {
        strcpy(symbol_name, "unknown");
        strcpy(file_name, "unknown");
        *type = '0';
    }

    if (line)
    {
        free(line);
    }

    fclose(fp);
    return found;
}

/*
 *find address and specific type from list of map_files. return buffer: symbol_name, file_name.
 *function return: TRUE/FALSE
 */
int find_sym_in_files(unsigned int address, char type, 
                      int map_num, char* map_files[], 
                      char* symbol_name, char* file_name)
{
    int m;
    char sym_type = 0;

    if(!symbol_name || !file_name)
    {
        printf("symbol, file or type buffer is null!\n");
        return FALSE;
    }

    for(m = 0; m < map_num; m++)
    {
        if(find_sym_by_addr(address, map_files[m], symbol_name, file_name, &sym_type))
        {
            if(type == sym_type)
            {
                return TRUE;  /*found!*/
            }
        }
    }

    strcpy(symbol_name, "NONE");
    strcpy(file_name, "NONE");
    return FALSE;
}

/*
 * Trace the dump_register_s and print the stack. 
 */
void do_back_trace(int map_num, char* map_files[])
{
    int i = 0;
    char sym_name[MAX_SYMBOL_NAME_LEN];
    char file_name[MAX_FILE_NAME_LEN];
    unsigned int address;

    printf("====================debug information===================\n");
    printf("target id = %x\n", dump->target_id);
    printf("assertion line = %d\n", dump->assline);

    /*except pc*/
    address = dump->pc & ADDRESS_MASK;
    if(!find_sym_in_files(address, 'T', map_num, map_files, sym_name, file_name))
    {
        find_sym_in_files(address, 't', map_num, map_files, sym_name, file_name);
    }
    printf("exception pc = 0x%.8x %s %s\n", address, sym_name, file_name); 

    /*bad add*/
    address = dump->badvaddr & ADDRESS_MASK;
    if(!find_sym_in_files(address, 'T', map_num, map_files, sym_name, file_name))
    { 
        find_sym_in_files(address, 't', map_num, map_files, sym_name, file_name);
    }
    printf("bad virtual pc = 0x%.8x  %s %s\n", address, sym_name, file_name);

    /*program state*/
    printf("program state = 0x%.8x\n", dump->exc_frame.xt_ps);

    /*execption cause*/
    printf("cause = 0x%.8x\n", dump->exc_frame.xt_exccause); 
    printf("=========================================================\n\n");

    printf("------------------------stack end------------------------------\n");
    if(stack_frame)
    {
        if (dump->pc != 0) 
        {    
            for(i = 0; i < MAX_STACK_FRAM_DEPTH; i++)
            {
                address = stack_frame[i] & ADDRESS_MASK;
                if(!find_sym_in_files(address, 'T', map_num, map_files, sym_name, file_name))
                {
                    find_sym_in_files(address, 't', map_num, map_files, sym_name, file_name);
                }
                printf("#%d  0x%.8x in %s (arg TBD) %s\n", i, address, sym_name, file_name);
            }
        }else 
        {
            address = stack_frame[3] & ADDRESS_MASK;
            printf("mem ctl isr epc=%x\n", stack_frame[3]);
            if(!find_sym_in_files(address, 'T', map_num, map_files, sym_name, file_name))
            {
                find_sym_in_files(address, 't', map_num, map_files, sym_name, file_name);
            }
            printf("#%d  0x%.8x in %s (arg TBD) %s\n", i, address, sym_name, file_name);
        }
    }else
    {
        for(i = 0; i<REGDUMP_FRAMES; i++)
        {
            address = (dump->exc_frame).wb[i].a0 & ADDRESS_MASK;
            if(!find_sym_in_files(address, 'T', map_num, map_files, sym_name, file_name))
            {
                find_sym_in_files(address, 't', map_num, map_files, sym_name, file_name);
            }
            printf("#%d  0x%.8x in %s (arg TBD) %s\n", i, address, sym_name, file_name);
        }
    }
    printf("------------------------stack start------------------------------\n");
    return;
}

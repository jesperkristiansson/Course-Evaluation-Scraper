#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

#define INITIAL_BUF_SIZE 1024

static void crash(const char *msg){
    fprintf(stderr, "Program crashed: %s\n", msg);
    exit(EXIT_FAILURE);
}

static char *read_from_file(const char *path){
    FILE *fp = fopen(path, "r");
    if(fp == NULL){
        crash("fopen");
    }

    //find size of file
    if(fseek(fp, 0, SEEK_END) == -1){
        crash("fseek");
    }
    long file_size = ftell(fp);
    if(file_size == -1){
        crash("ftell");
    }
    rewind(fp);

    //create buffer
    char *buf = malloc(sizeof(*buf)*(file_size + 1));
    if(buf == NULL){
        crash("malloc");
    }

    //read contents into buffer
    if(fread(buf, 1, file_size, fp) == 0){
        crash("fread");
    }

    //make sure string is null-terminated
    buf[file_size] = '\0';

    if(fclose(fp) != 0){
        crash("fclose");
    }

    return buf;
}

char **find_urls(const char *path, int *size){
    char *buf = read_from_file(path);

    size_t found_urls_size = INITIAL_BUF_SIZE;
    char **found_urls = calloc(found_urls_size, sizeof(*found_urls));
    size_t number_of_found_urls = 0;

    regex_t reg;
    //matches all strings in the form "http://www.ceq.lth.se/rapporter/YYYY_XT/LPX/[COURSE_CODE]_YYYY_XT_LPX_slutrapport.html"
    //which is a link to a swedish report of course [COURSE_CODE]
        //the course code is in the form of either ABCDXX or ABCXXX
        //all years are matched with YYYY
        //all study periods (LP1-LP4) are matched
        //all semesters (HT/VT)
    //const char *course_code_pattern = "[[:upper:]]\\{4\\}[[:digit:]]\\{2\\}\\|[[:upper:]]\\{3\\}[[:digit:]]\\{3\\}";
    const char *pattern = "http:\\/\\/www\\.ceq\\.lth\\.se\\/rapporter\\/\\([[:digit:]]\\)\\{4\\}_.T\\/LP.\\/\\([[:upper:]]\\{4\\}[[:digit:]]\\{2\\}\\|[[:upper:]]\\{3\\}[[:digit:]]\\{3\\}\\)_\\([[:digit:]]\\)\\{4\\}_.T_LP._slutrapport_en\\.html";
    if(regcomp(&reg, pattern, 0) != 0){
        crash("regcomp");
    }

    regmatch_t match;
    int status;
    int offset = 0;
    do{
        status = regexec(&reg, buf + offset, 1, &match, 0);
        if(status != 0){
            break;
        }
        int len = match.rm_eo - match.rm_so;

        //add url to found_urls
            //allocates memory and copies string, could instead point to different offsets in buf for efficieny
        char *str = malloc(len + 1);
        strncpy(str, buf + offset + match.rm_so, len);
        str[len] = '\0';
        if(number_of_found_urls == found_urls_size){
            found_urls_size *= 2;
            found_urls = realloc(found_urls, sizeof(*found_urls)*found_urls_size);
        }
        found_urls[number_of_found_urls++] = str;

        offset += match.rm_eo;
    } while(status == 0);

    *size = number_of_found_urls;
    
    regfree(&reg);
    free(buf);

    return found_urls;
}
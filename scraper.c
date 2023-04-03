#include <curl/curl.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <inttypes.h>

#define URL "https://www.ceq.lth.se/rapporter/?lasar_lp=2122&program=alla&kurskod=&sort=program"
#define BUF_START_SIZE 4096

typedef char byte;

struct recvbuf{
    byte *buf;
    size_t current_size;
    size_t max_size;
};

struct course_info{
    char *name;
    char *code;
    int8_t teaching_score;      //int8_t is used since the possible values are [-100,100]
    int8_t clear_goal_score;
    int8_t assessment_score;
    int8_t workload_score;
    int8_t importance_score;
    int8_t satisfaction_score;
    float average_score;       //can be calculated from other fields, is stored to cache result
};

char **find_urls(const char *path, int *size);

void crash(const char *msg){
    fprintf(stderr, "Program crashed: %s\n", msg);
    exit(EXIT_FAILURE);
}

size_t write_cb_curl(char *ptr, size_t size, size_t nmemb, void *userdata){
    struct recvbuf *buf = (struct recvbuf *) userdata;
    size_t tot_size = size*nmemb;
    size_t new_size = buf->current_size + tot_size;
    if(new_size >= buf->max_size){
        buf->max_size = new_size*2;
        buf->buf = realloc(buf->buf, buf->max_size);
    }
    memcpy(&buf->buf[buf->current_size], ptr, tot_size);
    buf->current_size = new_size;
    return tot_size;
}

void init_easy_handle(CURL *handle, const char* url, struct recvbuf *buf){
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_cb_curl);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, buf);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
}

int init_recvbuf(struct recvbuf *buf, size_t max_size){
    byte *ptr = malloc(max_size);
    if(ptr == NULL){
        return 1;
    }

    buf->buf = ptr;
    buf->max_size = max_size;
    buf->current_size = 0;
    return 0;
}

void reset_recvbuf(struct recvbuf *buf){
    buf->current_size = 0;
}

void cleanup_recvbuf(struct recvbuf *buf){
    free(buf->buf);
}

void print_course_info(struct course_info *info){
    printf("Course name: %s\n", info->name);
    printf("Course code: %s\n", info->code);
    printf("Good Teaching: %" PRId8 "\n", info->teaching_score);
    printf("Clear Goals and Standards: %" PRId8 "\n", info->clear_goal_score);
    printf("Appropriate Assessment: %" PRId8 "\n", info->assessment_score);
    printf("Appropriate Workload: %" PRId8 "\n", info->workload_score);
    printf("The course seems important for my education: %" PRId8 "\n", info->importance_score);
    printf("Overall satisfaction: %" PRId8 "\n", info->satisfaction_score);
    printf("Average score: %.2f\n", info->average_score);
    printf("\n");
}

regmatch_t regex_match(const char *str, const char *pattern){
    regex_t reg;
    if(regcomp(&reg, pattern, 0) != 0){
        crash("regcomp");
    }

    regmatch_t match;
    if(regexec(&reg, str, 1, &match, 0) != 0){
        crash("regexec");
    }

    regfree(&reg);
    return match;
}

char *first_string_before_delim(const char *buf, char delim){   //maybe do everything in one loop instead of going over buf twice?
    int str_len = 0;
    while(buf[++str_len] != delim);

    char *ptr = malloc((str_len+1)*sizeof(char));
    strncpy(ptr, buf, str_len);
    ptr[str_len] = '\0';
    return ptr;
}

int course_compare(const void *data1, const void *data2){
    struct course_info *course1 = (struct course_info *) data1;
    struct course_info *course2 = (struct course_info *) data2;
    if(course2->average_score < course1->average_score){
        return -1;
    } else if(course2->average_score == course1->average_score){
        return 0;
    } else{
        return 1;
    }
}

void find_stuff(char *buf, struct course_info *info){
    const char *pre_name_pattern = "<tr><td>Course name<\\/td><td><b>";
    const char *pre_code_pattern = "<tr><td>Course code<\\/td><td>";
    const char *pre_teaching_pattern = "<tr><td>Good Teaching<\\/td><td align=\"right\">";
    const char *pre_goal_pattern = "<tr><td>Clear Goals and Standards<\\/td><td align=\"right\">";
    const char *pre_assessment_pattern = "<tr><td>Appropriate Assessment<\\/td><td align=\"right\">";
    const char *pre_workload_pattern = "<tr><td>Appropriate Workload<\\/td><td align=\"right\">";
    const char *pre_importance_pattern = "<tr><td>The course seems important for my education<\\/td><td align=\"right\">";
    const char *pre_satisfaction_pattern = "<tr><td>Overall, I am satisfied with this course<\\/td><td align=\"right\">";

    regmatch_t match;
    char *temp;

    //course name
    match = regex_match(buf, pre_name_pattern); //consider moving into function
    buf += match.rm_eo;
    info->name = first_string_before_delim(buf, '<');

    //course name
    match = regex_match(buf, pre_code_pattern); //consider moving into function
    buf += match.rm_eo;
    info->code = first_string_before_delim(buf, '&');

    //teaching score
    match = regex_match(buf, pre_teaching_pattern);
    buf += match.rm_eo;
    temp = first_string_before_delim(buf, '<');
    info->teaching_score = atoi(temp);
    free(temp);

    //goal score
    match = regex_match(buf, pre_goal_pattern);
    buf += match.rm_eo;
    temp = first_string_before_delim(buf, '<');
    info->clear_goal_score = atoi(temp);
    free(temp);

    //assessment score
    match = regex_match(buf, pre_assessment_pattern);
    buf += match.rm_eo;
    temp = first_string_before_delim(buf, '<');
    info->assessment_score = atoi(temp);
    free(temp);

    //workload score
    match = regex_match(buf, pre_workload_pattern);
    buf += match.rm_eo;
    temp = first_string_before_delim(buf, '<');
    info->workload_score = atoi(temp);
    free(temp);

    //importance score
    match = regex_match(buf, pre_importance_pattern);
    buf += match.rm_eo;
    temp = first_string_before_delim(buf, '<');
    info->importance_score = atoi(temp);
    free(temp);

    //satisfaction score
    match = regex_match(buf, pre_satisfaction_pattern);
    buf += match.rm_eo;
    temp = first_string_before_delim(buf, '<');
    info->satisfaction_score = atoi(temp);
    free(temp);
}

float calculate_average_score(struct course_info *info){
    return (float) (info->teaching_score + info->clear_goal_score + info->assessment_score + info->workload_score + info->importance_score + info->satisfaction_score)/6;
}

int main(int argc, char **argv){
    char *url = URL;
    int courses_to_print = 1000;  //read this as an argument
    if(argc == 2){
        url = argv[1];
    }

    //init buffer for curling
    struct recvbuf buf;
    if(init_recvbuf(&buf, BUF_START_SIZE) != 0){
        crash("init_recvbuf");
    }

    //init curl
    if(curl_global_init(CURL_GLOBAL_DEFAULT) != 0){
        crash("curl_global_init");
    }

    //create easy handle
    CURL *easy_handle = curl_easy_init();
    if(easy_handle == NULL){
        curl_global_cleanup();
        crash("curl_easy_init");
    }
    
    int number_found_urls;
    char **course_urls = find_urls("page.html", &number_found_urls);

    struct course_info *infos = malloc(number_found_urls * sizeof(struct course_info));

    //set options for handle
    init_easy_handle(easy_handle, NULL, &buf);

    for(int i = 0; i < number_found_urls; ++i){
        curl_easy_setopt(easy_handle, CURLOPT_URL, course_urls[i]);

        CURLcode res = curl_easy_perform(easy_handle);
        if(res != CURLE_OK){
            curl_global_cleanup();
            fprintf(stderr, "CURLcode %u: %s\n", res, curl_easy_strerror(res));
            crash("curl_easy_perform");
        }

        buf.buf[buf.current_size] = '\0';

        find_stuff(buf.buf, &infos[i]);

        infos[i].average_score = calculate_average_score(&infos[i]);

        free(course_urls[i]);

        reset_recvbuf(&buf);
    }
    
    //filter relevant courses after course code
    //relevant courses are FMAXXX, EDAXXX, EITXXX FRTXXX, FMNXXX

    qsort(infos, number_found_urls, sizeof(*infos), course_compare);

    for(int i = 0; i < courses_to_print && i < number_found_urls; ++i){
        print_course_info(&infos[i]);
    }

    //cleanup
    for(int i = 0; i < number_found_urls; ++i){
        free(infos[i].name);
    }
    free(infos);

    curl_easy_cleanup(easy_handle);
    curl_global_cleanup();

    cleanup_recvbuf(&buf);
    free(course_urls);
}
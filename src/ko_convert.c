/*<$S>*/
//When we found tag, process it and ignore the EOL belongs to that line

#include <stdio.h>
#include <stdlib.h>

//#define DEBUG
#include "ko_convert.h"

// TAGS  /*<$F="원래 영어 문서의 상대적 경로">*/
//  $F - 원래 커널 문서의 파일명 (File Name)
//  $A - 원저자 (알려진 경우)
//  $V - 원문이 포함된 Linux Kernel Version
//  $T - 번역자 (Translator)
//  $D - 번역일 (Data Translated)
//  $R - 감수자 (Reviewer)
//  $L - 마지막 수정일 (Last Updated)
//  $H - Header를 넣고 문서 본문 시작
//  $E - 영어본문 시작
//  $K - 위 본문에 대한 변역 본문 시작
//  $* - 끝
//

enum {      /* State Definition */
  S_TAG = 0,
  S_KOREAN,
  S_ENGLISH,
  S_DONE,
  S_UNKNOWN
};

#define MAX_LENGTH      300
#define MAX_REVIEWER    20

static int proc_tag();
static int get_char(char *ch);

int Mode = S_TAG;       /* Initial State = reading TAGs */
int line = 1;

char _filename[MAX_LENGTH];
char _version[MAX_LENGTH];
char _author[MAX_LENGTH];
char _translator[MAX_LENGTH];
char _date[MAX_LENGTH];
char *_reviewer[MAX_REVIEWER];
char _last_update[MAX_LENGTH];

int num_reviewer;

int main(int argc, char *argv[])
{
    char ch;
    int i;
    
    if (argc != 1) {
        fprintf(stderr, "ko_convert [options] < infile > outfile\n");
        //fprintf(stderr, "    options: ...\n");
        return 1;
    }
    /* Running Simple Automata */
    #ifdef DEBUG
        printf("%5d: , line\n");
    #endif
    while (1) {
        if (get_char(&ch) < 0)
            return 1;
        if (ch == '/') {
            if (proc_tag() < 0)
                return 1;
            if (Mode == S_DONE)
                break;
            continue;
        }
        if (Mode == S_KOREAN)
            putchar(ch);
        /* else, ignore all input */
    }
    for (i = 0; i < num_reviewer; i++)
        free(_reviewer[i]);
    return 0;
}

int
put_header(void)
{
    int i;

    if (*_filename == '\0') {
        fprintf(stderr, "Document not specified /*<$F=""Documentation/00-INDEX"">*/\n");
        return -1;
    }
    if (*_version == '\0') {
        fprintf(stderr, "Version not specified, /*<$V=""3.7.4"">*/\n");
        return -1;
    }
    if (*_translator == '\0') {
        fprintf(stderr, "Translator not specified, /*<$T=""Minsuk Lee <email> (이 민석)"">*/\n");
        return -1;
    }
    if (*_date == '\0') {
        fprintf(stderr, "Date not specified, /*<$T=""TUE 2013 FEB 26"">*/\n");
        return -1;
    }
    printf(HEADER_STRING_EN, _filename, _version);

    printf(HEADER_STRING_KR, _filename, _version);
    if (*_author)
        printf("Authored by: %s\n", _author);
    printf("Translated by: %s\n", _translator);
    printf("Translated on: %s\n", _date);
    for (i = 0; i < num_reviewer; i++)
        printf("Reviewed by: %s\n", _reviewer[i]);
    if (*_last_update)
        printf("Last Updated on: %s\n", _last_update);
    printf("\n===\n\n");
    return 0;
}

int get_char(char *ch)
{
loop:
	*ch = getchar();
    if (*ch == EOF) {
        fprintf(stderr, "\nLine %d: Unexpected ENF-OF-FILE\n", line);
        return -1;
    }
    #ifdef DEBUG
        putchar(*ch);
    #endif
    if (*ch == '\n') {
        line++;
        #ifdef DEBUG
            printf("%5d: \n", line);
        #endif
    } else if (*ch == '\r') {
        goto loop;	/* just ignore */
    }
    return 0;
}

static int check_left_char(char target, char *str)
{
    char ch;

    if (get_char(&ch) < 0)
        return -1;
    #ifdef DEBUG
        putchar(ch);
    #endif
    if (ch != target) {
        //printf("--%s%c(target:%c)--", str, ch, target);
        if (Mode == S_KOREAN)
            printf("%s%c", str, ch);
        return 1;
    }
    return 0;
}

static int check_right_char(char target)
{
    char ch;

    if (get_char(&ch) < 0)
        return -1;
    #ifdef DEBUG
        putchar(ch);
    #endif
    if (ch != target) {
        fprintf(stderr, "\nLine %d: Tag format error\n", line);
        return -1;
    }
    return 0;
}

static int proc_tag()
{
    char ch;
    int tag, index = 0;
    char *s, *o;
    
    if (check_left_char('*', "/") != 0)
        return 0;
    if (check_left_char('<', "/*") != 0)
        return 0;
    if (check_left_char('$', "/*<") != 0)
        return 0;
    // we are here because input string is "/,*,<,$"
 
    if (get_char(&ch) < 0)
        return -1;
    if ((ch == 'H') || (ch == 'E') || (ch == 'K') || (ch == '*')) {
        switch (ch) {
          case 'H' : if (put_header() < 0)
                         return -1;
                     Mode = S_KOREAN;
                     break;
          case 'E' : Mode = S_ENGLISH; break;
          case 'K' : Mode = S_KOREAN; break;
          case '*' : Mode = S_DONE; break;
        }
        goto end_tags;
    }
    switch (ch) {
      case 'F' : s = _filename;     break;
      case 'A' : s = _author;       break;
      case 'V' : s = _version;      break;
      case 'T' : s = _translator;   break;
      case 'D' : s = _date;         break;
      case 'L' : s = _last_update;  break;
      case 'R' : 
        _reviewer[num_reviewer] = (char *)malloc(MAX_LENGTH);
        if (_reviewer[num_reviewer] == NULL) {
            fprintf(stderr, "\nLine %d: malloc failure\n", line);
            return -1;
        }        
        s = _reviewer[num_reviewer++]; 
        break;
      default :
        fprintf(stderr, "\nLine %d: Unknown Tag '%c'\n", line, ch);
        return -1;
    }
    tag = ch;
    o = s;
    if (check_right_char('=') < 0)
        return -1;
    if (check_right_char('"') < 0)
        return -1;
    while (1) {
        ch = getchar();
        #ifdef DEBUG
            putchar(ch);
        #endif
        if (ch == '"') {
            *s = 0;
            switch (tag) {
              case 'F' : 
              case 'A' : 
              case 'V' : 
              case 'T' :
              case 'D' : 
              case 'L' :
              case 'R' : 
                        break;
              default : fprintf(stderr, "\nOOOPPPPPS\n");
            }
            break;
        }
        if ((ch == '\t') || (ch == '\n') || (ch == '\r'))
            ch = ' ';
        if ((ch == ' ') && (*s == ' '))
                continue;
        *s++ = ch;
        if (index == MAX_LENGTH) {
            fprintf(stderr, "\nLine %d: string too long\n", line);
            return -1;
        }
    }
    #ifdef DEBUG
        printf("Line %d: TAG '%c' = '%s'\n", line, tag, o);
    #endif
end_tags:
    if (check_right_char('>') < 0)
        return -1;
    if (check_right_char('*') < 0)
        return -1;
    if (check_right_char('/') < 0)
        return -1;
    if (Mode != S_DONE) {
    	do
            get_char(&ch);
        while (ch != '\n');
    }
    return 0;
}

#include <pinyin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

struct py {
    pinyin_context_t* context;
    pinyin_instance_t* instance;
};

void py_init(struct py * p, char *sys_data_path, char *user_data_path);
void py_free(struct py *p);

int main()
{
    char * prefixbuf = NULL;
    //size_t prefixsize = 0;
    char * linebuf = NULL;
    size_t linesize = 0;
    ssize_t read;

    struct py p;
    py_init(&p, NULL, NULL);

    while(TRUE) {
        fprintf(stdout, "pinyin:");
        fflush(stdout);

        if ((read = getline(&linebuf, &linesize, stdin)) == -1)
            break;

        if ('\n' == linebuf[strlen(linebuf) - 1]) {
            linebuf[strlen(linebuf) - 1] = '\0';
        }

        if (strcmp(linebuf, "quit") == 0)
            break;

        pinyin_parse_more_full_pinyins(p.instance, linebuf);
        pinyin_guess_sentence_with_prefix(p.instance, "");
        pinyin_guess_full_pinyin_candidates(p.instance, 0);

        guint pinyin_len = 0;
        pinyin_get_n_pinyin(p.instance, &pinyin_len);
        printf("pinyin length: %u\n", pinyin_len);


#ifndef GROUP_LEN
#define GROUP_LEN 5
#endif
        guint group_len = GROUP_LEN;
        const char *words[GROUP_LEN];
        lookup_candidate_t *candidates[GROUP_LEN];
        guint len = 0;
        pinyin_get_n_candidate(p.instance, &len);
        if (len == 0) continue;
        guint group_num = len / group_len + ((len % group_len > 0) ? 1 : 0);

        guint i = 0;
        // operation
        char c;
#define MAX_SELECT_WORDS_NUM 256
        char *selected_words[MAX_SELECT_WORDS_NUM];
        int current_selected_word_index = 0;
        int hehe = 0;
        do {
            for (guint j = 0; j < group_len; j++) {
                guint index = i * group_len + j;
                if (index >= len) break;
                lookup_candidate_t * candidate = NULL;
                pinyin_get_candidate(p.instance, i * group_len + j, &candidate);
                candidates[j] = candidate;

                const char * word = NULL;
                pinyin_get_candidate_string(p.instance, candidate, &word);
                words[j] = word;
                printf("%u: %s\t", (int)j, word);
            }

            printf("\n");
            for (guint j = 0; j < group_len; j++) {
                const char *type_name[] = {
                    "BEST_MATCH",
                    "NORMAL",
                    "DIVIDED",
                    "RESPLIT",
                    "ZOMBIE",
                };
                lookup_candidate_type_t type;
                pinyin_get_candidate_type(p.instance, candidates[j], &type);
                printf("%u: %s\t", (int)j, type_name[type]);
            }

            printf("\noperation(+/- or digit): ");
            c = getchar();
            // strip the enter key
            if (c != '\n') getchar();
            if (c == '+' && i < group_num) {
                ++i;
            } else if (c == '-' && i > 0) {
                --i;
            } else if (isdigit(c)) {
                const char * _word = words[c - '0'];
                printf("select: %s\n", _word);
                int _word_len = strlen(_word);
                selected_words[current_selected_word_index] = (char *)malloc(_word_len + 1); 
                if (selected_words[current_selected_word_index] == NULL) {
                    perror("cannot alloc memory for selected word!");
                    return 1;
                }
                strcpy(selected_words[current_selected_word_index], _word);
                current_selected_word_index++;
                hehe = pinyin_choose_candidate(p.instance, hehe, candidates[(int)(c - '0')]);
                printf("hehe: %d\n", hehe);
                lookup_candidate_type_t type;
                pinyin_get_candidate_type(p.instance, candidates[(int)(c - '0')], &type);

                if (type == 0) {
                }

                if (hehe >= (int)pinyin_len) {
                    printf("all select: ");
                    for (int i = 0; i < current_selected_word_index; i++) {
                        printf("%s", selected_words[i]);
                        free(selected_words[i]);
                    }
                    printf("\n");
                    current_selected_word_index = 0;
                    break;
                };
                pinyin_guess_sentence_with_prefix(p.instance, _word);
                pinyin_guess_full_pinyin_candidates(p.instance, hehe);
            } else if (c == ' ') {
                printf("select: %s\n", words[0]);
            }
        } while (isdigit(c) || c == '+' || c == '-');

        //pinyin_train(p.instance);
        pinyin_reset(p.instance);
        //pinyin_save(p.context);
    }

    py_free(&p);
    free(prefixbuf);
    free(linebuf);
    return 0;
}

void py_init(struct py * p, char *sys_data_path, char *user_data_path)
{
    if (!sys_data_path) sys_data_path = (char *)"/usr/lib/libpinyin/data";
    if (!user_data_path) user_data_path = (char *)"./data";
    p->context = pinyin_init("/usr/lib/libpinyin/data", "./data");

    pinyin_option_t options = PINYIN_CORRECT_ALL | USE_DIVIDED_TABLE
        | USE_RESPLIT_TABLE | DYNAMIC_ADJUST;
    pinyin_set_options(p->context, options);
    p->instance = pinyin_alloc_instance(p->context);
}

void py_free(struct py *p)
{
    pinyin_free_instance(p->instance);

    pinyin_mask_out(p->context, 0x0, 0x0);
    pinyin_save(p->context);
    pinyin_fini(p->context);
}

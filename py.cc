#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "py.h"

int main()
{
    char py_buf[X_PY_BUF_LEN];

    // init libpinyin
    struct py p;
    const char * sys_data_path = " /usr/lib/libpinyin/data";
    const char * usr_data_path = "./data";
    lookup_candidate_t *candidates[PY_CANDIDATE_PAGER_LEN];
    for (int i = 0; i < PY_CANDIDATE_PAGER_LEN; i++) {
        candidates[i] = NULL;
    }
    const char * candidate_string = NULL;
    lookup_candidate_type_t candidate_type;
    const char *candidate_type_name[] = {
        "BEST_MATCH",
        "NORMAL",
        "DIVIDED",
        "RESPLIT",
        "ZOMBIE",
    };

    py_init(&p, sys_data_path, usr_data_path);

    // read input pinyin and instruction
    while (true) {
        printf("pinyin: ");
        fgets(py_buf, X_PY_BUF_LEN, stdin);
        py_clean_pinyin(py_buf);
        if (strcmp(py_buf, "bye") == 0) break;
        printf("pinyin inputed: %s\n", py_buf);

        pinyin_parse_more_full_pinyins(p.instance, py_buf);
        pinyin_guess_sentence_with_prefix(p.instance, "");
        pinyin_guess_full_pinyin_candidates(p.instance, 0);

        // get pinyin token length
        unsigned int py_pinyin_len = 0;
        unsigned int py_pinyin_offset = 0;
        pinyin_get_n_pinyin(p.instance, &py_pinyin_len);
        printf("pinyin length: %d\n", py_pinyin_len);

        int candidate_pager_index = 0;
        int candidate_pager_num = 0;
        py_get_candidates_pager_num(&p, &candidate_pager_num);
        do {
            // display first page of candidates
            py_get_candidates_by_pager(&p, candidate_pager_index, candidates);
            for (int i = 0; i < PY_CANDIDATE_PAGER_LEN; i++) {
                if (candidates[i] == NULL) break;
                pinyin_get_candidate_string(p.instance,
                        candidates[i], &candidate_string);
                printf("%d: %s\t", i, candidate_string);
            }
            // read user selection
            printf("\noperation: ");
            fgets(py_buf, X_PY_BUF_LEN, stdin);
            char operation = py_get_operation(py_buf);
            if (operation == '+') { // page forward
                if (candidate_pager_index < candidate_pager_num - 1)
                    candidate_pager_index++;
            } else if (operation == '-') { // page backword
                if (candidate_pager_index > 0) candidate_pager_index--;
            } else if (isdigit(operation)) { // select a candidate
                int _offset = (int) (operation - '0');
                if (_offset > PY_CANDIDATE_PAGER_LEN)
                    _offset = PY_CANDIDATE_PAGER_LEN - 1;

                pinyin_get_candidate_string(p.instance,
                        candidates[_offset], &candidate_string);
                pinyin_get_candidate_type(p.instance,
                        candidates[_offset], &candidate_type);
                py_pinyin_offset = pinyin_choose_candidate(p.instance,
                        py_pinyin_offset, candidates[_offset]);

                pinyin_get_n_pinyin(p.instance, &py_pinyin_len);
                printf("select: %s\ttype: %s\toffset/len: %u/%u\n",
                        candidate_string,
                        candidate_type_name[candidate_type - 1],
                        py_pinyin_offset,
                        py_pinyin_len);

                if (candidate_type == BEST_MATCH_CANDIDATE) {
                    py_pinyin_offset += g_utf8_strlen(candidate_string, -1);
                }
                if (py_pinyin_len == py_pinyin_offset) break;

                pinyin_guess_sentence_with_prefix(p.instance,
                        candidate_string);
                pinyin_guess_full_pinyin_candidates(p.instance,
                        py_pinyin_offset);
            } else {
                goto NO_TRAIN;
            }
        } while (true);
        // training
        pinyin_train(p.instance);
NO_TRAIN:
        pinyin_reset(p.instance);
        pinyin_save(p.context);
    }

    // clear libpinyin
    py_free(&p);
    printf("bye\n");

    return 0;
}

void py_clean_pinyin(char * pinyin)
{
    char * first = pinyin;
    char * second = pinyin;

    while (*first) {
        if (isalpha(*first)) {
            *second++ = *first;
        }
        first++;
    }
    *second = '\0';
}

char py_get_operation(char * input)
{
    char * ptr = input;
    for (; *ptr; ptr++) {
        if (!isblank(*ptr) && !isalpha(*ptr)) break;
    }

    return *ptr;
}

void py_init(struct py * p, const char *sys_data_path, const char *user_data_path)
{
    if (!sys_data_path) sys_data_path = (char *)"/usr/lib/libpinyin/data";
    if (!user_data_path) user_data_path = (char *)"./data";
    p->context = pinyin_init("/usr/lib/libpinyin/data", "./data");

    pinyin_option_t options = PINYIN_CORRECT_ALL | USE_DIVIDED_TABLE
        | USE_RESPLIT_TABLE | DYNAMIC_ADJUST;
    pinyin_set_options(p->context, options);
    p->instance = pinyin_alloc_instance(p->context);
}

void py_get_candidates_pager_num(struct py *p, int *num)
{
    unsigned int candidate_len = 0;
    pinyin_get_n_candidate(p->instance, &candidate_len);

    *num = candidate_len / PY_CANDIDATE_PAGER_LEN
        + ((candidate_len % PY_CANDIDATE_PAGER_LEN) ? 1 : 0);
}

void py_get_candidates_by_pager(struct py *p,
        int candidate_pager_offset, lookup_candidate_t *candidates[])
{
    int pager_num;
    py_get_candidates_pager_num(p, &pager_num);
    if (candidate_pager_offset < 0) candidate_pager_offset = 0;
    if (candidate_pager_offset >= pager_num)
        candidate_pager_offset = pager_num - 1;

    int pager_base = candidate_pager_offset * PY_CANDIDATE_PAGER_LEN;
    for (int i = 0; i < PY_CANDIDATE_PAGER_LEN; i++) {
        unsigned int index = (unsigned int)(pager_base + i);
        pinyin_get_candidate(p->instance, index, &(candidates[i]));
    }
}

void py_free(struct py *p)
{
    pinyin_free_instance(p->instance);

    pinyin_mask_out(p->context, 0x0, 0x0);
    pinyin_save(p->context);
    pinyin_fini(p->context);
}

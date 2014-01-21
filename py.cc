#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "py.h"

int main(int argc, char *argv[])
{
    //char py_buf[X_PY_BUF_LEN];
    char *py_buf;

    struct py_selected_word_node selected_word_list;
    selected_word_list.word = NULL;
    selected_word_list.next = NULL;
    struct py_selected_word_node * current_selected_word_node = &selected_word_list;

    if (argc != 2 || strlen(argv[1]) == 0) {
        perror("Usage: py pinyin");
        return -1;
    }
    py_buf = argv[1];

    // init libpinyin
    struct py p;
    const char * sys_data_path = " /opt/libpinyin/lib/libpinyin/data";
    const char * usr_data_path = "./data";
    lookup_candidate_t *candidates[PY_CANDIDATE_PAGER_LEN];
    for (int i = 0; i < PY_CANDIDATE_PAGER_LEN; i++) {
        candidates[i] = NULL;
    }
    const char * candidate_string = NULL;

    py_init(&p, sys_data_path, usr_data_path);

    // read input pinyin and instruction
    py_clean_pinyin(py_buf);

    pinyin_parse_more_full_pinyins(p.instance, py_buf);
    pinyin_guess_sentence_with_prefix(p.instance, "");
    pinyin_guess_full_pinyin_candidates(p.instance, 0);

    // get pinyin token length
    unsigned int py_pinyin_len = 0;
    unsigned int py_pinyin_offset = 0;

    int candidate_pager_index = 0;
    int candidate_pager_num = 0;
    py_get_candidates_pager_num(&p, &candidate_pager_num);
    do {
        unsigned int candidate_len = 0;
        pinyin_get_n_candidate(p.instance, &candidate_len);
        // for invalid pinyin
        if (candidate_len == 0) break;
        // display first page of candidates
        py_get_candidates_by_pager(&p, candidate_pager_index, candidates);
        for (int i = 0; i < PY_CANDIDATE_PAGER_LEN; i++) {
            if (candidates[i] == NULL) break;
            pinyin_get_candidate_string(p.instance,
                    candidates[i], &candidate_string);
            printf("%d: %s\t", i, candidate_string);
        }
        // read user selection
        fgets(py_buf, X_PY_BUF_LEN, stdin);
        char operation = py_get_operation(py_buf);
        if (operation == '+' || operation == '=' || operation == ']') {
            if (candidate_pager_index < candidate_pager_num - 1)
                candidate_pager_index++;
        } else if (operation == '-' || operation == '[') {
            if (candidate_pager_index > 0) candidate_pager_index--;
        } else if (isdigit(operation)) { // select a candidate
            int _offset = (int) (operation - '0');
            if (_offset > PY_CANDIDATE_PAGER_LEN)
                _offset = PY_CANDIDATE_PAGER_LEN - 1;

            pinyin_get_candidate_string(p.instance,
                    candidates[_offset], &candidate_string);
            py_pinyin_offset = pinyin_choose_candidate(p.instance,
                    py_pinyin_offset, candidates[_offset]);
            char *_word = (char *)malloc(sizeof(char) * strlen(candidate_string));
            if (_word == NULL) {
                perror("malloc failed for _word");
                return -1;
            }
            strcpy(_word, candidate_string);
            struct py_selected_word_node * _node = new (struct py_selected_word_node);
            if (_node == NULL) {
                perror("malloc failed for _node");
                return -1;
            }
            _node->word = _word;
            _node->next = NULL;
            current_selected_word_node->next = _node;

            current_selected_word_node = current_selected_word_node->next;
            pinyin_guess_sentence_with_prefix(p.instance, candidate_string);
            pinyin_guess_full_pinyin_candidates(p.instance, py_pinyin_offset);

            pinyin_get_n_pinyin(p.instance, &py_pinyin_len);
            if (py_pinyin_len == py_pinyin_offset) {
                pinyin_train(p.instance);
                break;
            }
        } else break;
    } while (true);
    // training
    pinyin_reset(p.instance);
    pinyin_save(p.context);

    // clear libpinyin
    py_free(&p);

    current_selected_word_node = &selected_word_list;
    while (current_selected_word_node->next != NULL) {
        current_selected_word_node = current_selected_word_node->next;
        printf("%s ", current_selected_word_node->word);
    }
    printf("\n");

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

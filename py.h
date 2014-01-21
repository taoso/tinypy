#include <pinyin.h>

#define X_PY_BUF_LEN 256
#define PY_CANDIDATE_PAGER_LEN 5

#define SYS_DATA_PATH "/opt/libpinyin/lib/libpinyin/data";
#define USR_DATA_PATH "./data";

struct py {
    pinyin_context_t* context;
    pinyin_instance_t* instance;
};

struct py_selected_word_node {
    const char * word;
    struct py_selected_word_node * next;
};

void py_clean_pinyin(char * pinyin);
char py_get_operation(char * input);

void py_init(struct py * p, const char *sys_data_path, const char *user_data_path);
void py_get_candidates_pager_num(struct py *p, int *num);
void py_get_candidates_by_pager(struct py *p,
        int candidate_pager_index, lookup_candidate_t *candidates[]);
void py_free(struct py *p);

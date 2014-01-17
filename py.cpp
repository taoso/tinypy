#include <pinyin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char * argv[]){
    pinyin_context_t * context =
        pinyin_init("/usr/lib/libpinyin/data", "./data");

    pinyin_option_t options =
        PINYIN_CORRECT_ALL | USE_DIVIDED_TABLE | USE_RESPLIT_TABLE |
        DYNAMIC_ADJUST;
    pinyin_set_options(context, options);

    pinyin_instance_t * instance = pinyin_alloc_instance(context);

    char * prefixbuf = NULL;
    size_t prefixsize = 0;
    char * linebuf = NULL;
    size_t linesize = 0;
    ssize_t read;

    while(TRUE) {
        //if ( '\n' == prefixbuf[strlen(prefixbuf) - 1] ) {
        //    prefixbuf[strlen(prefixbuf) - 1] = '\0';
        //}
        fprintf(stdout, "pinyin:");
        fflush(stdout);

        if ((read = getline(&linebuf, &linesize, stdin)) == -1)
            break;

        if ('\n' == linebuf[strlen(linebuf) - 1]) {
            linebuf[strlen(linebuf) - 1] = '\0';
        }

        if (strcmp(linebuf, "quit") == 0)
            break;

        pinyin_parse_more_full_pinyins(instance, linebuf);
        //pinyin_guess_sentence_with_prefix(instance, prefixbuf);
        pinyin_guess_full_pinyin_candidates(instance, 0);

        guint len = 0;
        guint group_len = 5;
        const char *words[5];
        guint word_index = 0;
        pinyin_get_n_candidate(instance, &len);
        if (len == 0) continue;
        guint group_num = len / group_len + ((len % group_len > 0) ? 1 : 0);

        guint i = 0;
        // operation
        char c;
        do {
            for (guint j = 0; j < group_len; j++) {
                guint index = i * group_len + j;
                if (index >= len) break;
                lookup_candidate_t * candidate = NULL;
                pinyin_get_candidate(instance, i * group_len + j, &candidate);
                const char * word = NULL;
                pinyin_get_candidate_string(instance, candidate, &word);
                words[j] = word;
                printf("%u: %s\t", (int)j, word);
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
                printf("select: %s\n", words[(int)(c - '0')]);
            }
        } while (isdigit(c) || c == '+' || c == '-');

        pinyin_train(instance);
        pinyin_reset(instance);
        pinyin_save(context);
    }

    pinyin_free_instance(instance);

    pinyin_mask_out(context, 0x0, 0x0);
    pinyin_save(context);
    pinyin_fini(context);

    free(prefixbuf); free(linebuf);
    return 0;
}

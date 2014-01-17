#include <pinyin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char * argv[]){
    pinyin_context_t * context =
        pinyin_init("/usr/lib/libpinyin/data", "./data");

    pinyin_option_t options =
        PINYIN_CORRECT_ALL | USE_DIVIDED_TABLE | USE_RESPLIT_TABLE |
        DYNAMIC_ADJUST;
    pinyin_set_options(context, options);

    pinyin_instance_t * instance = pinyin_alloc_instance(context);

    char * prefixbuf = NULL; size_t prefixsize = 0;
    char * linebuf = NULL; size_t linesize = 0;
    ssize_t read;

    while( TRUE ){
        fprintf(stdout, "prefix:");
        fflush(stdout);

        if ((read = getline(&prefixbuf, &prefixsize, stdin)) == -1)
            break;

        if ( '\n' == prefixbuf[strlen(prefixbuf) - 1] ) {
            prefixbuf[strlen(prefixbuf) - 1] = '\0';
        }

        fprintf(stdout, "pinyin:");
        fflush(stdout);

        if ((read = getline(&linebuf, &linesize, stdin)) == -1)
            break;

        if ( '\n' == linebuf[strlen(linebuf) - 1] ) {
            linebuf[strlen(linebuf) - 1] = '\0';
        }

        if ( strcmp ( linebuf, "quit" ) == 0)
            break;

        pinyin_parse_more_full_pinyins(instance, linebuf);
        pinyin_guess_sentence_with_prefix(instance, prefixbuf);
        pinyin_guess_full_pinyin_candidates(instance, 0);

        guint len = 0;
        pinyin_get_n_candidate(instance, &len);
        for (size_t i = 0; i < len; ++i) {
            lookup_candidate_t * candidate = NULL;
            pinyin_get_candidate(instance, i, &candidate);

            const char * word = NULL;
            pinyin_get_candidate_string(instance, candidate, &word);

            printf("%d:%s\t", (int)i, word);
        }
        printf("\n");

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

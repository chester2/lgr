#include <assert.h>

#include "recordlist.h"
#include "recordtree.h"
#include "t_framework.h"
#include "t_refrecs.h"

// void test_init(void);
void test_print(void);

int main(int argc, char** argv)
{
    TFWK_LOG = (argc > 1 && STR_EQ(argv[1], "-s"));

    // file setup
    FILE* f = ref_mkfile();
    rl_init(f);
    assert(rt_init());

    // test_init();
    test_print();

    // teardown
    rl_deinit();
    rt_deinit();
    ref_rmfile(f);
}

void test_print(void)
{
    log_intro("print");
    FILE* f = TFWK_LOG ? stdout : fopen("/dev/null", "w");
    assert(rt_print(f) == 27);
    fclose(f);
    log_end();
}

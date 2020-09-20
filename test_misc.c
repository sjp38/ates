#include "ates.h"
#include "misc.h"

int test_sum(void)
{
	ates_fail_if(sum(1, 2) != 3, "sum fail");

	return 0;
}

int main(int argc, char *argv[])
{
	struct ates_test *tests;

	tests = (struct ates_test []) {
		{.fn = test_sum, .name = "test sum", .is_passfail = 1},
		{}
	};
	return ates_run_tests(tests);
}

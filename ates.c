/*
 * ates: ates is _A Tes_t Envorinment Setup module
 */

#include <stdlib.h>
#include <unistd.h>

#include "ates.h"

/* a clock */

/*
 * Reference [1] for assembly usage, [2] for architecture predefinition
 *
 * [1] https://www.mcs.anl.gov/~kazutomo/rdtsc.html
 * [2] https://sourceforge.net/p/predef/wiki/Architectures/
 */
#if defined(__i386__)
#define ACLK_HW_CLOCK
static __inline__ unsigned long long aclk_clock(void)
{
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
	return x;
}

#elif defined(__x86_64__)
#define ACLK_HW_CLOCK
static __inline__ unsigned long long aclk_clock(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#elif defined(__powerpc__)
#define ACLK_HW_CLOCK
static __inline__ unsigned long long aclk_clock(void)
{
	unsigned long long int result=0;
	unsigned long int upper, lower,tmp;
	__asm__ volatile(
			"0:                  \n"
			"\tmftbu   %0           \n"
			"\tmftb    %1           \n"
			"\tmftbu   %2           \n"
			"\tcmpw    %2,%0        \n"
			"\tbne     0b         \n"
			: "=r"(upper),"=r"(lower),"=r"(tmp)
			);
	result = upper;
	result = result<<32;
	result = result|lower;

	return(result);
}

#else
static inline unsigned long long aclk_clock(void)
{
	return (unsigned long long)clock();
}

#endif

/*
 * aclk_freq - return clock frequency
 *
 * This function get cpu frequency in two ways.  If aclk does not support
 * hardware rdtsc instruction for the architecture this code is running, it
 * fallbacks to CLOCKS_PER_SEC.  Otherwise, it calculates the frequency by
 * measuring hardware rdtsc before and after sleep of 0.1 second.
 */
static inline unsigned long long aclk_freq(void)
{
/*
 * clock() cannot used with sleep. Refer to [1] for more information
 * [1] http://cboard.cprogramming.com/linux-programming/91589-using-clock-sleep.html
 */
#ifndef ACLK_HW_CLOCK
	return CLOCKS_PER_SEC;
#endif
	unsigned long long start;

	start = aclk_clock();
	usleep(1000 * 100);
	return (aclk_clock() - start) * 10;
}

/*
 * Normally expected usage of ates for performance test is,
 * ates_pr_csv_startmark() followed by ates_pr_ops_csv(), and multiple pairs of
 * ates_measure_latency_start(), ates_measure_pr_ops_csv(), and finally
 * ates_pr_csv_endmark().  It results to both human and machine readable, clean
 * output.
 */

#define ATES_CSV_START_MARK	"___ates_csvstart___"
#define ATES_CSV_END_MARK	"___ates_csvend___"

static unsigned long long cpu_freq;

/* Only ates_measure_latency_(start|end) should access. */
static unsigned long long latency_start;

/**
 * Note that this function is not thread safe!
 */
void ates_measure_latency_start(void)
{
	if (cpu_freq == 0) {
		fprintf(stderr, "test should run with run_tests\n");
		exit(1);
	}

	latency_start = aclk_clock();
}

/**
 * Note that this function is not thread safe!
 */
double ates_measure_latency_end(void)
{
	return (double)(aclk_clock() - latency_start) / cpu_freq;
}

/**
 * Calculates operations per second
 */
double ates_calc_ops(double latency, unsigned nr_ops)
{
	return nr_ops / latency;
}

/**
 * Should be paired with ates_measure_latency_start()
 */
double ates_measure_ops(unsigned nr_ops)
{
	return ates_calc_ops(ates_measure_latency_end(), nr_ops);
}

/**
 * Should be paired with ates_measure_latency_start()
 */
void ates_measure_pr_ops_csv(unsigned nr_ops, char *prefix, char *suffix)
{
	printf("%s%u,\t %.2lf%s\n",
			prefix, nr_ops, ates_measure_ops(nr_ops), suffix);
}

void ates_pr_ops_csv_legend_with(char *prefix, char *suffix)
{
	printf("%snr_ops,\t ops per sec%s\n", prefix, suffix);
}

void ates_pr_csv_startmark(char *title)
{
	printf(ATES_CSV_START_MARK", %s\n", title);
}

void ates_pr_csv_endmark(void)
{
	printf(ATES_CSV_END_MARK"\n");
}

/**
 * ates_append_test - append a test to an array of tests
 *
 * @tests	Array of test that has tests.
 * @sz_tests	Size of the array.
 * @test	Pointer to a test that will be appended.
 *
 * This function creates a new array and copy tests of @tests and append @test
 * in it and return a pointer to the new array.  Note that it does not append
 * in the original array.
 *
 * Returns new array with appended test.
 */
struct ates_test *ates_append_test(struct ates_test *tests, int sz_tests,
					struct ates_test *test)
{
	struct ates_test *new;
	int i;

	new = (struct ates_test *)malloc(sizeof(struct ates_test) *
					(sz_tests + 1));
	for (i = 0; i < sz_tests; i++) {
		new[i] = tests[i];
	}
	new[i] = *test;

	return new;
}

/*
 * run_tests - run multiple tests
 *
 * @tests	Array of tests.
 * @nr_tests	Number of tests in @tests.
 * @do_pf	Do pass/fail tests.
 * @do_nonpf	Do non-pass/fail tests.
 *
 * Returns 0 if whole pass/fail tests passed, non-zero else.
 */
int run_tests(struct ates_test tests[], int nr_tests,
		char do_pf, char do_nonpf)
{
	struct ates_test *test;
	int i;
	int result;
	int nr_passed;
	int nr_executed;
	unsigned long long start_time;
	double exec_time;

	nr_passed = 0;
	nr_executed = 0;
	for (i = 0; i < nr_tests; i++) {
		test = &tests[i];
		if (test->is_passfail && !do_pf)
			continue;
		if (!test->is_passfail && !do_nonpf)
			continue;
		start_time = aclk_clock();
		result = test->fn();
		exec_time = (aclk_clock() - start_time) / cpu_freq;

		if (test->is_passfail && result) {
			printf("%s[ATES] \"%s\" failed%s\n",
					KRED, test->name, KNRM);
			return result;
		}
		printf("%s[ATES] test \"%s\" %s in %.3lf second%s\n",
				KGRN, test->name,
				(test->is_passfail ? "passed" : "executed"),
				exec_time, KNRM);
		if (test->is_passfail)
			nr_passed++;
		nr_executed++;
	}
	printf("%s[ATES] total: %d passed, %d executed%s\n",
			KGRN, nr_passed, nr_executed, KNRM);
	return 0;
}

static char DO_CORRECT_TEST = 1;
static char DO_PERF_TEST = 1;

/*
 * ates_run_tests - run multiple tests
 *
 * @tests	Array of tests.
 * @nr_tests	Number of tests in @tests.
 *
 * Returns 0 if whole pass/fail tests passed, non-zero else.
 */
int ates_run_tests(struct ates_test tests[], int nr_tests)
{
	if (cpu_freq == 0)
		cpu_freq = aclk_freq();
	return run_tests(tests, nr_tests, DO_CORRECT_TEST, DO_PERF_TEST);
}

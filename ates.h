#ifndef _ATEST_H
#define _ATEST_H

#include <stdio.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

struct ates_test {
	int (*fn)(void);
	char *name;
	char is_passfail;
};

#define pr_fail_msg(...)						\
	do {								\
		printf("%s[fail]%s %s: %s(line %d)\n",			\
				KRED, KYEL,				\
				__FILE__, __func__, __LINE__);		\
		printf("\t");						\
		printf(__VA_ARGS__);					\
		printf("%s", KNRM);					\
	} while (0);

#define ates_fail_if(cond, ...)						\
	do {								\
		if (!(cond))						\
			break;						\
		printf("%s[fail]%s %s: %s(line %d)\n",			\
				KRED, KYEL,				\
				__FILE__, __func__, __LINE__);		\
		printf("\t");						\
		printf(__VA_ARGS__);					\
		printf("%s", KNRM);					\
		return 1;						\
	} while (0);

void ates_measure_latency_start();
double ates_measure_latency_end();
double ates_calc_ops(double latency, unsigned nr_ops);
double ates_measure_ops(unsigned nr_ops);
void ates_measure_pr_ops_csv(unsigned nr_ops, char *prefix, char *suffix);
void ates_pr_ops_csv_legend_with(char *prefix, char *suffix);
void ates_pr_csv_startmark(char *title);
void ates_pr_csv_endmark(void);

struct ates_test *ates_append_test(struct ates_test *tests, int sz_tests,
					struct ates_test *test);

int run_tests(struct ates_test tests[],
		char do_pf, char do_nonpf);
int ates_run_tests(struct ates_test tests[]);

#endif

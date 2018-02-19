/*
 * Realtek Semiconductor Corp.
 *
 * Viller Hsiao (villerhsiao@realtek.com)
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * test counts
 */
#define NTEST 7
#define CAL_P(x) (((x) << 1 ) + 1)
#define CAL_C(x) (((x) << 1))


/*
 * main routine
 */
int main(int argc, void **argv)
{
	pid_t child;
	unsigned int old;
	unsigned int val;
	int status = 0;

	child = fork();

	if (child != 0) {
		unsigned int i;
		unsigned int vparent[NTEST];
		/*
		 * parent
		 */
		asm volatile (
			"mfru	%[v], $24\n"
			: [v] "=&r" (val)
			:: "memory");
		printf("parent default is: 0x%x\n", val);

		for(i = 0; i < NTEST; i++) {
			/* write radiax */
			val = CAL_P(i);
			asm volatile (
				"mtru	%[v], $24\n"
				:: [v] "r" (val)
				: "memory");

			usleep (10000);

			/* read radiax */
			asm volatile (
				"mfru	%[v], $24\n"
				: [v] "=&r" (val)
				:: "memory");

			/* store */
			vparent[i] = val;
		}

		while (waitpid (child, &status, 0) < 0) {
			if (errno != EINTR) {
				status = -1;
				break;
			}
		}

		printf("----\nparent value is:\n");
		for(i = 0; i < NTEST; i++) {
			printf(" 0x%x,", vparent[i]);
		}
		printf("\nIt should be:\n");
		for(i = 0; i < NTEST; i++) {
			printf(" 0x%x,", CAL_P(i));
		}

		printf("\n----\n");

	}
	else {
		unsigned int i;
		unsigned int vchild[NTEST];
		/*
		 * child
		 */
		asm volatile (
			"mfru	%[v], $24\n"
			: [v] "=&r" (val)
			:: "memory");
		printf("child default is: 0x%x\n", val);

		for(i = 0; i < NTEST; i++) {
			/* write radiax */
			val = CAL_C(i);
			asm volatile (
				"mtru	%[v], $24\n"
				:: [v] "r" (val)
				: "memory");

			usleep (10000);

			/* read radiax */
			asm volatile (
				"mfru	%[v], $24\n"
				: [v] "=&r" (val)
				:: "memory");

			/* store */
			vchild[i] = val;
		}

		printf("----\nchild value is:\n");
		for(i = 0; i < NTEST; i++) {
			printf(" 0x%x,", vchild[i]);
		}
		printf("\nIt should be:\n");
		for(i = 0; i < NTEST; i++) {
			printf(" 0x%x,", CAL_C(i));
		}

		printf("\n");

	}

out:
	return status;
}

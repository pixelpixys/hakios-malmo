/*
 * A client of the first real iOS services: each call below is answered by a
 * daemon that launchd starts on demand.
 *   notifyd   - Darwin notifications (libnotify)
 *   cfprefsd  - preferences (CoreFoundation)
 *   logd      - unified logging (os_log)
 */
#include <CoreFoundation/CoreFoundation.h>
#include <notify.h>
#include <os/log.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int failures;

static void
check(int ok, const char *what)
{
	failures += !ok;
	printf("[%s] %s\n", ok ? " ok " : "FAIL", what);
	fflush(stdout);
}

int
main(void)
{
	int token = -1, changed = 0;
	uint32_t status;
	uint64_t state = 0;

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("svc_test: pid %d, uid %d\n", getpid(), getuid());

	/* notifyd */
	status = notify_register_check("com.hakios.test", &token);
	check(status == NOTIFY_STATUS_OK, "notifyd: register for a notification");
	if (status == NOTIFY_STATUS_OK) {
		notify_check(token, &changed);          /* the first check always reports a change */
		status = notify_set_state(token, 0x1234abcd);
		check(status == NOTIFY_STATUS_OK && notify_get_state(token, &state) == NOTIFY_STATUS_OK && state == 0x1234abcd,
		    "notifyd: store and fetch a state value");
		changed = 0;
		status = notify_post("com.hakios.test");
		for (int i = 0; i < 100; i++) {
			if (notify_check(token, &changed) == NOTIFY_STATUS_OK && changed) {
				break;
			}
			sched_yield();
		}
		check(status == NOTIFY_STATUS_OK && changed,
		    "notifyd: post a notification and see it arrive");
	} else {
		printf("  status %u\n", status);
	}

	/* cfprefsd */
	printf("[SKIP] cfprefsd: preferences sync requires full /var hierarchy\n");

	/* logd */
	os_log(OS_LOG_DEFAULT, "svc_test says hello to the unified log (pid %d)", getpid());
	check(1, "logd: os_log does not crash");

	printf("svc_test: %d failed\n", failures);
	return failures;
}

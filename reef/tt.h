// tt.h - minimal C test framework
//
// A "test framework" automates running test functions, counting pass/fail,
// and reporting results. Without one, you'd manually call each test and
// check output by eye.
//
// Key concepts:
//   TEST()      - defines a test function that gets auto-registered
//   ASSERT_*()  - checks a condition, records pass/fail
//   SETUP       - code that runs before EACH test (allocate resources)
//   TEARDOWN    - code that runs after EACH test (free resources)
//   SUITE_*     - like SETUP/TEARDOWN but runs ONCE for all tests
//   PARAMS(n)   - runs block n times with i=0..n-1 (parameterized tests)
//   --fork      - run each test in subprocess (crash isolation)
//   --seed N    - set random seed for reproducible fuzz tests
//   -f          - inject mutations to verify assertions detect failures
//
// Features:
//   - Grouped assertions: one TEST() can have many ASSERT_*() calls
//   - Auto-registration: TEST() functions are discovered automatically
//   - Setup/teardown: hooks that run before/after each test
//   - Suite fixtures: setup/teardown that run once for all tests
//   - Parameterized tests: run same test with different inputs
//   - Random seed: reproducible randomness via --seed and tt_rand()
//   - Fork isolation: optionally run each test in a subprocess (crash-safe)
//   - Concurrent execution: run N tests in parallel
//   - Filtering: run only tests matching a name
//   - Timing: measure each test's duration
//   - Self-check: -f verifies assertions can detect failures
//
// Usage:
//   #include "tt.h"
//
//   // Optional: per-test hooks (run before/after EACH test)
//   SETUP { fp = fopen("test.dat", "w"); }
//   TEARDOWN { fclose(fp); }
//
//   // Optional: per-suite hooks (run ONCE for all tests)
//   SUITE_SETUP { db = db_connect(); }
//   SUITE_TEARDOWN { db_close(db); }
//
//   TEST(math) {
//       ASSERT_EQ("addition", 2 + 2, 4);
//       ASSERT_EQ("subtract", 5 - 3, 2);
//   }
//
//   // Parameterized test
//   static int inputs[] = {4, 9, 16, 25};
//   static int expected[] = {2, 3, 4, 5};
//
//   TEST(sqrt_values) {
//       PARAMS(4) {
//           ASSERT_EQ("sqrt", (int)sqrt(inputs[i]), expected[i]);
//       }
//   }
//
//   int main(int argc, char **argv) {
//       return tt_main(argc, argv);
//   }
//
// Run:
//   ./test                  run all (in-process, fast)
//   ./test --fork           fork each test (crash isolation)
//   ./test --concurrent 4   run 4 tests in parallel (implies fork)
//   ./test --seed 12345     use specific random seed (reproducibility)
//   ./test --list           list test names
//   ./test /math            run only /math
//   ./test -f               verify assertions detect failures

#ifndef TT_H
#define TT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// =============================================================================
// Constants
// =============================================================================

#define TT_MAX_TESTS      256   // max registered tests
#define TT_MAX_CONCURRENT 64    // max parallel jobs
#define TT_PIPE_BUF       4096  // buffer for capturing child stderr

// ANSI escape codes for terminal colors.
// \033[ is the "escape sequence introducer", followed by color code + 'm'.
#define TT_GREEN "\033[32m"
#define TT_RED   "\033[31m"
#define TT_RESET "\033[0m"

// xorshift32 constants: magic bit-shift amounts that produce good randomness.
// These specific values (13, 17, 5) are from Marsaglia's xorshift paper.
#define TT_XS_A 13
#define TT_XS_B 17
#define TT_XS_C 5

// Time unit conversions.
#define TT_NS_TO_SEC  1e-9
#define TT_SEC_TO_US  1e6
#define TT_SEC_TO_MS  1e3
#define TT_MS_THRESH  0.001  // below this, show microseconds
#define TT_SEC_THRESH 1.0    // below this, show milliseconds

// =============================================================================
// Global State
// =============================================================================

static int tt_pass = 0;          // assertions that passed
static int tt_fail = 0;          // assertions that failed
static int tt_test_failed = 0;   // current test has any failure?
static int tt_param_idx = -1;    // current param index, -1 if not in PARAMS
static unsigned int tt_seed = 0; // random seed for reproducible tests
static int tt_detected = 0;      // mutations caught in self-check
static int tt_expected = 0;      // mutations expected in self-check

// =============================================================================
// Setup/Teardown Hooks
//
// "Hooks" are functions that run at specific points in the test lifecycle.
// SETUP runs before each test, TEARDOWN runs after. This is the "fixture"
// pattern: prepare resources before test, clean up after.
//
// Example: if tests need a temp file:
//   static FILE *fp;
//   SETUP { fp = fopen("test.dat", "w"); }
//   TEARDOWN { fclose(fp); }
//
// Each test starts with fresh file handle. Teardown runs even if test fails.
// =============================================================================

// hook_fn: pointer to void function with no arguments.
// Used for setup/teardown callbacks.
typedef void (*hook_fn)(void);

static hook_fn tt_setup = NULL;    // runs before each test
static hook_fn tt_teardown = NULL; // runs after each test

// SETUP { code } - defines function that runs before each test.
//
// __attribute__((constructor)) makes the registration function run
// automatically before main(). This is how tests "auto-register".
#define SETUP \
    static void tt_user_setup(void); \
    __attribute__((constructor)) static void tt_reg_setup(void) { \
        tt_setup = tt_user_setup; \
    } \
    static void tt_user_setup(void)

// TEARDOWN { code } - defines function that runs after each test.
#define TEARDOWN \
    static void tt_user_teardown(void); \
    __attribute__((constructor)) static void tt_reg_teardown(void) { \
        tt_teardown = tt_user_teardown; \
    } \
    static void tt_user_teardown(void)

// =============================================================================
// Suite Fixtures
//
// Like SETUP/TEARDOWN, but run ONCE for all tests instead of per-test.
// Use for expensive operations: database connections, loading large files,
// spawning servers.
//
// Example:
//   static Database *db;
//   SUITE_SETUP { db = db_connect("test.db"); }
//   SUITE_TEARDOWN { db_disconnect(db); }
//
// All tests share the same db connection (faster than reconnecting each time).
// =============================================================================

static hook_fn tt_suite_setup = NULL;    // runs once before all tests
static hook_fn tt_suite_teardown = NULL; // runs once after all tests

// SUITE_SETUP { code } - runs once before any tests start.
#define SUITE_SETUP \
    static void tt_user_suite_setup(void); \
    __attribute__((constructor)) static void tt_reg_suite_setup(void) { \
        tt_suite_setup = tt_user_suite_setup; \
    } \
    static void tt_user_suite_setup(void)

// SUITE_TEARDOWN { code } - runs once after all tests finish.
#define SUITE_TEARDOWN \
    static void tt_user_suite_teardown(void); \
    __attribute__((constructor)) static void tt_reg_suite_teardown(void) { \
        tt_suite_teardown = tt_user_suite_teardown; \
    } \
    static void tt_user_suite_teardown(void)

// =============================================================================
// Random Number Generator
//
// tt_rand() returns reproducible random numbers based on tt_seed. Same seed
// always produces same sequence. Useful for "fuzz tests" (tests with random
// inputs) - when a test fails, rerun with printed seed to reproduce exactly.
//
// Uses xorshift32: XOR the number with bit-shifted versions of itself.
// Fast, simple, good distribution. NOT cryptographically secure.
// =============================================================================

// tt_rand - returns next random number, advances seed.
//
// xorshift32 algorithm: each XOR+shift "mixes" the bits. The specific
// shift amounts (13, 17, 5) were chosen by Marsaglia to maximize period.
static unsigned int
tt_rand(void) {
    tt_seed ^= tt_seed << TT_XS_A;
    tt_seed ^= tt_seed >> TT_XS_B;
    tt_seed ^= tt_seed << TT_XS_C;
    return tt_seed;
}

// tt_rand_range - returns random value in [min, max] inclusive.
//
// Uses modulo to constrain range. Has slight bias for non-power-of-2 ranges,
// but acceptable for testing purposes.
static unsigned int
tt_rand_range(unsigned int min, unsigned int max) {
    if (min >= max) return min;
    return min + (tt_rand() % (max - min + 1));
}

// =============================================================================
// Predicates
//
// Small functions that answer yes/no questions. Named so code reads like prose:
//   if (streq(a, b)) ...     reads as "if a equals b"
//   if (is_filter_arg(s))    reads as "if s is a filter argument"
// =============================================================================

// doubles_equal - checks if two floats are "close enough".
//
// Floating-point math has rounding errors: 0.1 + 0.2 != 0.3 exactly.
// So instead of a == b, we check |a - b| <= epsilon (tolerance).
static int
doubles_equal(double a, double b, double eps) {
    return (a - b <= eps) && (b - a <= eps);
}

// streq - string equality. strcmp returns 0 when equal (confusing!).
static int
streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

// has_prefix - checks if string starts with prefix.
static int
has_prefix(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

// is_help_flag - user wants help?
static int
is_help_flag(const char *arg) {
    return streq(arg, "--help") || streq(arg, "-h");
}

// is_filter_arg - argument is a test filter? (starts with /)
static int
is_filter_arg(const char *arg) {
    return arg[0] == '/';
}

// matches_filter - does test name match the filter?
// NULL filter matches everything (no filtering).
static int
matches_filter(const char *name, const char *filter) {
    if (!filter) return 1;
    return streq(name, filter);
}

// in_params - currently inside a PARAMS() block?
// Used to show parameter index in failure messages: "FAIL[2]: ..."
static int
in_params(void) {
    return tt_param_idx >= 0;
}

// all_detected - did self-check catch all expected mutations?
static int
all_detected(void) {
    return tt_detected >= tt_expected;
}

// =============================================================================
// Assertion Recording
//
// When ASSERT_*() runs, it either passes or fails. These functions update
// the counters and print failure details. Failures show:
//   - assertion name (what we were checking)
//   - actual vs expected values (what went wrong)
//   - file:line (where to look)
//   - param index if in PARAMS block (which iteration failed)
// =============================================================================

// record_pass - assertion succeeded, increment counter.
static void
record_pass(void) {
    tt_pass++;
}

// mark_failed - common logic for all failures: update counters.
static void
mark_failed(void) {
    tt_fail++;
    tt_test_failed = 1;
}

// print_fail_prefix - prints "    FAIL: " or "    FAIL[i]: " for params.
static void
print_fail_prefix(const char *name) {
    if (in_params())
        fprintf(stderr, "    FAIL[%d]: %s", tt_param_idx, name);
    else
        fprintf(stderr, "    FAIL: %s", name);
}

// print_location - prints " (file.c:42)\n" suffix.
static void
print_location(const char *file, int line) {
    fprintf(stderr, " (%s:%d)\n", file, line);
}

// record_fail - generic failure, no value comparison.
static void
record_fail(const char *name, const char *file, int line) {
    mark_failed();
    print_fail_prefix(name);
    print_location(file, line);
}

// record_fail_cmp - failure with expression comparison (shows code).
static void
record_fail_cmp(const char *name, const char *a, const char *b,
                const char *file, int line) {
    mark_failed();
    print_fail_prefix(name);
    fprintf(stderr, " - %s != %s", a, b);
    print_location(file, line);
}

// record_fail_str - failure with string values (shows quoted strings).
static void
record_fail_str(const char *name, const char *a, const char *b,
                const char *file, int line) {
    mark_failed();
    print_fail_prefix(name);
    fprintf(stderr, " - \"%s\" != \"%s\"", a, b);
    print_location(file, line);
}

// record_fail_dbl - failure with double values (shows numbers).
static void
record_fail_dbl(const char *name, double a, double b,
                const char *file, int line) {
    mark_failed();
    print_fail_prefix(name);
    fprintf(stderr, " - %.6f != %.6f", a, b);
    print_location(file, line);
}

// =============================================================================
// Assertions
//
// Macros that check conditions and record pass/fail. Each takes a "name"
// argument that describes what's being tested (shown in failure output).
//
// do { ... } while (0) is a C idiom that makes the macro behave like a
// statement (can be used with if/else without braces).
//
// #a "stringifies" the argument: ASSERT_EQ("x", 2+2, 4) shows "2+2 != 4".
// =============================================================================

#define ASSERT_TRUE(name, cond) do { \
    if (cond) record_pass(); \
    else record_fail(name, __FILE__, __LINE__); \
} while (0)

#define ASSERT_FALSE(name, cond) ASSERT_TRUE(name, !(cond))

// ASSERT_EQ uses #a/#b to show the actual expressions in failure message.
#define ASSERT_EQ(name, a, b) do { \
    if ((a) == (b)) record_pass(); \
    else record_fail_cmp(name, #a, #b, __FILE__, __LINE__); \
} while (0)

#define ASSERT_NE(name, a, b) ASSERT_TRUE(name, (a) != (b))
#define ASSERT_LT(name, a, b) ASSERT_TRUE(name, (a) < (b))
#define ASSERT_LE(name, a, b) ASSERT_TRUE(name, (a) <= (b))
#define ASSERT_GT(name, a, b) ASSERT_TRUE(name, (a) > (b))
#define ASSERT_GE(name, a, b) ASSERT_TRUE(name, (a) >= (b))

#define ASSERT_NULL(name, p)     ASSERT_TRUE(name, (p) == NULL)
#define ASSERT_NOT_NULL(name, p) ASSERT_TRUE(name, (p) != NULL)

// ASSERT_STR_EQ - string comparison (quotes values in output).
#define ASSERT_STR_EQ(name, a, b) do { \
    if (streq((a), (b))) record_pass(); \
    else record_fail_str(name, a, b, __FILE__, __LINE__); \
} while (0)

// ASSERT_DOUBLE_EQ - float comparison with epsilon tolerance.
// Stores in temp vars to avoid evaluating expressions twice.
#define ASSERT_DOUBLE_EQ(name, a, b, eps) do { \
    double _a = (a), _b = (b); \
    if (doubles_equal(_a, _b, eps)) record_pass(); \
    else record_fail_dbl(name, _a, _b, __FILE__, __LINE__); \
} while (0)

// =============================================================================
// Parameterized Tests
//
// Run same test with different inputs. Instead of writing 5 separate tests
// for 5 inputs, use PARAMS(5) to loop with i=0,1,2,3,4.
//
// On failure, output shows which index failed: "FAIL[2]: ..." so you know
// which input caused the problem.
//
// Example:
//   static int inputs[] = {1, 2, 3, 4, 5};
//   static int squares[] = {1, 4, 9, 16, 25};
//
//   TEST(square) {
//       PARAMS(5) {
//           ASSERT_EQ("square", inputs[i] * inputs[i], squares[i]);
//       }
//   }
// =============================================================================

// PARAMS(n) - runs block n times with i=0..n-1.
//
// The "(tt_param_idx = i, 1)" sets the global index and evaluates to 1 (true),
// so the loop continues. On each iteration end, resets to -1 (not in params).
#define PARAMS(n) \
    for (int i = 0; i < (int)(n) && (tt_param_idx = i, 1); i++, tt_param_idx = -1)

// PARAMS_ARRAY(arr) - PARAMS with array length computed automatically.
// sizeof(arr)/sizeof(arr[0]) is the standard C idiom for array element count.
// Only works for actual arrays, not pointers (pointers don't know their size).
#define PARAMS_ARRAY(arr) PARAMS(sizeof(arr) / sizeof((arr)[0]))

// =============================================================================
// Test Registration
//
// Tests are "auto-registered": you write TEST(foo) { ... } and it's added
// to the test list automatically. No need to manually add it to main().
//
// This uses __attribute__((constructor)) - a GCC/Clang feature that runs
// a function before main(). Each TEST() macro creates a registration function
// that adds itself to the global tt_tests array.
// =============================================================================

// test_fn: pointer to void function with no arguments.
typedef void (*test_fn)(void);

// test_entry: one registered test (name + function pointer).
struct test_entry {
    const char *name;
    test_fn fn;
};

static struct test_entry tt_tests[TT_MAX_TESTS]; // all registered tests
static int tt_count = 0;                          // how many registered

// TEST(name) - defines and auto-registers a test function.
//
// test_##tname creates unique function name: TEST(foo) -> test_foo
// tt_reg_##tname creates unique registration function: tt_reg_foo
// #tname stringifies: TEST(foo) -> name = "foo"
#define TEST(tname) \
    static void test_##tname(void); \
    __attribute__((constructor)) static void tt_reg_##tname(void) { \
        if (tt_count < TT_MAX_TESTS) { \
            tt_tests[tt_count].name = #tname; \
            tt_tests[tt_count].fn = test_##tname; \
            tt_count++; \
        } \
    } \
    static void test_##tname(void)

// =============================================================================
// Timing
//
// Measures how long each test takes. Uses CLOCK_MONOTONIC which always
// increases (unlike wall clock, which can jump when system time changes).
// =============================================================================

// now_secs - returns current time in seconds (with nanosecond precision).
static double
now_secs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * TT_NS_TO_SEC;
}

// fmt_duration - formats seconds into human-readable string.
// Chooses unit (us/ms/s) based on magnitude for readability.
static void
fmt_duration(char *buf, size_t sz, double secs) {
    if (secs < TT_MS_THRESH) {
        snprintf(buf, sz, "%3.0f us", secs * TT_SEC_TO_US);
        return;
    }
    if (secs < TT_SEC_THRESH) {
        snprintf(buf, sz, "%.1f ms", secs * TT_SEC_TO_MS);
        return;
    }
    snprintf(buf, sz, "%.2f s", secs);
}

// =============================================================================
// Process Utilities
//
// When running tests with --fork, each test runs in a child process. The
// parent waits for the child and checks its exit status. These functions
// wrap the Unix wait() status macros with readable names.
//
// Why fork? If a test crashes (segfault, abort), only the child dies.
// The parent continues running other tests. Without fork, one crash kills
// the whole test run.
// =============================================================================

// drain_pipe - reads all data from pipe fd and writes to stderr.
// Used to capture child process output and display after it exits.
static void
drain_pipe(int fd) {
    char buf[TT_PIPE_BUF];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        fwrite(buf, 1, n, stderr);
    close(fd);
}

// child_signaled - was child killed by a signal (crash, SIGKILL, etc)?
static int
child_signaled(int status) {
    return WIFSIGNALED(status);
}

// child_exited - did child exit normally (called exit() or returned)?
static int
child_exited(int status) {
    return WIFEXITED(status);
}

// exit_code - what value did child pass to exit()? (only valid if exited)
static int
exit_code(int status) {
    return WEXITSTATUS(status);
}

// term_signal - what signal killed the child? (only valid if signaled)
static int
term_signal(int status) {
    return WTERMSIG(status);
}

// child_failed - did child indicate failure? (non-zero exit or signal)
static int
child_failed(int status) {
    if (child_signaled(status)) return 1;
    if (child_exited(status)) return exit_code(status) != 0;
    return 1; // unknown status = assume failure
}

// =============================================================================
// Job Management
//
// A "job" is a forked child process running one test. The parent tracks
// multiple jobs (for concurrent execution) and waits for them to finish.
//
// Communication: child writes failure messages to a pipe, parent reads
// after child exits. This prevents interleaved output from concurrent tests.
// =============================================================================

// job: tracks one forked test process.
struct job {
    pid_t pid;      // child process ID, 0 if slot is free
    int idx;        // index into tt_tests[]
    int pipe_fd;    // read end of pipe for capturing child's stderr
    double start;   // when the test started (for timing)
};

// spawn_failed_job - returns a job struct indicating spawn failure.
static struct job
spawn_failed_job(int idx) {
    struct job j = {0};
    j.pid = -1;
    j.idx = idx;
    return j;
}

// run_test_in_child - child process: run test and exit with result.
// This function never returns (calls _exit).
static void
run_test_in_child(int idx, int write_fd) {
    dup2(write_fd, STDERR_FILENO); // redirect stderr to pipe
    close(write_fd);

    tt_test_failed = 0;
    if (tt_setup) tt_setup();
    tt_tests[idx].fn();
    if (tt_teardown) tt_teardown();
    _exit(tt_test_failed); // 0 = pass, 1 = fail
}

// spawn_test - forks child to run test, returns job handle.
// Parent uses job to wait for result and read output.
static struct job
spawn_test(int idx) {
    int pfd[2];
    if (pipe(pfd) < 0) {
        perror("pipe");
        return spawn_failed_job(idx);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(pfd[0]);
        close(pfd[1]);
        return spawn_failed_job(idx);
    }

    if (pid == 0) {
        close(pfd[0]); // child doesn't read from pipe
        run_test_in_child(idx, pfd[1]);
    }

    // Parent continues here
    close(pfd[1]); // parent doesn't write to pipe

    struct job j = {0};
    j.pid = pid;
    j.idx = idx;
    j.pipe_fd = pfd[0];
    j.start = now_secs();
    return j;
}

// report_signal_death - prints message when child killed by signal.
static void
report_signal_death(int sig) {
    fprintf(stderr, "    killed by signal %d\n", sig);
}

// wait_job - waits for job to finish, returns 1 if failed.
static int
wait_job(struct job *j) {
    int status;
    waitpid(j->pid, &status, 0);
    drain_pipe(j->pipe_fd);

    if (child_signaled(status)) {
        report_signal_death(term_signal(status));
        return 1;
    }
    return child_failed(status);
}

// slot_has_pid - does this job slot contain the given pid?
static int
slot_has_pid(struct job *j, pid_t pid) {
    return j->pid == pid;
}

// slot_is_free - is this job slot available?
static int
slot_is_free(struct job *j) {
    return j->pid == 0;
}

// find_slot - finds job slot containing given pid, returns -1 if not found.
static int
find_slot(struct job *jobs, int n, pid_t pid) {
    for (int i = 0; i < n; i++) {
        if (slot_has_pid(&jobs[i], pid)) return i;
    }
    return -1;
}

// find_free_slot - finds available job slot, returns -1 if all busy.
static int
find_free_slot(struct job *jobs, int n) {
    for (int i = 0; i < n; i++) {
        if (slot_is_free(&jobs[i])) return i;
    }
    return -1;
}

// wait_any - waits for any child to finish, returns slot index.
// Sets *failed=1 if the finished test failed.
static int
wait_any(struct job *jobs, int n, int *failed) {
    int status;
    pid_t pid = waitpid(-1, &status, 0);
    if (pid < 0) return -1;

    int slot = find_slot(jobs, n, pid);
    if (slot < 0) return -1;

    drain_pipe(jobs[slot].pipe_fd);
    jobs[slot].pid = 0; // mark slot as free

    if (child_signaled(status)) {
        report_signal_death(term_signal(status));
        *failed = 1;
        return slot;
    }

    *failed = child_failed(status);
    return slot;
}

// =============================================================================
// Direct Execution (no fork)
//
// Default mode: run test in main process. Faster than fork (no process
// overhead), but a crash kills the whole test run. Good for quick iteration
// during development; use --fork for CI where crash isolation matters.
// =============================================================================

// run_direct - runs test in current process, returns 1 if failed.
static int
run_direct(int idx) {
    tt_test_failed = 0;
    if (tt_setup) tt_setup();
    tt_tests[idx].fn();
    if (tt_teardown) tt_teardown();
    return tt_test_failed;
}

// =============================================================================
// Output
//
// Functions that print test results to terminal. Uses ANSI colors for
// visual feedback: green=pass, red=fail. Results are aligned for readability.
// =============================================================================

// print_result - prints one test's result line.
// %-*s left-aligns name in field of maxlen width (for column alignment).
static void
print_result(const char *name, int maxlen, int failed, double elapsed) {
    char tbuf[32];
    fmt_duration(tbuf, sizeof(tbuf), elapsed);
    printf("/%-*s ", maxlen, name);
    if (failed)
        printf(TT_RED "[FAIL]" TT_RESET "  %s\n", tbuf);
    else
        printf(TT_GREEN "[ OK ]" TT_RESET "  %s\n", tbuf);
}

// print_usage - prints help message showing all options.
static void
print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS] [/filter]\n", prog);
    printf("  --list           list all tests\n");
    printf("  --fork           fork each test (crash isolation)\n");
    printf("  --concurrent N   run N tests in parallel (implies fork)\n");
    printf("  --seed N         set random seed (for reproducibility)\n");
    printf("  -f               verify assertions detect failures\n");
    printf("  /name            run only matching test\n");
}

// print_list - prints all test names (for --list option).
static void
print_list(void) {
    for (int i = 0; i < tt_count; i++)
        printf("/%s\n", tt_tests[i].name);
}

// print_summary - prints final stats after all tests.
static void
print_summary(int pass, int fail, int skip, double total) {
    char tbuf[32];
    fmt_duration(tbuf, sizeof(tbuf), total);
    printf("\n%d/%d tests", pass, pass + fail);
    if (skip) printf(" (%d skipped)", skip);
    printf(", %d/%d assertions", tt_pass, tt_pass + tt_fail);
    printf(" in %s\n", tbuf);
}

// max_name_len - finds longest test name (for column alignment).
// Only considers tests matching filter.
static int
max_name_len(const char *filter) {
    int maxlen = 0;
    for (int i = 0; i < tt_count; i++) {
        if (!matches_filter(tt_tests[i].name, filter)) continue;
        int len = strlen(tt_tests[i].name) + 1; // +1 for leading /
        if (len > maxlen) maxlen = len;
    }
    return maxlen;
}

// =============================================================================
// Sequential Runner
//
// Runs tests one at a time, in registration order. Simpler than concurrent:
// no job management, deterministic ordering. Good for debugging.
// =============================================================================

// spawn_failed - did the spawn attempt fail? (couldn't fork)
static int
spawn_failed(struct job *j) {
    return j->pid < 0;
}

// run_one_test - runs single test, returns 1 if failed.
static int
run_one_test(int idx, int use_fork) {
    if (!use_fork) return run_direct(idx);

    struct job j = spawn_test(idx);
    if (spawn_failed(&j)) return 1;
    return wait_job(&j);
}

// run_sequential - runs all tests sequentially.
static void
run_sequential(const char *filter, int use_fork,
               int *pass, int *fail, int *skip, double *total, int maxlen) {
    for (int i = 0; i < tt_count; i++) {
        if (!matches_filter(tt_tests[i].name, filter)) {
            (*skip)++;
            continue;
        }

        double t0 = now_secs();
        int failed = run_one_test(i, use_fork);
        double elapsed = now_secs() - t0;
        *total += elapsed;

        print_result(tt_tests[i].name, maxlen, failed, elapsed);
        if (failed) (*fail)++;
        else (*pass)++;
    }
}

// =============================================================================
// Concurrent Runner
//
// Runs multiple tests in parallel using fork(). Each test runs in its own
// process. The parent manages a pool of job slots, spawning new tests as
// slots become free. Faster on multi-core, but output order is non-deterministic.
// =============================================================================

// clamp_njobs - ensures njobs is in valid range [1, TT_MAX_CONCURRENT].
static int
clamp_njobs(int njobs) {
    if (njobs > TT_MAX_CONCURRENT) return TT_MAX_CONCURRENT;
    if (njobs < 1) return 1;
    return njobs;
}

// has_room - can we spawn another test? (running < max)
static int
has_room(int running, int njobs) {
    return running < njobs;
}

// has_more_tests - are there more tests to spawn?
static int
has_more_tests(int next) {
    return next < tt_count;
}

// all_done - no tests running and none left to spawn?
static int
all_done(int running) {
    return running == 0;
}

// run_concurrent - runs tests in parallel with njobs max concurrent.
static void
run_concurrent(const char *filter, int njobs,
               int *pass, int *fail, int *skip, double *total, int maxlen) {
    njobs = clamp_njobs(njobs);

    struct job jobs[TT_MAX_CONCURRENT] = {0};
    double starts[TT_MAX_TESTS];
    int running = 0;
    int next = 0;
    double wall_start = now_secs();

    while (1) {
        // Spawn tests until we hit max concurrent or run out
        while (has_room(running, njobs) && has_more_tests(next)) {
            if (!matches_filter(tt_tests[next].name, filter)) {
                (*skip)++;
                next++;
                continue;
            }

            int slot = find_free_slot(jobs, njobs);
            if (slot < 0) break;

            jobs[slot] = spawn_test(next);
            starts[next] = jobs[slot].start;
            if (!spawn_failed(&jobs[slot])) running++;
            next++;
        }

        if (all_done(running)) break;

        // Wait for one test to finish
        int failed;
        int slot = wait_any(jobs, njobs, &failed);
        if (slot < 0) continue;

        int idx = jobs[slot].idx;
        double elapsed = now_secs() - starts[idx];
        print_result(tt_tests[idx].name, maxlen, failed, elapsed);

        if (failed) (*fail)++;
        else (*pass)++;
        running--;
    }

    *total = now_secs() - wall_start;
}

// =============================================================================
// Argument Parsing
//
// Parses command-line arguments into an options struct. Supports two styles:
//   --option value  (separate argument)
//   --option=value  (combined)
// =============================================================================

// options: all command-line settings.
struct options {
    int use_fork;        // run each test in subprocess
    int concurrent;      // number of parallel tests (0 = sequential)
    int list_only;       // just print test names, don't run
    int self_check;      // run mutation self-check instead of tests
    unsigned int seed;   // random seed value
    int seed_set;        // was --seed explicitly provided?
    const char *filter;  // only run tests matching this name
};

// has_next_arg - is there another argument after current one?
static int
has_next_arg(int i, int argc) {
    return i + 1 < argc;
}

// parse_int_arg - parses integer from string, handles both styles.
// For "--opt val", pass argv[i+1]. For "--opt=val", pass ptr to val part.
static int
parse_int_arg(const char *s) {
    return atoi(s);
}

// parse_args - parses argv into options struct.
static struct options
parse_args(int argc, char **argv) {
    struct options opts = {0};

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        if (streq(arg, "--fork")) {
            opts.use_fork = 1;
        } else if (streq(arg, "--list")) {
            opts.list_only = 1;
        } else if (streq(arg, "-f")) {
            opts.self_check = 1;
        } else if (is_help_flag(arg)) {
            print_usage(argv[0]);
            exit(0);
        } else if (is_filter_arg(arg)) {
            opts.filter = arg + 1; // skip leading /
        } else if (streq(arg, "--concurrent") && has_next_arg(i, argc)) {
            opts.concurrent = parse_int_arg(argv[++i]);
        } else if (has_prefix(arg, "--concurrent=")) {
            opts.concurrent = parse_int_arg(arg + 13);
        } else if (streq(arg, "--seed") && has_next_arg(i, argc)) {
            opts.seed = (unsigned int)parse_int_arg(argv[++i]);
            opts.seed_set = 1;
        } else if (has_prefix(arg, "--seed=")) {
            opts.seed = (unsigned int)parse_int_arg(arg + 7);
            opts.seed_set = 1;
        }
    }

    return opts;
}

// =============================================================================
// Main Entry Point
//
// tinytest_main() is called from user's main(). It parses arguments, runs
// tests, and returns exit code (0 = all passed, 1 = some failed).
// =============================================================================

// init_seed - initializes random seed from option or time.
// Seed 0 produces poor xorshift output, so we use 1 instead.
static void
init_seed(struct options *opts) {
    if (opts->seed_set) {
        tt_seed = opts->seed ? opts->seed : 1;
    } else {
        tt_seed = (unsigned int)time(NULL);
        if (tt_seed == 0) tt_seed = 1;
    }
}

// any_failed - did any tests fail?
static int
any_failed(int fail) {
    return fail > 0;
}

// wants_concurrent - user requested parallel execution?
static int
wants_concurrent(struct options *opts) {
    return opts->concurrent > 0;
}

// =============================================================================
// Self-Check
//
// -f mode: inject mutations into data or logic and verify that your
// assertions actually catch them. Same pattern as bash-test -f.
//
// Override tt_run_check() in your test file:
//
//   void tt_run_check(void) {
//       tt_expected = 2;
//
//       // Mutation 1: inject false positive
//       del_list[del_count++] = "preserved_blob";
//       tt_check("preserved_blob_wrongly_deleted",
//                find_in_list(del_list, del_count, "preserved_blob"));
//
//       // Mutation 2: remove true positive
//       remove_from_list(del_list, &del_count, "deleted_blob");
//       tt_check("deleted_blob_missing",
//                !find_in_list(del_list, del_count, "deleted_blob"));
//   }
// =============================================================================

// tt_check - record one mutation check result.
// detected=1 if the mutation was caught, 0 if missed.
static void
tt_check(const char *name, int detected) {
    if (detected) {
        tt_detected++;
        printf(TT_GREEN "/check/%s [ OK ]\n" TT_RESET, name);
    } else {
        fprintf(stderr, TT_RED "/check/%s [FAIL]\n" TT_RESET, name);
    }
}

// tt_run_check - override in your test file to inject mutations.
// Default: no mutations (EXPECTED=0, always passes).
void __attribute__((weak))
tt_run_check(void) {
    tt_expected = 0;
}

// tt_main - runs all tests, returns exit code.
// Call this from main(): return tt_main(argc, argv);
static int
tt_main(int argc, char **argv) {
    struct options opts = parse_args(argc, argv);

    if (opts.list_only) {
        print_list();
        return 0;
    }

    if (opts.self_check) {
        tt_run_check();
        printf("\n%s -f: %s%d/%d mutations detected%s\n",
               argv[0],
               all_detected() ? TT_GREEN : TT_RED,
               tt_detected, tt_expected,
               TT_RESET);
        return all_detected() ? 0 : 1;
    }

    init_seed(&opts);
    printf("seed: %u\n\n", tt_seed);

    if (tt_suite_setup) tt_suite_setup();

    int maxlen = max_name_len(opts.filter);
    int pass = 0, fail = 0, skip = 0;
    double total = 0;

    if (wants_concurrent(&opts))
        run_concurrent(opts.filter, opts.concurrent,
                       &pass, &fail, &skip, &total, maxlen);
    else
        run_sequential(opts.filter, opts.use_fork,
                       &pass, &fail, &skip, &total, maxlen);

    if (tt_suite_teardown) tt_suite_teardown();

    print_summary(pass, fail, skip, total);
    return any_failed(fail) ? 1 : 0;
}

#endif // TT_H

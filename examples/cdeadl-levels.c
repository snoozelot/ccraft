// cdeadl-levels.c — example for cdeadl level mode
//
// Same code as cdeadl.c, but demonstrates level-based verification.
// Assign levels: Config.mu=1 (acquired first), Account.mu=2 (acquired second).
// withdraw() violates this order.
//
// Run: cdeadl --level 1 Config.mu --level 2 Account.mu examples/cdeadl-levels.c
// Expected: level violation in withdraw(): Config.mu (level 1, line 43) acquired after Account.mu (level 2, line 42)

#include <pthread.h>

struct Config {
    pthread_mutex_t mu;
    int rate;
};

struct Account {
    pthread_mutex_t mu;
    int balance;
};

struct Config g_config;
struct Account g_account;

// Correct order: Config (level 1) before Account (level 2)
void
deposit(struct Config *cfg, struct Account *acc, int amount)
{
    pthread_mutex_lock(&cfg->mu);
    pthread_mutex_lock(&acc->mu);

    acc->balance += amount * cfg->rate;

    pthread_mutex_unlock(&acc->mu);
    pthread_mutex_unlock(&cfg->mu);
}

// Wrong order: level 2 before level 1 (should be ascending)
void
withdraw(struct Account *acc, struct Config *cfg, int amount)
{
    pthread_mutex_lock(&acc->mu);   // level 2 — acquired first
    pthread_mutex_lock(&cfg->mu);   // level 1 — violation! lower level after higher

    acc->balance -= amount * cfg->rate;

    pthread_mutex_unlock(&cfg->mu);
    pthread_mutex_unlock(&acc->mu);
}

int
main(void)
{
    deposit(&g_config, &g_account, 100);
    withdraw(&g_account, &g_config, 50);
    return 0;
}

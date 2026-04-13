// cdeadl-levels.c — example for cdeadl level mode
//
// Level-based deadlock prevention: assign each lock a level, acquire in
// ascending order only. Violations detected at analysis time.
//
// Levels declared via source comments (preferred) or CLI flags.
//
// Run: cdeadl examples/cdeadl-levels.c
// Expected:
//   level violation: Account.mu vs Config.mu
//     withdraw(): Account.mu:N → Config.mu:M (level 2 → 1)

#include <pthread.h>

typedef struct {
    pthread_mutex_t mu;  // lock_level=1
    int rate;
} Config;

typedef struct {
    pthread_mutex_t mu;  // lock_level=2
    int balance;
} Account;

Config g_config;
Account g_account;

// Correct order: level 1 before level 2
void
deposit(Config *cfg, Account *acc, int amount)
{
    pthread_mutex_lock(&cfg->mu);   // level 1
    pthread_mutex_lock(&acc->mu);   // level 2 — OK, ascending

    acc->balance += amount * cfg->rate;

    pthread_mutex_unlock(&acc->mu);
    pthread_mutex_unlock(&cfg->mu);
}

// Wrong order: level 2 before level 1
void
withdraw(Account *acc, Config *cfg, int amount)
{
    pthread_mutex_lock(&acc->mu);   // level 2
    pthread_mutex_lock(&cfg->mu);   // level 1 — violation! must be ascending

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

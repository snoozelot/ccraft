// deadlock.c — example for cdeadl
//
// Deadlock patterns: 2-lock conflicts, 3-lock chain, and deep nesting.
//
// Type-based lock identity: tx->src->data->mu resolves to Data.mu regardless
// of access path depth (3 pointer dereferences here). Different variables of
// the same struct type share lock identity.
//
// Run: cdeadl examples/deadlock.c
// Expected output:
//   deadlock: Account.mu vs Config.mu
//   deadlock: Account.mu vs Wallet.mu
//   deadlock: Data.mu vs Meta.mu
//   deadlock: Account.mu → Logger.mu → Config.mu → Account.mu

#include <pthread.h>

struct Config {
    pthread_mutex_t mu;
    int rate;
};

struct Account {
    pthread_mutex_t mu;
    int balance;
};

struct Logger {
    pthread_mutex_t mu;
    int fd;
};

// Nested structures for deep access patterns
struct Wallet {
    pthread_mutex_t mu;
    int coins;
};

struct User {
    struct Wallet *wallet;
    struct Account *primary;
};

struct Bank {
    struct User *users;
    int user_count;
};

// Deep chain: tx->src->data->mu vs tx->dst->meta->mu
struct Data {
    pthread_mutex_t mu;
    int value;
};

struct Meta {
    pthread_mutex_t mu;
    int flags;
};

struct Source {
    struct Data *data;
};

struct Destination {
    struct Meta *meta;
};

struct Transaction {
    struct Source *src;
    struct Destination *dst;
};

struct Config g_config;
struct Account g_account;
struct Logger g_logger;
struct Bank g_bank;

// ============================================================================
// 2-lock deadlock: Config vs Account
// ============================================================================

void
deposit(struct Config *cfg, struct Account *acc, int amount)
{
    pthread_mutex_lock(&cfg->mu);
    pthread_mutex_lock(&acc->mu);
    acc->balance += amount * cfg->rate;
    pthread_mutex_unlock(&acc->mu);
    pthread_mutex_unlock(&cfg->mu);
}

void
withdraw(struct Account *acc, struct Config *cfg, int amount)
{
    pthread_mutex_lock(&acc->mu);
    pthread_mutex_lock(&cfg->mu);
    acc->balance -= amount * cfg->rate;
    pthread_mutex_unlock(&cfg->mu);
    pthread_mutex_unlock(&acc->mu);
}

// ============================================================================
// 3-lock chain deadlock: Account → Logger → Config → Account
//
// Thread 1: transfer()   holds Account, waits for Logger
// Thread 2: checkpoint() holds Logger, waits for Config
// Thread 3: reload()     holds Config, waits for Account
// ============================================================================

void
transfer(struct Account *acc, struct Logger *log)
{
    pthread_mutex_lock(&acc->mu);
    pthread_mutex_lock(&log->mu);
    // log transfer
    pthread_mutex_unlock(&log->mu);
    pthread_mutex_unlock(&acc->mu);
}

void
checkpoint(struct Logger *log, struct Config *cfg)
{
    pthread_mutex_lock(&log->mu);
    pthread_mutex_lock(&cfg->mu);
    // save config to log
    pthread_mutex_unlock(&cfg->mu);
    pthread_mutex_unlock(&log->mu);
}

void
reload(struct Config *cfg, struct Account *acc)
{
    pthread_mutex_lock(&cfg->mu);
    pthread_mutex_lock(&acc->mu);
    // reload account from config
    pthread_mutex_unlock(&acc->mu);
    pthread_mutex_unlock(&cfg->mu);
}

// ============================================================================
// Deep nesting: bank->users[i]->wallet->mu vs bank->users[i]->primary->mu
//
// Lock identity is type-based:
//   bank->users[0]->wallet->mu  → Wallet.mu
//   bank->users[0]->primary->mu → Account.mu
//   user->wallet->mu            → Wallet.mu (same as above)
// ============================================================================

void
pay_from_wallet(struct Bank *bank, int user_idx)
{
    // Deep chain: bank->users[idx]->primary->mu is Account.mu
    // Deep chain: bank->users[idx]->wallet->mu is Wallet.mu
    pthread_mutex_lock(&bank->users[user_idx].primary->mu);
    pthread_mutex_lock(&bank->users[user_idx].wallet->mu);
    // transfer from wallet to account
    pthread_mutex_unlock(&bank->users[user_idx].wallet->mu);
    pthread_mutex_unlock(&bank->users[user_idx].primary->mu);
}

void
refund_to_wallet(struct User *user)
{
    // Same types accessed via different path — still conflicts
    // user->wallet->mu is Wallet.mu, user->primary->mu is Account.mu
    pthread_mutex_lock(&user->wallet->mu);
    pthread_mutex_lock(&user->primary->mu);
    // refund to wallet
    pthread_mutex_unlock(&user->primary->mu);
    pthread_mutex_unlock(&user->wallet->mu);
}

// ============================================================================
// Deep inner chain: tx->src->data->mu vs tx->dst->meta->mu
//
// 3 levels of pointer indirection:
//   tx->src->data->mu  → Data.mu   (Transaction* -> Source* -> Data*)
//   tx->dst->meta->mu  → Meta.mu   (Transaction* -> Destination* -> Meta*)
// ============================================================================

void
begin_transaction(struct Transaction *tx)
{
    // tx->src->data->mu resolves to Data.mu
    // tx->dst->meta->mu resolves to Meta.mu
    pthread_mutex_lock(&tx->src->data->mu);
    pthread_mutex_lock(&tx->dst->meta->mu);
    // begin
    pthread_mutex_unlock(&tx->dst->meta->mu);
    pthread_mutex_unlock(&tx->src->data->mu);
}

void
rollback_transaction(struct Transaction *tx)
{
    // Opposite order — deadlock with begin_transaction!
    pthread_mutex_lock(&tx->dst->meta->mu);
    pthread_mutex_lock(&tx->src->data->mu);
    // rollback
    pthread_mutex_unlock(&tx->src->data->mu);
    pthread_mutex_unlock(&tx->dst->meta->mu);
}

int
main(void)
{
    struct User users[1];
    g_bank.users = users;

    deposit(&g_config, &g_account, 100);
    withdraw(&g_account, &g_config, 50);
    transfer(&g_account, &g_logger);
    checkpoint(&g_logger, &g_config);
    reload(&g_config, &g_account);
    pay_from_wallet(&g_bank, 0);
    refund_to_wallet(g_bank.users);
    return 0;
}

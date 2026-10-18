// compile:
// gcc -pthread -o sum_arrays sum_arrays.c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SEQ_THRESHOLD 50000LL
#define MIN_CHUNK 4096

typedef long long ll;

typedef enum { 
    S_AUTO,
    S_SEQ,
    S_ELEM
} strategy_mode;

typedef struct { 
    ll from, to;
} range_t;

static int K = 8;
static ll N = 1000000;
static int T = 1;
static ll **A;       
static ll *res;    
static int hold_sec = 0; 

static void hold_threads(void) {
    if (hold_sec > 0){
        sleep(hold_sec);
    }  
}

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void run_seq(void) {
    memcpy(res, A[0], N * sizeof(ll));
    for (int k = 1; k < K; k++) {
        const ll *a = A[k];
        for (ll i = 0; i < N; i++) {
            res[i] += a[i];
        } 
    }
}

static void *worker_elem(void *p) {
    range_t *r = p;
    memcpy(res + r->from, A[0] + r->from, (r->to - r->from) * sizeof(ll));
    for (int k = 1; k < K; k++) {
        const ll *a = A[k];
        for (ll i = r->from; i < r->to; i++) {
            res[i] += a[i];
        } 
    }
    hold_threads();
    return NULL;
}

static void run_elem(int threads) {
    pthread_t th[threads];
    range_t rg[threads];
    ll chunk = (N + threads - 1) / threads;
    int started = 0;
    for (int t = 0; t < threads; t++) {
        rg[t].from = (ll)t * chunk;
        rg[t].to = rg[t].from + chunk;
        if (rg[t].from >= N) break;
        if (rg[t].to > N) rg[t].to = N;
        pthread_create(&th[t], NULL, worker_elem, &rg[t]);
        started++;
    }
    for (int t = 0; t < started; t++) {
        pthread_join(th[t], NULL);
    } 
}

static const char *choose(strategy_mode m, int *threads) {
    if (m == S_SEQ || T <= 1) { *threads = 1; return "seq"; }
    if (m == S_ELEM) return "elem";
    ll ops = (ll)(K - 1) * N;
    if (ops < SEQ_THRESHOLD) { *threads = 1; return "seq"; }
    ll max_by_n = N / MIN_CHUNK;
    if (max_by_n < 2) { *threads = 1; return "seq"; }   
    if (max_by_n < T) *threads = (int)max_by_n;          
    return "elem";
}

int main(int argc, char **argv) {
    strategy_mode mode = S_AUTO;
    int reps = 5, hold = 0, opt;
    while ((opt = getopt(argc, argv, "k:n:t:s:r:w:")) != -1) {
        switch (opt) {
        case 'k': K = atoi(optarg); break;
        case 'n': N = atoll(optarg); break;
        case 't': T = atoi(optarg); break;
        case 'r': reps = atoi(optarg); break;
        case 'w': hold = atoi(optarg); break;
        case 's':
            if (!strcmp(optarg, "auto")) mode = S_AUTO;
            else if (!strcmp(optarg, "seq")) mode = S_SEQ;
            else if (!strcmp(optarg, "elem")) mode = S_ELEM;
            else { fprintf(stderr, "bad -s\n"); return 1; }
            break;
        default:
            fprintf(stderr, "usage: %s -k K -n N -t T [-s auto|seq|elem] [-r R] [-w SEC]\n", argv[0]);
            return 1;
        }
    }
    if (K < 1 || N < 1 || T < 1) { fprintf(stderr, "bad params\n"); return 1; }

    A = malloc(K * sizeof(ll*));
    for (int k = 0; k < K; k++) {
        A[k] = malloc(N * sizeof(ll));
        for (ll i = 0; i < N; i++) {
            A[k][i] = i + k;
        } 
    }
    res = malloc(N * sizeof(ll));

    hold_sec = hold;
    if (hold > 0) reps = 1;
    int th = T;
    const char *strat = choose(mode, &th);
    double best = 1e30;
    for (int r = 0; r < reps; r++) {
        double t0 = now();
        if (!strcmp(strat, "seq")) {
            run_seq();
        } 
        else {
            run_elem(th);
        }
        double dt = now() - t0;
        if (dt < best) {
            best = dt;
        }
    }

    int ok = 1;
    for (ll i = 0; i < N && ok; i++) {
        ll s = 0;
        s = K * i + K * (K - 1) / 2; 
        if (s != res[i]) {
            ok = 0; 
            fprintf(stderr, "mistake at %lld\n", i);
        }
    }

    printf("K=%d N=%lld T=%d strategy=%s used_threads=%d time=%.6f check=%s\n",
           K, N, T, strat, th, best * 100.f, ok ? "OK" : "FAIL");

    return 0;
}


#include "world/entity/ai/goal.h"
#include <cstdlib>

static const unsigned GOAL_SLOT = 48;

static const int GOAL_POOL = 768;

static unsigned char s_pool[GOAL_POOL][GOAL_SLOT];
static bool          s_used[GOAL_POOL];
static int           s_next = 0;

void* Goal::operator new(size_t n) {
    if (n <= GOAL_SLOT) {
        for (int k = 0; k < GOAL_POOL; k++) {
            int i = s_next + k; if (i >= GOAL_POOL) i -= GOAL_POOL;
            if (!s_used[i]) {
                s_used[i] = true;
                s_next = (i + 1 == GOAL_POOL) ? 0 : i + 1;
                return s_pool[i];
            }
        }
    }
    return malloc(n);
}

void Goal::operator delete(void* p) {
    if (!p) return;
    unsigned char* c = (unsigned char*)p;
    if (c >= s_pool[0] && c < s_pool[0] + sizeof(s_pool)) {
        s_used[(c - s_pool[0]) / GOAL_SLOT] = false;
        return;
    }
    free(p);
}

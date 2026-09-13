
#ifndef MCPSP_WORLD_ENTITY_AI_GOAL_H
#define MCPSP_WORLD_ENTITY_AI_GOAL_H

#include <cstddef>

class Goal {
public:

    enum { CONTROL_MOVE = 1, CONTROL_LOOK = 2, CONTROL_JUMP = 4 };

    Goal() : controlFlags(0) {}
    virtual ~Goal() {}

    static void* operator new(size_t n);
    static void  operator delete(void* p);

    virtual bool canUse() = 0;

    virtual bool canContinueToUse() { return canUse(); }
    virtual bool canInterrupt() { return true; }
    virtual void start() {}
    virtual void stop() {}
    virtual void tick() {}

    void setRequiredControlFlags(int f) { controlFlags = f; }
    int  getRequiredControlFlags() const { return controlFlags; }

private:
    int controlFlags;
};

#endif

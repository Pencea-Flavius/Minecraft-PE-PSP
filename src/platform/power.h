
#ifndef MCPSP_PLATFORM_POWER_H
#define MCPSP_PLATFORM_POWER_H

void powerHoldInit();
void powerHoldAcquire();
void powerHoldRelease();

struct PowerHold {
    PowerHold()  { powerHoldAcquire(); }
    ~PowerHold() { powerHoldRelease(); }
private:
    PowerHold(const PowerHold&);
    PowerHold& operator=(const PowerHold&);
};

#endif

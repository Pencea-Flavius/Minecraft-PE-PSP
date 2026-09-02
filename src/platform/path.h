
#ifndef MCPSP_PLATFORM_PATH_H
#define MCPSP_PLATFORM_PATH_H

void pathInit(const char* argv0);

const char* assetPath(const char* rel);

const char* savePath(const char* rel);

void savePathInit(void);

const char* pathDevice(void);

#endif

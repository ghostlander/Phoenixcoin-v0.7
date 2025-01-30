// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software licence, see the accompanying
// file LICENCE or http://opensource.org/license/mit

#ifndef INIT_H
#define INIT_H

#include <string>

void StartShutdown();
void Shutdown(void *parg);
bool AppInit2();
std::string HelpMessage();

extern std::string strClientLaunchDateTime;

#endif /* INIT_H */

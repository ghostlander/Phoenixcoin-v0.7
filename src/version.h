// Copyright (c) 2012 The Bitcoin developers
// Distributed under the MIT/X11 software licence, see the accompanying
// file LICENCE or http://opensource.org/license/mit

#ifndef VERSION_H
#define VERSION_H

#include <string>

#include "clientversion.h"

static const int CLIENT_VERSION =
                           1000000 * CLIENT_VERSION_MAJOR
                         +   10000 * CLIENT_VERSION_MINOR
                         +     100 * CLIENT_VERSION_REVISION
                         +       1 * CLIENT_VERSION_BUILD;

extern const std::string CLIENT_NAME;
extern const std::string CLIENT_BUILD;
extern const std::string CLIENT_DATE;

static const int PROTOCOL_VERSION = 70000;
static const int MAX_PROTOCOL_VERSION = 79999;
static const int MIN_PROTOCOL_VERSION = 60004;

// earlier versions not supported as of Feb 2012, and are disconnected
static const int MIN_PROTO_VERSION = 209;

#endif /* VERSION_H */

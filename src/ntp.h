#ifndef NTP_H
#define NTP_H

#include <string>

#include "datatypes.h"

extern std::string strTrustedNTP;
extern int64 nNtpOffset;
extern bool fNtpWarning;

class CNetAddr;

/* Get time from a random server and return its IP address */
int64 NtpGetTime(CNetAddr &ip);

/* Get time from the trusted server */
int64 NtpGetTime(const std::string &strHostName);

/* NTP polling thread */
void ThreadNtpPoller(void *parg);

#endif /* NTP_H */

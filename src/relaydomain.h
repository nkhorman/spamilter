
#ifndef _RELAYDOMAIN_H_
#define _RELAYDOMAIN_H_

// Find pDomain in list of relay domains
// Returns;
//	-1 = error
//	1 = found
//	0 = not found
int relayDomain(const char *pDomain, const char *pFname);

#endif

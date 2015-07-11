#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "relaydomain.h"

// Find pDomain in list of relay domains
// Returns;
//	-1 = error
//	1 = found
//	0 = not found
int relayDomain(const char *pDomain, const char *pFname)
{
	int found = -1;
	FILE *fin = fopen(pFname, "r");

	if(fin != NULL)
	{
		char buff[1024];
		char *pStr = NULL;
		char *p;

		found = 0;
		while(!feof(fin) && !found)
		{
			memset(buff, 0, sizeof(buff));
			pStr = fgets(buff, sizeof(buff)-1, fin);

			if(pStr != NULL && *pStr)
			{
				// left trim
				while(*pStr && (*pStr == ' ' || *pStr == '\t'))
					pStr ++;

				// right trim
				p = pStr + strlen(pStr) - 1;
				while(p > pStr && (*p == ' ' || *p == '\t' || *p =='\r' || *p == '\n'))
					*(p--) = 0;

				if(*pStr)
					found = (strcasecmp(pStr, pDomain) == 0);
			}
		}

		fclose(fin);
	}

	return found;
}

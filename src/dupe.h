#ifndef _DUPE_H_
#define _DUPE_H_

#define DUPE_FROM 1
#define DUPE_RCPT 2

	void dupe_init(mlfiPriv *priv);
	void dupe_query(mlfiPriv *priv, char *mailstr, int stage);
	void dupe_action(SMFICTX *ctx, mlfiPriv *priv);
#endif

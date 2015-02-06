/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Neal Horman - http://www.wanlink.com
 *	Copyright (c) 2013 Neal Horman. All Rights Reserved
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions
 *	are met:
 *	1. Redistributions of source code must retain the above copyright
 *	   notice, this list of conditions and the following disclaimer.
 *	2. Redistributions in binary form must reproduce the above copyright
 *	   notice, this list of conditions and the following disclaimer in the
 *	   documentation and/or other materials provided with the distribution.
 *	3. All advertising materials mentioning features or use of this software
 *	   must display the following acknowledgement:
 *	This product includes software developed by Neal Horman.
 *	4. Neither the name Neal Horman nor the names of any contributors
 *	   may be used to endorse or promote products derived from this software
 *	   without specific prior written permission.
 *	
 *	THIS SOFTWARE IS PROVIDED BY NEAL HORMAN AND ANY CONTRIBUTORS ``AS IS'' AND
 *	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *	ARE DISCLAIMED.  IN NO EVENT SHALL NEAL HORMAN OR ANY CONTRIBUTORS BE LIABLE
 *	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *	OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *	OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *	SUCH DAMAGE.
 *
 *	$Id: watcher.c,v 1.4 2013/07/20 06:50:54 neal Exp $
 *
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: watcher.c,v 1.4 2013/07/20 06:50:54 neal Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <sysexits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>

#include <fcntl.h>
#include <paths.h>
#include <sys/param.h>

#include "config.h"

#ifndef NEED_LIBUTIL_LOCAL
#include <libutil.h>
#else
#include "libutil/pidfile.h"
#endif

#include "watcher.h"

pid_t g_WorkerPid	= 1;
struct pidfh *g_pfh	= NULL;

void main_worker_signal_hndlr(int signo)
{
	switch(signo)
	{
		case SIGHUP:
			syslog(LOG_ERR,"Worker - Signal %u received\n",signo);
			break;
		default:
			syslog(LOG_ERR,"Worker - Shutdown with signal %u\n",signo);
			exit(signo);
			break;
	}
}

void main_watcher_shutdown();

void main_watcher_signal_hndlr(int signo)
{
	switch(signo)
	{
		case SIGHUP:
			syslog(LOG_ERR,"Watcher - Signal %u received\n",signo);
			break;
		default:
			syslog(LOG_ERR,"Watcher - Shutdown with signal %u\n",signo);
			main_watcher_shutdown();
			exit(signo);
			break;
	}
}

int main_watcher_waitforchild()
{	int restart = 0;
	int wstatus = 0;

	syslog(LOG_ERR,"Watcher - waiting for Worker [%d] to exit\n",g_WorkerPid);
	waitpid(g_WorkerPid,&wstatus,WUNTRACED|WCONTINUED);
	if(WIFEXITED(wstatus))
	{	int exitval = WEXITSTATUS(wstatus);

		syslog(LOG_ERR,"Watcher - Worker [%d] exited with %d\n",g_WorkerPid,exitval);
		restart = g_WorkerPid = 1;//(exitval != SIGTERM && exitval != SIGQUIT);
	}
	else if(WIFSIGNALED(wstatus))
	{	int signalval = WTERMSIG(wstatus);

		syslog(LOG_ERR,"Watcher - Worker [%d] exited with signal %d\n",g_WorkerPid,signalval);
		restart = g_WorkerPid = (signalval != SIGTERM && signalval != SIGQUIT);
	}

	return restart;
}

void main_watcher_shutdown()
{
	if(g_WorkerPid != -1 && g_WorkerPid != 1 && g_WorkerPid != 0)
	{
		syslog(LOG_ERR,"Watcher - send SIGKILL to worker [%d]\n",g_WorkerPid);
		kill(g_WorkerPid,SIGKILL);
		main_watcher_waitforchild();
	}

	if(g_pfh != NULL)
	{
		pidfile_remove(g_pfh);
		g_pfh = NULL;
	}
}

void stdio_2_devnull()
{	int fd = open(_PATH_DEVNULL, O_RDWR, 0);

	if(fd != -1)
	{
		(void)dup2(fd, STDIN_FILENO);
		(void)dup2(fd, STDOUT_FILENO);
		(void)dup2(fd, STDERR_FILENO);
		if (fd > 2)
			(void)close(fd);
	}
}

int main_watcher(int forking, char *pFnamePid)
{	int rc = 0;
	pid_t otherpid = 0;

	g_pfh = pidfile_open(pFnamePid, 0644, &otherpid);

	if(g_pfh != NULL)
	{
		if(!forking)
		{
#ifdef HAVE_SETPROCTITLE
			setproctitle("debug worker");
#endif
			pidfile_write(g_pfh);
			rc = worker_main();
			pidfile_remove(g_pfh);
		}
		else if(fork() == 0)	// fork a watcher
		{
#ifdef HAVE_SETPROCTITLE
			setproctitle("watcher");
#endif
			pidfile_write(g_pfh);

			stdio_2_devnull();

			pid_t pid = getpid();

			syslog(LOG_ERR,"Watcher - started as [%d]\n",pid);

			while(g_WorkerPid != 0 && g_WorkerPid != -1)
			{
				if((g_WorkerPid = fork()) == 0)	// fork a worker
				{
					signal(SIGTERM,main_worker_signal_hndlr);
					signal(SIGQUIT,main_worker_signal_hndlr);
					signal(SIGINT,main_worker_signal_hndlr);
					signal(SIGHUP,main_worker_signal_hndlr);
					signal(SIGSEGV,main_worker_signal_hndlr);
					signal(SIGBUS,main_worker_signal_hndlr);
					signal(SIGPIPE,main_worker_signal_hndlr);
					signal(SIGABRT,main_worker_signal_hndlr);

					syslog(LOG_ERR,"Worker - started as [%d]\n",getpid());
#ifdef HAVE_SETPROCTITLE
					setproctitle("worker");
#endif
					return worker_main();
				}
				else
				{
					signal(SIGTERM,main_watcher_signal_hndlr);
					signal(SIGQUIT,main_watcher_signal_hndlr);
					signal(SIGINT,main_watcher_signal_hndlr);
					signal(SIGHUP,main_watcher_signal_hndlr);
					signal(SIGSEGV,main_watcher_signal_hndlr);
					signal(SIGBUS,main_watcher_signal_hndlr);
					signal(SIGPIPE,main_watcher_signal_hndlr);
					signal(SIGABRT,main_watcher_signal_hndlr);

					if(g_WorkerPid != -1)	// wait for for worker to exit
					{
						if(main_watcher_waitforchild()) // restart ?
						{
							syslog(LOG_ERR,"Watcher - sleeping 30 seconds before worker restart\n");
							sleep(30);
						}
					}
				}
			}
			main_watcher_shutdown();
		}
	}
	else
	{
		if (errno == EEXIST)
			fprintf(stderr,"Already running, pid: %d\n", (int)otherpid);
		else
			fprintf(stderr,"Unable to create pid file, error %d\n",errno);
	}

	return rc;
}

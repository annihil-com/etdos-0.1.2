#ifndef _NET_H
#define _NET_H

void get_packets();
void check_resend();
void send_cmds(int force);
void sv_download(const char *remote_file);

#endif

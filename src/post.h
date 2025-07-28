#ifndef _POST_H
#define _POST_H

char *build_json(char *machine_id, char *hostname, char *os_name, char *cpu_brand, long sec, long nsec, char *version);
int postmark(char *json, char** response);

#endif
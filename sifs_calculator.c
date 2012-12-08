#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define STR_MAC_ADDR_LEN 20
#define MAX_FILENAME 20

#define IEEE80211_ACK 0x1d
#define IEEE80211_DATA 0x20

typedef char macaddr[STR_MAC_ADDR_LEN];

void usage(void)
{
    printf("usage\n");
    return;
}

void parse_line(char *line, int *type, int *rate, int *frmlen, long *timestamp, macaddr dst, macaddr bssid, macaddr src)
{
    int i=0, pass=0, read_val = 1;
    char *cur;

    for(cur = line; (cur - line) < 127; cur++) {
	if(*cur == ',') {
	    cur++;
	    read_val = 1;
	}
	if(read_val) {
	    switch(pass) {
	    case 0:
		/* Type */
		*type = (int)strtol(cur, NULL, 0);
		break;
	    case 2:
		/* Timestamp */
		*timestamp = (long)strtol(cur, NULL, 0);
		break;
	    case 4:
		/* rate */
		*rate = (int)strtol(cur, NULL, 0);
		break;
	    case 5:
		/* frmlen */
		*frmlen = (int)strtol(cur, NULL, 0);
		break;
	    case 6:
		/* dst */
		strncpy(dst, cur, 17);
		bssid[17] = 0;
		break;
	    case 7:
		/* bssid */
		strncpy(bssid, cur, 17);
		mac[17] = 0;
		break;
	    case 8:
		/* src */
		strncpy(src, cur, 17);
		mac[17] = 0;
		break;

	    default: break;
	    }
	    pass++;
	    read_val = 0;
	}
    }
}

void calculate_sifs(char *input_file, macaddr mac, macaddr bssid)
{
    FILE *fp = NULL;
    char line[128];
    macaddr oldmac={0};
    long old_timestamp;

    fp = fopen(input_file, "r+");
    if(!fp) {
	printf("Couldn't open input file");
	exit(EXIT_FAILURE);
    }

    while(!feof(fp)) {
	int type, rate, frmlen;
	long timestamp;
	macaddr dst_pkt, src_pkt, bssid_pkt;

	fgets(line, 128, fp);	
	parse_line(line, &type, &rate, &frmlen, &timestamp, dst_pkt, bssid_pkt, src_pkt);
	switch(type) {
	case IEEE80211_DATA:
	    if(strncmp(bssid_pkt, bssid, 17) != 0 ||
	       (strncmp(mac, dst, 17) != 0 &&
		strncmp(mac, src, 17) != 0))
		continue;
	    if(oldmac[0] == 0) {
		
	    }
	case IEEE80211_ACK:

	}
    }
    
}

int is_valid(char *str)
{
    return 1;
}

int main(int argc, char *argv[])
{
    char c;
    macaddr mac={0}, bssid={0};
    char input_file[MAX_FILENAME] = {0};

    while((c = getopt(argc, argv, "i:b:m:")) != -1) {
	switch(c) {
	case 'i':
	    strcpy(input_file, optarg);
	    break;
	case 'b':
	    strcpy(bssid, optarg);
	    break;
	case 'm':
	    strcpy(mac, optarg);
	    break;
	}
    }
    if(!(is_valid(input_file) && is_valid(bssid) && is_valid(mac))) {
	usage();
	return 0;
    }
    calculate_sifs(input_file, mac, bssid);
}

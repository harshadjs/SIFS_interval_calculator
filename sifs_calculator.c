#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define STR_MAC_ADDR_LEN 20
#define MAX_FILENAME 20

#define IEEE80211_ACK 0x1d
#define IEEE80211_DATA 0x20
#define IEEE80211_RTS 0x1b
#define IEEE80211_CTS 0x1c

#define PLCP_TIME 29

typedef char macaddr[STR_MAC_ADDR_LEN];
#define BROADCAST_MAC "FF:FF:FF:FF:FF:FF"
#define STRING_EQUAL(_str1, _str2, _no) (strncasecmp((_str1), (_str2), (_no)) == 0)

struct packet {
    int type, rate, frmlen, number;
    long long timestamp;
    macaddr dst, src, bssid;
};

int is_prism;

void usage(void)
{
    printf("usage\n");
    return;
}

void parse_line(char *line, struct packet *packet)
{
    int i=0, pass=0, read_val = 1;
    char *cur, *ptr;

    for(cur = line; (cur - line) < 127;) {
	if(*cur == ',') {
	    cur++;
	    read_val = 1;
	    if(*cur == ',')
		read_val = 0;
	}
	if(*cur == '\n')
	    break;
	if(read_val) {
	    switch(pass) {
	    case 0:
		/* Type */
		packet->type = (int)strtol(cur, NULL, 0);
		break;
	    case 2:
		/* Timestamp */
		packet->timestamp = (long long)strtoll(cur, NULL, 0);
		break;
	    case 4:
		if(is_prism)
		    packet->rate = (int)strtol(cur, NULL, 0);
		else
		    packet->rate = ((float)strtof(cur, NULL) * 2.0);
		break;
	    case 5:
		/* frmlen */
		packet->frmlen = (int)strtol(cur, NULL, 0);
		break;
	    case 6:
		/* dst */
		strncpy(packet->dst, cur, 17);
		packet->dst[17] = 0;
		break;
	    case 7:
		/* bssid */
		strncpy(packet->bssid, cur, 17);
		packet->bssid[17] = 0;
		break;
	    case 8:
		/* src */
		strncpy(packet->src, cur, 17);
		packet->src[17] = 0;
		break;
	    case 9:
		/* receiver address in case of RTS/CTS/ACK */
		strncpy(packet->dst, cur, 17);
		packet->dst[17] = 0;
		break;
	    default: break;
	    }
	    read_val = 0;
	}
	pass++;
	ptr = strchr(cur, ',');
	if(ptr)
	    cur = ptr;
	else
	    break;
    }
}

inline long long time_for(long frmlen, int rate)
{
    return (long long)(((frmlen * 8) * 1024.0 * 1024.0)/(((double)rate/2) * 1000.0 * 1000.0));
}

inline int is_ackable(int type)
{
    switch(type) {
    case IEEE80211_ACK:
    case IEEE80211_CTS:
    case IEEE80211_RTS:
	return 0;
    }
    return 1;
}

void dump_packet(struct packet *packet)
{
    printf("%d)\t%x\t%d\t%d\t%lld\t%s\t%s\t%s\n", packet->number, packet->type,
	   packet->rate, packet->frmlen, packet->timestamp,
	   packet->dst, packet->src, packet->bssid);
}

void calculate_sifs(char *input_file, macaddr mac, macaddr bssid)
{
    FILE *fp = NULL;
    char line[128];
    macaddr oldmac={0};
    struct packet old_packet;
    int packet_number, old_packet_acked;

    fp = fopen(input_file, "r+");
    if(!fp) {
	printf("Couldn't open input file");
	exit(EXIT_FAILURE);
    }

    packet_number = 0;
    memset(&old_packet, 0, sizeof(struct packet));
    while(!feof(fp)) {
	struct packet packet;

	fgets(line, 128, fp);	
	memset(&packet, 0, sizeof(struct packet));

	parse_line(line, &packet);
	packet.number = ++packet_number;

	if(STRING_EQUAL(BROADCAST_MAC, packet.dst, 17))
	    continue;
	if(packet.type == IEEE80211_DATA && !STRING_EQUAL(bssid, packet.bssid, 17))
	    continue;
	if(packet.type != IEEE80211_ACK && !STRING_EQUAL(packet.src, mac, 17)
	   && !STRING_EQUAL(packet.dst, mac, 17))
	    continue;

	if(packet.type == IEEE80211_ACK) {
	    if(STRING_EQUAL(old_packet.src, packet.dst, 17) ||
	       STRING_EQUAL(old_packet.bssid, packet.dst, 17)) {
		long long timestamp_ack, timestamp_old;
		//		printf("%d: Ack RECEIVER %s - Acks packet number %d (%d)--->\n", packet_number, packet.dst, old_packet.number, time_for(packet.frmlen, packet.rate));
		timestamp_ack = packet.timestamp - time_for(packet.frmlen, packet.rate) - PLCP_TIME;
		timestamp_old = old_packet.timestamp;
		if(STRING_EQUAL(old_packet.dst, mac, 17))
		    printf("%lld\n", timestamp_ack - timestamp_old);
		//		printf("\t%d: DST %s BSSID %s SRC %s\n", old_packet.number, old_packet.dst, old_packet.bssid, old_packet.src);
		//		printf("\t\tReceived within %ldus\n\n", timestamp_ack - timestamp_old);
		memset(&old_packet, 0, sizeof(struct packet));
	    }
	    /* else  */
	    /* 	printf("%d: Unresolved ack packet dst %s\n\n", packet_number, packet.dst); */
	}
	else {
	    if(is_ackable(packet.type))
		old_packet = packet;
	}
    }
}

int is_valid(char *str)
{
    if(str[0] == 0)
	return 0;
    return 1;
}

void read_clients(char *input_file)
{
    FILE *fp = NULL;
    char line[128];

    fp = fopen(input_file, "r+");
    if(!fp) {
	printf("Couldn't open input file");
	exit(EXIT_FAILURE);
    }
    while(!feof(fp)) {
	struct packet packet;

	fgets(line, 128, fp);	
	memset(&packet, 0, sizeof(struct packet));

	parse_line(line, &packet);
	
    }
}

int main(int argc, char *argv[])
{
    char c;
    macaddr mac={0}, bssid={0};
    char input_file[MAX_FILENAME] = {0};

    is_prism = 0;
    while((c = getopt(argc, argv, "i:b:m:p")) != -1) {
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
	case 'p':
	    is_prism = 1;
	    break;
	}
    }
    if(!is_valid(input_file))
	return 0;
    if(!is_valid(bssid) || !is_valid(mac)) {
	read_clients(input_file);
	return 0;
    }
    printf("mac - %s\t bssid - %s\n", mac, bssid);
    calculate_sifs(input_file, mac, bssid);
}

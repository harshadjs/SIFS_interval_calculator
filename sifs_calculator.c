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

#define IS_OUTLIER(packet_gap) (((packet_gap) > 100) || ((packet_gap) < 0))


struct packet {
	int type, rate, frmlen, number;
	long long timestamp;
	macaddr dst, src, bssid;
};

int proto;

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
				if(proto == 'p')
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

void calculate_sifs(char *input_file)
{
	FILE *fp = NULL;
	char line[128];
	macaddr oldmac={0};
	struct packet old_packet;
	int packet_number, old_packet_acked;
	long long timestamp, timestamp_old = 0;
	long long ack_count = 0;
	long double sifs = 0.0;

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

		timestamp = packet.timestamp - time_for(packet.frmlen, packet.rate);
		timestamp_old = old_packet.timestamp;
		//		printf("%d) Size %d travelling @%dmbps took %lld time(Interpacket gap = %lld)\n", packet.number, packet.frmlen, packet.rate, time_for(packet.frmlen, packet.rate) + PLCP_TIME, timestamp - timestamp_old);
		if((packet.type == 0x1d) &&
		   !IS_OUTLIER(timestamp - timestamp_old)) {
			sifs = ((((sifs) * ack_count) + (timestamp - timestamp_old)) / (++ack_count));
		}
		old_packet = packet;
	}

	printf("Avergage SIFS observed in the trace = %Lfus\n", sifs);
	printf("\tTotal ack frames = %lld\n", ack_count);
}

int is_valid(char *str)
{
	if(str[0] == 0)
		return 0;
	return 1;
}

void usage(void)
{
	printf("Usage: -i <input_file> -p <protocol p:prism r:radiotap>\n");
}

int main(int argc, char *argv[])
{
	char c;
	char input_file[MAX_FILENAME] = {0};

	proto = 0;
	while((c = getopt(argc, argv, "i:p:")) != -1) {
		switch(c) {
		case 'i':
			strcpy(input_file, optarg);
			break;
		case 'p':
			proto = (int)optarg;
			break;
		default:
			usage();
			return 0;
		}
	}
	if((!is_valid(input_file)) || (proto == 0)) {
		usage();
		return 0;
	}
	calculate_sifs(input_file);
	return 0;
}

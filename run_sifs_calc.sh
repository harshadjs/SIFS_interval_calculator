#!/bin/bash

PCAP_FILE=$1
TMP_CSV_FILE=/tmp/sifs_calc.csv

tshark -r $PCAP_FILE -T fields -e wlan.fc.type_subtype -e prism.did.hosttime -e prism.did.rate -e prism.did.frmlen -e wlan.da -e wlan.bssid -s wlan.sa -E separator=',' > $TMP_CSV_FILE

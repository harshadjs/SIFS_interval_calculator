#!/bin/bash

PCAP_FILE=$1
HEADER_TYPE=$2
TMP_CSV_FILE=/tmp/sifs_calc.csv

if [ "$HEADER_TYPE" = "PRISM" ] ; then
    tshark -r $PCAP_FILE -T fields -e wlan.fc.type_subtype -e prism.did.hosttime -e prism.did.rate -e prism.did.frmlen -e wlan.da -e wlan.bssid -e wlan.sa -e wlan.ra -E separator=',' > $TMP_CSV_FILE
else if [ "$HEADER_TYPE" = "RADIOTAP" ] ; then
    tshark -r $PCAP_FILE -T fields -e wlan.fc.type_subtype -e'dummy' -e radiotap.mactime -e'dummy' -e radiotap.datarate -e frame.len -e wlan.da -e wlan.bssid -e wlan.sa -e wlan.ra -E separator=',' > $TMP_CSV_FILE
else
    echo "Error"
fi

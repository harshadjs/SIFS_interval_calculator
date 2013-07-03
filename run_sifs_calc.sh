
#!/bin/bash

PCAP_FILE=$1
TMP_CSV_FILE=/tmp/sifs_calc.csv

calculate_sifs()
{
    if [ "$2" = "prism" ]; then
	proto=p
    else
	proto=r
    fi
    ./sifs_calculator -i $1 -p $proto
    rm $TMP_CSV_FILE
}


if [ "$1" = "" ]; then
    echo "Usage: $0 <pcap_file>"
    exit
fi

# Check if tshark is installed
hash tshark 2>/dev/null
if [ "$?" != "0" ]; then
    echo "tshark is not installed. Run the following command to install it:"
    echo "sudo apt-get install tshark"
    exit
fi

HEADER_TYPE=`tshark -r sample_trace.pcap -T fields -e frame.number -e frame.protocols frame.number==1 -E separator="," | cut -d "," -f2 | cut -d ":" -f1`

echo -ne "Exporting to csv ... "
if [ "$HEADER_TYPE" = "prism" ] ; then
    tshark -r $PCAP_FILE -T fields -e wlan.fc.type_subtype -e prism.did.hosttime -e prism.did.rate -e prism.did.frmlen -e wlan.da -e wlan.bssid -e wlan.sa -e wlan.ra -E separator=',' > $TMP_CSV_FILE
    if [ "$?" = "0" ] ; then
	echo "[DONE]"
	calculate_sifs $TMP_CSV_FILE $HEADER_TYPE
	exit
    else
	echo "pcap can't be exported to CSV."
	exit
    fi
elif [ "$HEADER_TYPE" = "radiotap" ] ; then
    tshark -r $PCAP_FILE -T fields -e wlan.fc.type_subtype -e "dummy" -e radiotap.mactime -e "dummy" -e radiotap.datarate -e frame.len -e wlan.da -e wlan.bssid -e wlan.sa -e wlan.ra -E separator=',' > $TMP_CSV_FILE
    if [ "$?" = "0" ] ; then
	echo "Successfully created CSV file : $TMP_CSV_FILE"
	exit
    else
	echo "Failure while creating CSV file"
	exit
    fi
else
    echo "Unrecognized header type"
fi

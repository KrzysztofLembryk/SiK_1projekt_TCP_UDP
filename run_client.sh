for file in *.txt; do ./ppcb_client udp 0.0.0.0 3331 <"$file"
done
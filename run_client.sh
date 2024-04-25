for file in *.txt; do ./ppcb_client tcp 0.0.0.0 3338 <"$file"
done
#!/bin/bash
# Advance to the next slide (or set specific slide)
SLIDE_FILE="/tmp/atomik_slide.txt"
if [ -n "$1" ]; then
    echo "$1" > "$SLIDE_FILE"
else
    current=$(cat "$SLIDE_FILE" 2>/dev/null || echo 0)
    echo $((current + 1)) > "$SLIDE_FILE"
fi
echo "Slide: $(cat $SLIDE_FILE)"

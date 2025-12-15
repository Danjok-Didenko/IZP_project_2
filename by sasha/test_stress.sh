#!/bin/bash

# Fix for tput in non-interactive environments
export TERM=${TERM:-xterm}

# ==============================================================================
# CONFIGURATION & COLORS
# ==============================================================================
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
GRAY='\033[0;90m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Spinner characters
SPINNER="⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏"

# Argument Parsing (MAX_JOBS)
MAX_JOBS=${1:-4}
if ! [[ "$MAX_JOBS" =~ ^[0-9]+$ ]]; then
    echo "Usage: $0 [MAX_JOBS]"
    exit 1
fi

# ==============================================================================
# PYTHON HELPER SCRIPTS (Embedded)
# ==============================================================================

# 1. Fast Data Generator (Bash loops are too slow for 50k+ lines)
generate_data_py() {
    python3 -c "
import sys, random
filename = sys.argv[1]
count = int(sys.argv[2])
with open(filename, 'w') as f:
    f.write(f'count={count}\n')
    for i in range(count):
        b = random.randint(100, 10000)
        t = random.randint(1, 100)
        pkts = random.randint(1, 50)
        d = round(random.uniform(0.01, 1.0), 3)
        ip = f'10.0.{i%255}.{i%255}'
        f.write(f'{i} {ip} {ip} {b} {t} {pkts} {d}\n')
" "$1" "$2"
}

# 2. Sorting Validator
verify_sort_py() {
    python3 -c "
import sys
last_first_id = -1
try:
    for line in sys.stdin:
        line = line.strip()
        if not line.startswith('cluster'): continue
        parts = line.split(':')
        if len(parts) < 2: continue
        ids = [int(x) for x in parts[1].split()]
        if not ids: continue
        
        # Check 1: IDs inside cluster must be sorted
        if ids != sorted(ids): sys.exit(10)
        
        # Check 2: Clusters must be sorted by first ID
        if ids[0] < last_first_id: sys.exit(11)
        last_first_id = ids[0]
    sys.exit(0)
except Exception:
    sys.exit(1)
" < "$1"
}

# ==============================================================================
# SETUP
# ==============================================================================

echo -e "${BLUE}${BOLD}======================================================${NC}"
echo -e "${BLUE}${BOLD}      HIGH-PERFORMANCE STRESS TEST SUITE (GRID)       ${NC}"
echo -e "${BLUE}${BOLD}======================================================${NC}"
echo -e "Max Parallel Jobs: ${BOLD}$MAX_JOBS${NC}"

# Compile
echo -ne "Compiling... "
cc -std=c11 -Wall -Wextra -Werror -pedantic flows.c -o flows -lm -O3
if [ $? -ne 0 ]; then
    echo -e "${RED}FAIL${NC}"
    exit 1
fi
echo -e "${GREEN}OK${NC}"

# Grid Definitions
FLOW_COUNTS=(100 500 1000 2000 5000)
CLUSTER_COUNTS=(1 5 10 20)

# Generate Data Files
echo -ne "Generating Datasets (Python)... "
mkdir -p temp_test_data
for count in "${FLOW_COUNTS[@]}"; do
    if [ ! -f "temp_test_data/data_${count}.txt" ]; then
         generate_data_py "temp_test_data/data_${count}.txt" "$count"
    fi
done
echo -e "${GREEN}Done${NC}"

# ==============================================================================
# EXECUTION ENGINE
# ==============================================================================

# Function run inside background process
run_test_job() {
    local idx=$1
    local cmd="$2"
    local outfile="temp_test_data/out_${idx}.txt"
    local exp_count=$3
    
    # Run command and measure time
    local ts=$(date +%s%N)
    $cmd > "$outfile" 2>&1
    local ret=$?
    local te=$(date +%s%N)
    local duration=$(( (te - ts) / 1000000 )) # ms

    # Write result metadata to a status file
    if [ $ret -ne 0 ]; then
        echo "$ret|$duration|0|0" > "temp_test_data/status_${idx}.txt"
        return
    fi

    # Validation
    local count_ok=1
    local actual_count=$(grep -c "^cluster" "$outfile")
    if [ "$actual_count" -ne "$exp_count" ]; then count_ok=0; fi

    verify_sort_py "$outfile"
    local sort_ret=$?
    local sort_ok=1
    if [ $sort_ret -ne 0 ]; then sort_ok=0; fi

    echo "0|$duration|$count_ok|$sort_ok" > "temp_test_data/status_${idx}.txt"
}

# Initialize State Arrays
# We map (row, col) to a linear index: idx = row * num_cols + col
NUM_ROWS=${#FLOW_COUNTS[@]}
NUM_COLS=${#CLUSTER_COUNTS[@]}
TOTAL_TESTS=$((NUM_ROWS * NUM_COLS))

PIDS=()
STATUS=() # PENDING, RUNNING, DONE
RESULTS=() # Store formatted result string

for ((i=0; i<TOTAL_TESTS; i++)); do
    STATUS[$i]="PENDING"
    RESULTS[$i]="..."
done

# ==============================================================================
# DRAW TABLE HEADER
# ==============================================================================
echo ""
# Header Row
printf "      "
for c in "${CLUSTER_COUNTS[@]}"; do
    printf "${BOLD}%-10s${NC}" "N=$c"
done
printf "\n"

# Initial Table Body
START_ROW_LINE=$(tput lines) # This is tricky, better to just print and then move up
# We will print the table once, then remember how many lines up we need to go
for ((r=0; r<NUM_ROWS; r++)); do
    f=${FLOW_COUNTS[$r]}
    printf "${BOLD}%-6s${NC}" "$f"
    for ((c=0; c<NUM_COLS; c++)); do
        printf "%-10s" "..."
    done
    printf "\n"
done

# Calculate lines to move up
LINES_UP=$NUM_ROWS

# ==============================================================================
# MAIN LOOP
# ==============================================================================
tput civis # Hide cursor

running_jobs=0
next_job_idx=0
finished_jobs=0
spin_idx=0

while [ $finished_jobs -lt $TOTAL_TESTS ]; do
    
    # 1. Check for finished jobs
    for ((i=0; i<next_job_idx; i++)); do
        if [ "${STATUS[$i]}" == "RUNNING" ]; then
            if ! kill -0 "${PIDS[$i]}" 2>/dev/null; then
                STATUS[$i]="DONE"
                ((running_jobs--))
                ((finished_jobs++))
                
                # Read result
                if [ -f "temp_test_data/status_${i}.txt" ]; then
                    IFS="|" read -r ret dur count_ok sort_ok < "temp_test_data/status_${i}.txt"
                    
                    # Use python for float math since bc might be missing
                    time_str=$(python3 -c "print(f'{${dur}/1000:.3f}s')")
                    
                    if [ "$ret" -ne 0 ]; then
                        RESULTS[$i]="${RED}CRASH${NC}"
                    elif [ "$count_ok" -eq 0 ]; then
                        RESULTS[$i]="${YELLOW}CNT ERR${NC}"
                    elif [ "$sort_ok" -eq 0 ]; then
                        RESULTS[$i]="${YELLOW}SRT ERR${NC}"
                    else
                        RESULTS[$i]="${GREEN}${time_str}${NC}"
                    fi
                else
                    RESULTS[$i]="${RED}ERR${NC}"
                fi
            fi
        fi
    done

    # 2. Start new jobs if slots available
    while [ $running_jobs -lt $MAX_JOBS ] && [ $next_job_idx -lt $TOTAL_TESTS ]; do
        idx=$next_job_idx
        
        # Calculate params
        row=$((idx / NUM_COLS))
        col=$((idx % NUM_COLS))
        
        flow_count=${FLOW_COUNTS[$row]}
        cluster_count=${CLUSTER_COUNTS[$col]}
        file="temp_test_data/data_${flow_count}.txt"
        
        # Launch
        run_test_job "$idx" "./flows $file $cluster_count 1 1 1 1" "$cluster_count" &
        PIDS[$idx]=$!
        STATUS[$idx]="RUNNING"
        
        ((running_jobs++))
        ((next_job_idx++))
    done

    # 3. Redraw Table
    tput cuu $LINES_UP
    
    for ((r=0; r<NUM_ROWS; r++)); do
        f=${FLOW_COUNTS[$r]}
        printf "${BOLD}%-6s${NC}" "$f"
        for ((c=0; c<NUM_COLS; c++)); do
            idx=$((r * NUM_COLS + c))
            st=${STATUS[$idx]}
            
            if [ "$st" == "PENDING" ]; then
                printf "${GRAY}%-10s${NC}" "..."
            elif [ "$st" == "RUNNING" ]; then
                sp=${SPINNER:spin_idx%${#SPINNER}:1}
                printf "${CYAN}%s RUN     ${NC}" "$sp"
            else
                # DONE
                # We need to print the result with color, but printf padding counts invisible chars
                # So we manually pad or rely on the fact that our result strings are short
                # Hack: Print the string, then calculate length without color to pad?
                # Simpler: Just assume fixed width for the visible part is small enough
                # Or use a function to strip color for length calc.
                # For now, just print it directly, it might misalign slightly if lengths vary wildly
                # but our strings are fixed format mostly.
                
                # To fix alignment with colors:
                # 1. Print content
                # 2. Move cursor forward by fixed amount relative to start of cell? No.
                # 3. Use a placeholder for padding.
                
                # Simple approach: Reset color at end, and use a fixed width that is wide enough.
                # But 'CRASH' in RED is 5 chars + codes.
                # Let's just output the pre-formatted string.
                echo -ne "${RESULTS[$idx]}"
                
                # Padding correction:
                # We need to know the visible length.
                vis_len=$(echo -e "${RESULTS[$idx]}" | sed 's/\x1b\[[0-9;]*m//g' | wc -c)
                ((vis_len--)) # wc -c counts newline
                pad=$((10 - vis_len))
                if [ $pad -gt 0 ]; then printf "%${pad}s" ""; fi
            fi
        done
        printf "\n"
    done

    ((spin_idx++))
    sleep 0.1
done

tput cnorm
echo -e "\n${GREEN}All tests completed.${NC}"
rm -rf temp_test_data
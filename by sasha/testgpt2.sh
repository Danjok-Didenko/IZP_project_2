#!/bin/bash

# ==============================================================================
# NASTAVENÍ A POMOCNÉ FUNKCE
# ==============================================================================
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Funkce pro porovnání výstupu se souborem očekávání (pro malé testy)
check_test() {
    local test_name="$1"
    local actual_file="$2"
    local expected_file="$3"

    if diff -w "$actual_file" "$expected_file" > /dev/null; then
        echo -e "${GREEN}[OK] ${test_name}${NC}"
    else
        echo -e "${RED}[FAIL] ${test_name}${NC}"
        echo -e "${YELLOW}   Očekáváno:${NC}"
        cat "$expected_file" | sed 's/^/   /'
        echo -e "${YELLOW}   Získáno:${NC}"
        cat "$actual_file" | sed 's/^/   /'
    fi
}

# Funkce pro kontrolu návratového kódu
check_exit_code() {
    local test_name="$1"
    local code="$2"
    local should_fail="$3"

    if [ "$should_fail" -eq 1 ]; then
        if [ "$code" -ne 0 ]; then
            echo -e "${GREEN}[OK] ${test_name} (Správně selhal, exit code $code)${NC}"
        else
            echo -e "${RED}[FAIL] ${test_name} (Měl selhat, ale vrátil 0)${NC}"
        fi
    else
        if [ "$code" -eq 0 ]; then
            echo -e "${GREEN}[OK] ${test_name}${NC}"
        else
            echo -e "${RED}[FAIL] ${test_name} (Selhal s kódem $code)${NC}"
        fi
    fi
}

# NOVÁ FUNKCE: Kontrola počtu shluků
check_cluster_count() {
    local test_name="$1"
    local actual_file="$2"
    local expected_count="$3"
    local actual_count=$(grep -c "^cluster" "$actual_file")

    if [ "$actual_count" -eq "$expected_count" ]; then
         echo -e "${GREEN}[OK] ${test_name} (Count: $actual_count)${NC}"
    else
         echo -e "${RED}[FAIL] ${test_name} (Count mismatch: exp $expected_count, got $actual_count)${NC}"
    fi
}

# NOVÁ FUNKCE: Kontrola řazení (pomocí Pythonu)
check_sorting() {
    local test_name="$1"
    local file="$2"

    # Python skript, který řádek po řádku ověřuje pravidla řazení
    python3 -c "
import sys
last_first_id = -1
error = 0
line_num = 0

for line in sys.stdin:
    line = line.strip()
    line_num += 1
    if not line or line == 'Clusters:': continue
    if not line.startswith('cluster'): continue
    
    try:
        # Format: cluster X: ID1 ID2 ...
        parts = line.split(':')
        ids = [int(x) for x in parts[1].split()]
        
        if not ids: continue # Empty cluster?

        # 1. Check if IDs inside cluster are sorted
        if ids != sorted(ids):
            print(f'   [!] Error on line {line_num}: IDs not sorted {ids}')
            sys.exit(3)

        # 2. Check if clusters are sorted by first element
        current_first_id = ids[0]
        if current_first_id < last_first_id:
            print(f'   [!] Error on line {line_num}: Cluster order violation ({current_first_id} < {last_first_id})')
            sys.exit(4)
        
        last_first_id = current_first_id
    except ValueError:
        continue

sys.exit(0)
" < "$file"

    local ret=$?
    if [ $ret -eq 0 ]; then
        echo -e "${GREEN}[OK] ${test_name} (Sorting verified)${NC}"
    else
        echo -e "${RED}[FAIL] ${test_name} (Sorting error)${NC}"
    fi
}

# Generátor dat
generate_big_data() {
    local filename="$1"
    local count="$2"
    echo "Generuji data ($count záznamů) s náhodným pořadím ID..."
    
    # Použití Pythonu pro generování a zamíchání (rychlejší než bash loop pro 10k+)
    python3 -c "
import random
import sys

try:
    count = int(sys.argv[1])
    filename = sys.argv[2]
except IndexError:
    sys.exit(1)

# Vytvoření seznamu ID a jeho zamíchání
ids = list(range(1, count + 1))
random.shuffle(ids)

with open(filename, 'w') as f:
    f.write(f'count={count}\n')
    for i in ids:
        # Parametry závisí na hodnotě ID, nikoliv na pozici v souboru
        # Tím zajistíme konzistentní data i při zamíchání
        b = 1000 + (i * 17 % 5000)
        t = 5 + (i % 50)
        pkts = 5 + (i % 20)
        d_raw = 1 + (i % 99)
        d = f'0.{d_raw:02d}'
        ip_suffix = i % 255
        
        f.write(f'{i} 10.0.0.{ip_suffix} 10.0.1.{ip_suffix} {b} {t} {pkts} {d}\n')
" "$count" "$filename"
}

echo -e "${BLUE}=======================================${NC}"
echo -e "${BLUE}      KOMPLEXNÍ TESTY PRO FLOWS        ${NC}"
echo -e "${BLUE}=======================================${NC}"

# 1. KOMPILACE
rm -f flows
cc -std=c11 -Wall -Wextra -Werror -pedantic flows.c -o flows -lm
if [ ! -f ./flows ]; then
    echo -e "${RED}KRITICKÁ CHYBA: Kompilace selhala!${NC}"
    exit 1
fi
echo -e "${GREEN}Kompilace OK.${NC}\n"

# ==============================================================================
# SKUPINA 1-9 (Standardní testy)
# ==============================================================================

# ==============================================================================
# SKUPINA 1: OFICIÁLNÍ PŘÍKLAD ZE ZADÁNÍ
# ==============================================================================
echo -e "${BLUE}--- SKUPINA 1: Příklad ze zadání ---${NC}"

cat > temp_data_ex.txt <<EOF
count=4
10 192.168.1.1 192.168.1.2 2000 10 20 0.05
11 10.0.0.5    10.0.0.6    4000 20 30 0.07
12 192.168.2.1 192.168.2.3 1500 8  10 0.03
13 172.16.0.10 172.16.0.11 6000 25 45 0.06
EOF

cat > temp_exp_ex.txt <<EOF
Clusters:
cluster 0: 10 12
cluster 1: 11 13
EOF

./flows temp_data_ex.txt 2 1.0 1.0 1.0 1.0 > temp_out_ex.txt
check_test "Oficiální příklad (Output format)" temp_out_ex.txt temp_exp_ex.txt


# ==============================================================================
# SKUPINA 2: MATEMATIKA A VÁHY
# ==============================================================================
echo -e "\n${BLUE}--- SKUPINA 2: Logika a Výpočty ---${NC}"

# Vytvoříme dataset pro testování vah:
# ID 10: b=1000, t=10, s=100 (1000/10)
# ID 20: b=200,  t=10, s=100 (200/2)
# ID 30: b=5000, t=50, s=5000 (5000/1)
# ID 40: b=1000, t=90, s=100 (1000/10)
cat > temp_data_math.txt <<EOF
count=4
10 0.0.0.0 0.0.0.0 1000 10 10 0.5
20 0.0.0.0 0.0.0.0 200  10 2  0.5
30 0.0.0.0 0.0.0.0 5000 50 1  0.5
40 0.0.0.0 0.0.0.0 1000 90 10 0.9
EOF

# Test váhy B (Bytes): 10 a 40 mají 1000 bajtů -> měly by být spolu
./flows temp_data_math.txt 3 1.0 0 0 0 > temp_out_m1.txt
cat > temp_exp_m1.txt <<EOF
Clusters:
cluster 0: 10 40
cluster 1: 20
cluster 2: 30
EOF
check_test "Váha 'b' (Total Bytes)" temp_out_m1.txt temp_exp_m1.txt

# Test váhy T (Duration): 10 a 20 mají 10 sec -> měly by být spolu
./flows temp_data_math.txt 3 0 1.0 0 0 > temp_out_m2.txt
cat > temp_exp_m2.txt <<EOF
Clusters:
cluster 0: 10 20
cluster 1: 30
cluster 2: 40
EOF
check_test "Váha 't' (Flow Duration)" temp_out_m2.txt temp_exp_m2.txt

# Test váhy S (Avg Pkt Len - vypočítaný): 10, 20, 40 mají s=100 -> spolu
./flows temp_data_math.txt 2 0 0 0 1.0 > temp_out_m3.txt
cat > temp_exp_m3.txt <<EOF
Clusters:
cluster 0: 10 20 40
cluster 1: 30
EOF
check_test "Váha 's' (Vypočítaná průměrná délka)" temp_out_m3.txt temp_exp_m3.txt


# ==============================================================================
# SKUPINA 3: ŘAZENÍ (SORTING)
# ==============================================================================
echo -e "\n${BLUE}--- SKUPINA 3: Řazení výstupu ---${NC}"

# Vstup je zpřeházený (50 je první). Váha 'b' spojí 10 a 50.
# ID 50: b=100
# ID 10: b=100
# ID 30: b=900
cat > temp_data_sort.txt <<EOF
count=3
50 0.0.0.0 0.0.0.0 100 10 1 0.0
10 0.0.0.0 0.0.0.0 100 10 1 0.0
30 0.0.0.0 0.0.0.0 900 90 1 0.0
EOF

./flows temp_data_sort.txt 2 1.0 0 0 0 > temp_out_sort.txt
# Očekávání: Shluk {10, 50} (seřazeno uvnitř) a Shluk {30}.
# Shluky seřazeny podle nejmenšího prvku (10 < 30).
cat > temp_exp_sort.txt <<EOF
Clusters:
cluster 0: 10 50
cluster 1: 30
EOF
check_test "Řazení ID a Shluků" temp_out_sort.txt temp_exp_sort.txt


# ==============================================================================
# SKUPINA 4: HRANIČNÍ PŘÍPADY (EDGE CASES)
# ==============================================================================
echo -e "\n${BLUE}--- SKUPINA 4: Hraniční případy ---${NC}"

# C1: N = Počet toků (žádné shlukování, ale musí seřadit výstup)
./flows temp_data_sort.txt 3 1 1 1 1 > temp_out_edge1.txt
cat > temp_exp_edge1.txt <<EOF
Clusters:
cluster 0: 10
cluster 1: 30
cluster 2: 50
EOF
check_test "N = Počet toků (Seřazený výpis)" temp_out_edge1.txt temp_exp_edge1.txt

# C2: N = 1 (Vše v jednom)
./flows temp_data_sort.txt 1 1 1 1 1 > temp_out_edge2.txt
cat > temp_exp_edge2.txt <<EOF
Clusters:
cluster 0: 10 30 50
EOF
check_test "N = 1 (Vše sloučeno)" temp_out_edge2.txt temp_exp_edge2.txt

# C3: Soubor s jediným tokem
cat > temp_data_single.txt <<EOF
count=1
99 1.1.1.1 2.2.2.2 500 5 10 0.5
EOF
./flows temp_data_single.txt 1 1 1 1 1 > temp_out_edge3.txt
cat > temp_exp_edge3.txt <<EOF
Clusters:
cluster 0: 99
EOF
check_test "Soubor s jediným tokem" temp_out_edge3.txt temp_exp_edge3.txt

# C4: Nulový počet paketů (Dělení nulou?)
cat > temp_data_zero.txt <<EOF
count=1
99 0.0.0.0 0.0.0.0 0 10 0 0.0
EOF
./flows temp_data_zero.txt 1 1 1 1 1 > /dev/null 2>&1
check_exit_code "Stabilita při 0 paketech (nespadne)" $? 0


# ==============================================================================
# SKUPINA 5: CHYBOVÉ STAVY
# ==============================================================================
echo -e "\n${BLUE}--- SKUPINA 5: Chybové stavy ---${NC}"

# D1: Neexistující soubor
./flows neexistuje.txt 2 1 1 1 1 > /dev/null 2>&1
check_exit_code "Neexistující soubor" $? 1

# D2: Záporné N
./flows temp_data_ex.txt -5 1 1 1 1 > /dev/null 2>&1
check_exit_code "Záporné N" $? 1

# D3: Špatný počet argumentů
./flows temp_data_ex.txt 2 1 1 > /dev/null 2>&1
check_exit_code "Chybějící argumenty" $? 1

# ==============================================================================
# TEST 6: KOMBINOVANÉ VÁHY (Mixed Weights)
# ==============================================================================
echo -e "\n${BLUE}--- SKUPINA 6: Kombinované Váhy ---${NC}"
# Testujeme, zda Euklidovská vzdálenost funguje při více parametrech.
# Flow 10: Bytes=100, Time=10
# Flow 20: Bytes=100, Time=12  (Rozdíl jen v čase = 2)
# Flow 30: Bytes=200, Time=10  (Rozdíl jen v bytech = 100)
#
# Pokud jsou váhy 1.0 a 1.0:
# Dist(10,20) = sqrt(0^2 + 2^2) = 2
# Dist(10,30) = sqrt(100^2 + 0^2) = 100
# -> Měly by se spojit 10 a 20.

cat > data_mixed.txt <<EOF
count=3
10 0.0.0.0 0.0.0.0 100 10 1 0.0
20 0.0.0.0 0.0.0.0 100 12 1 0.0
30 0.0.0.0 0.0.0.0 200 10 1 0.0
EOF

./flows data_mixed.txt 2 1.0 1.0 0 0 > out_mixed.txt
cat > exp_mixed.txt <<EOF
Clusters:
cluster 0: 10 20
cluster 1: 30
EOF
check_test "Kombinace vah (Bytes + Time)" out_mixed.txt exp_mixed.txt


# ==============================================================================
# TEST 7: SINGLE LINKAGE (Efekt řetězení)
# ==============================================================================
echo -e "\n${BLUE}--- SKUPINA 7: Single Linkage Chaining ---${NC}"
# Metoda nejbližšího souseda má specifickou vlastnost "řetězení".
# A je blízko B. B je blízko C. Ale A je daleko od C.
# Single linkage by měl spojit A+B, a pak (AB)+C, protože vzdálenost shluku {A,B} k C
# je min(dist(A,C), dist(B,C)).
#
# Data (hodnota rysu 'b'):
# 10: 100
# 20: 102 (vzdálenost 2)
# 30: 104 (vzdálenost 2 od '20', ale 4 od '10')
# 40: 200 (vzdálenost 96 od '30')
#
# Cíl: 2 shluky.
# Krok 1: Spojí 10 a 20 (vzdálenost 2). Shluky {10,20}, {30}, {40}.
# Krok 2: Vzdálenost {10,20} a {30} je min(dist(10,30), dist(20,30)) = min(4, 2) = 2.
#         Vzdálenost {30} a {40} je 96.
#         Proto spojí {10,20} a {30}.
# Výsledek: {10, 20, 30} a {40}.

cat > data_chain.txt <<EOF
count=4
10 0.0.0.0 0.0.0.0 100 0 1 0.0
20 0.0.0.0 0.0.0.0 102 0 1 0.0
30 0.0.0.0 0.0.0.0 104 0 1 0.0
40 0.0.0.0 0.0.0.0 200 0 1 0.0
EOF

./flows data_chain.txt 2 1.0 0 0 0 > out_chain.txt
cat > exp_chain.txt <<EOF
Clusters:
cluster 0: 10 20 30
cluster 1: 40
EOF
check_test "Single Linkage (řetězení)" out_chain.txt exp_chain.txt


# ==============================================================================
# TEST 8: IDENTICKÉ TOKY (Vzdálenost 0)
# ==============================================================================
echo -e "\n${BLUE}--- SKUPINA 8: Identické toky ---${NC}"
# Co když jsou dva toky úplně stejné? Vzdálenost je 0.
# Musí se sloučit jako první a řazení ID musí stále fungovat.
# ID 50 a ID 5 jsou identické.

cat > data_ident.txt <<EOF
count=3
50 0.0.0.0 0.0.0.0 500 50 1 0.5
5  0.0.0.0 0.0.0.0 500 50 1 0.5
99 0.0.0.0 0.0.0.0 900 90 1 0.9
EOF

./flows data_ident.txt 2 1.0 1.0 1.0 1.0 > out_ident.txt
# 5 a 50 jsou spolu. Seřazeno vzestupně: 5 50.
# Shluk {5, 50} začíná 5. Shluk {99} začíná 99. 5 < 99.
cat > exp_ident.txt <<EOF
Clusters:
cluster 0: 5 50
cluster 1: 99
EOF
check_test "Identické toky (Duplicity)" out_ident.txt exp_ident.txt


# ==============================================================================
# TEST 9: Plovoucí řádová čárka (Precision)
# ==============================================================================
echo -e "\n${BLUE}--- SKUPINA 9: Desetinná přesnost ---${NC}"
# Testujeme rys 'd' (avg_interarrival) s malými rozdíly.
# 10: 0.001
# 20: 0.002 (diff 0.001)
# 30: 0.500 (diff hodně)

cat > data_float.txt <<EOF
count=3
10 0.0.0.0 0.0.0.0 0 0 1 0.001
20 0.0.0.0 0.0.0.0 0 0 1 0.002
30 0.0.0.0 0.0.0.0 0 0 1 0.500
EOF

./flows data_float.txt 2 0 0 1.0 0 > out_float.txt
cat > exp_float.txt <<EOF
Clusters:
cluster 0: 10 20
cluster 1: 30
EOF
check_test "Přesnost floatu (malá čísla)" out_float.txt exp_float.txt

rm -f temp_data_*.txt temp_exp_*.txt temp_out_*.txt data_*.txt out_*.txt exp_*.txt
# ==============================================================================
# SKUPINA 10: STRESS TESTY + VALIDACE ŘAZENÍ
# ==============================================================================
echo -e "\n${BLUE}--- SKUPINA 10: STRESS TESTY & VALIDACE ŘAZENÍ ---${NC}"

# --- TEST 100 ---
DATA_100="data_100.txt"
generate_big_data "$DATA_100" 100

echo -e "\n${YELLOW}[100 items] Test N=100 (No merge):${NC}"
time ./flows "$DATA_100" 100 1 1 1 1 > out_100_full.txt
if [ $? -eq 0 ]; then
    check_cluster_count "Count check" out_100_full.txt 100
    check_sorting "Sorting check" out_100_full.txt
else
    echo -e "${RED}[FAIL] Crash${NC}"
fi

echo -e "\n${YELLOW}[100 items] Test N=10 (Partial merge):${NC}"
time ./flows "$DATA_100" 10 1 1 1 1 > out_100.txt
if [ $? -eq 0 ]; then
    check_cluster_count "Count check" out_100.txt 10
    check_sorting "Sorting check" out_100.txt
else
    echo -e "${RED}[FAIL] Crash${NC}"
fi

echo -e "\n${YELLOW}[100 items] Test N=1 (Full merge):${NC}"
time ./flows "$DATA_100" 1 1 1 1 1 > out_100_1.txt
if [ $? -eq 0 ]; then
    check_cluster_count "Count check" out_100_1.txt 1
    check_sorting "Sorting check" out_100_1.txt
else
    echo -e "${RED}[FAIL] Crash${NC}"
fi

# --- TEST 1000 ---
DATA_1K="data_1k.txt"
generate_big_data "$DATA_1K" 1000

echo -e "\n${YELLOW}[1k items] Test N=1000 (No merge):${NC}"
time ./flows "$DATA_1K" 1000 1 1 1 1 > out_1k_full.txt
if [ $? -eq 0 ]; then
    check_cluster_count "Count check" out_1k_full.txt 1000
    check_sorting "Sorting check" out_1k_full.txt
else
    echo -e "${RED}[FAIL] Crash${NC}"
fi

echo -e "\n${YELLOW}[1k items] Test N=50 (Partial merge):${NC}"
time ./flows "$DATA_1K" 50 1 1 1 1 > out_1k.txt
if [ $? -eq 0 ]; then
    check_cluster_count "Count check" out_1k.txt 50
    check_sorting "Sorting check" out_1k.txt
else
    echo -e "${RED}[FAIL] Crash${NC}"
fi

echo -e "\n${YELLOW}[1k items] Test N=1 (Full merge):${NC}"
time ./flows "$DATA_1K" 1 1 1 1 1 > out_1k_1.txt
if [ $? -eq 0 ]; then
    check_cluster_count "Count check" out_1k_1.txt 1
    check_sorting "Sorting check" out_1k_1.txt
else
    echo -e "${RED}[FAIL] Crash${NC}"
fi

# --- TEST 10000 ---
DATA_10K="data_10k.txt"
generate_big_data "$DATA_10K" 10000

echo -e "\n${YELLOW}[10k items] Test N=10000 (No merge):${NC}"
time ./flows "$DATA_10K" 10000 1 1 1 1 > out_10k_full.txt
if [ $? -eq 0 ]; then
    check_cluster_count "Count check" out_10k_full.txt 10000
    check_sorting "Sorting check" out_10k_full.txt
else
    echo -e "${RED}[FAIL] Crash${NC}"
fi

echo -e "\n${YELLOW}[10k items] Test N=100 (Partial merge):${NC}"
time ./flows "$DATA_10K" 100 1 1 1 1 > out_10k.txt
if [ $? -eq 0 ]; then
    check_cluster_count "Count check" out_10k.txt 100
    check_sorting "Sorting check" out_10k.txt
else
    echo -e "${RED}[FAIL] Crash${NC}"
fi

echo -e "\n${YELLOW}[10k items] Test N=1 (Full merge):${NC}"
echo -e "${CYAN}Check speed (max 10-15s for optimized solution)${NC}"
time ./flows "$DATA_10K" 1 1 1 1 1 > out_10k_1.txt
if [ $? -eq 0 ]; then
    check_cluster_count "Count check" out_10k_1.txt 1
    check_sorting "Sorting check" out_10k_1.txt
else
    echo -e "${RED}[FAIL] Crash${NC}"
fi

echo -e "\n${BLUE}=== TESTOVÁNÍ DOKONČENO ===${NC}"
# rm -f temp_*.txt out_*.txt data_*.txt
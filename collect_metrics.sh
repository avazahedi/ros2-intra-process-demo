#!/usr/bin/env bash
# Measures one ROS 2 process in repeated runs and appends one summary row per run to a CSV.
#
# Usage: ./collect_metrics.sh <process_name|pid> <label> [latency_csv=latency.csv] [duration=10] [runs=30]
# Example: ./collect_metrics.sh image_subscriber baseline latency.csv 10 30
#
# Latency, message rate and bandwidth come from the subscriber's latency CSV
# (columns: stamp_seconds,latency_ms,size_bytes), restricted to each run's time window.
# CPU comes from pidstat.
# Output file: $SUMMARY (default: metrics_summary.csv).

set -u

TARGET=${1:?Usage: $0 <process_name|pid> <label> [latency_csv] [duration] [runs]}
LABEL=${2:?Usage: $0 <process_name|pid> <label> [latency_csv] [duration] [runs]}
LAT=${3:-latency.csv}
DUR=${4:-10}
RUNS=${5:-30}
SUMMARY=${SUMMARY:-metrics_summary.csv}

command -v pidstat >/dev/null || { echo "Missing required command: pidstat (sudo apt install sysstat)" >&2; exit 1; }
[[ -f $LAT ]] || { echo "Latency file not found: $LAT" >&2; exit 1; }

# Resolve the PID of the actual executable (not the `ros2 run` wrapper).
if [[ $TARGET =~ ^[0-9]+$ ]]; then
  PID=$TARGET
else
  PID=$(pgrep -f "/${TARGET}( |\$)" | head -n1)
fi
if [[ -z ${PID:-} ]] || ! kill -0 "$PID" 2>/dev/null; then
  echo "Could not find a running process for '$TARGET'" >&2
  exit 1
fi

# Create a temp file for storing data for each run
WINDOW=$(mktemp)
trap 'rm -f "$WINDOW"' EXIT

# Average %CPU of $PID from `pidstat -h -u` output (header line starts with '#').
avg_cpu() {
  awk -v pid="$PID" '
    /^#/ { sub(/^#/, ""); for (i = 1; i <= NF; i++) { if ($i == "PID") p = i; if ($i == "%CPU") c = i } next }
    p && $1 !~ /^Average/ && $p == pid { s += $c; n++ }
    END { if (n) printf "%.2f\n", s / n; else print "n/a" }'
}

# Message size in bytes, taken from the first row of the latency CSV.
BYTES=$(awk -F, 'NR == 2 { print $3; exit }' "$LAT")

HEADER="label,run,timestamp,process,window_s,msgs,dropped_msgs,rate_hz,bandwidth_mib_s,latency_mean_ms,latency_p50_ms,latency_p95_ms,latency_p99_ms,latency_max_ms,cpu_pct,cpu_ms_per_msg"
[[ -s $SUMMARY ]] || echo "$HEADER" > "$SUMMARY"

for RUN in $(seq 1 "$RUNS"); do
  if ! kill -0 "$PID" 2>/dev/null; then
    echo "Process $PID exited; stopping after $((RUN - 1)) run(s)." >&2
    break
  fi
  echo "Run $RUN/$RUNS: measuring PID $PID ($TARGET) for ${DUR}s..."

  START=$(date +%s.%N)
  CPU=$(LC_ALL=C pidstat -h -u -p "$PID" 1 "$DUR" | avg_cpu)
  END=$(date +%s.%N)
  W=$(awk -v s="$START" -v e="$END" 'BEGIN { printf "%.3f", e - s }')

  # "stamp latency_ms" pairs for messages that arrived inside this run's window
  awk -F, -v s="$START" -v e="$END" \
    'NR > 1 && $1 + 0 >= s + 0 && $1 + 0 <= e + 0 { print $1, $2 }' "$LAT" > "$WINDOW"

  read -r MSGS LAT_MEAN LAT_P50 LAT_P95 LAT_P99 LAT_MAX < <(awk '{ print $2 }' "$WINDOW" | sort -n | awk '
    function ceil(x) { return (x == int(x)) ? x : int(x) + 1 }
    function pct(p,  i) { i = ceil(p * n); if (i < 1) i = 1; return a[i] }
    { a[NR] = $1; s += $1 }
    END {
      n = NR
      if (!n) { print 0, "n/a", "n/a", "n/a", "n/a", "n/a"; exit }
      printf "%d %.3f %.3f %.3f %.3f %.3f\n", n, s / n, pct(0.50), pct(0.95), pct(0.99), a[n]
    }')

  # Dropped messages: expected count (from typical stamp spacing) minus received count
  MED=$(awk 'NR > 1 { print $1 - prev } { prev = $1 }' "$WINDOW" | sort -n \
    | awk '{ a[NR] = $1 } END { print (NR ? a[int((NR + 1) / 2)] : 0) }')
  DROPPED=$(awk -v med="$MED" -v n="$MSGS" '
    NR == 1 { f = $1 } { l = $1 }
    END { if (n < 2 || med <= 0) print "n/a"; else { d = int((l - f) / med + 0.5) + 1 - n; print (d < 0 ? 0 : d) } }' "$WINDOW")

  if [[ $MSGS -eq 0 ]]; then
    echo "WARNING: no latency samples fell inside the window. Is the subscriber running," >&2
    echo "         is '$LAT' the right file, and is the subscriber flushing it (std::endl)?" >&2
  fi

  # Rate (Hz), payload bandwidth (MiB/s) and CPU time per message (ms)
  read -r RATE BW CPU_MS_PER_MSG < <(awk -v m="$MSGS" -v w="$W" -v b="$BYTES" -v c="$CPU" '
    function num(x) { return x ~ /^[0-9.]+$/ }
    BEGIN {
      rate = m / w
      printf "%.3f %s %s\n", rate,
        num(b) ? sprintf("%.3f", rate * b / 1048576) : "n/a",
        (num(c) && m > 0) ? sprintf("%.3f", c * w * 10 / m) : "n/a"
    }')

  echo "$LABEL,$RUN,$(date -Iseconds),$TARGET,$W,$MSGS,$DROPPED,$RATE,$BW,$LAT_MEAN,$LAT_P50,$LAT_P95,$LAT_P99,$LAT_MAX,$CPU,$CPU_MS_PER_MSG" >> "$SUMMARY"
  printf "  -> msgs=%s  latency_mean=%s ms  cpu=%s%%  cpu_ms/msg=%s\n" "$MSGS" "$LAT_MEAN" "$CPU" "$CPU_MS_PER_MSG"
done

echo
echo "Done. Rows appended to $SUMMARY"

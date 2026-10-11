# ros2-intra-process-demo
Two packages and a script for measuring regular data transport vs. intra-process data transport in ROS 2.

## Usage
### Regular Data Transport
Build and launch:
```
colcon build --packages-select regular_data_transport
ros2 launch regular_data_transport data_transport.launch.xml
```

Optional arguments:
- `image_path`: Defaults to "1920x1080.png"
    - The image filepath for the publisher to publish
- `log_to_console`: Defaults to false
    - Print logs of the data's memory address, size, and dimensions
- `log_to_csv`: Defaults to false
    - Write to a csv tracking the timestamp, latency, and size

### Zero-Copy Data Transport
Build and launch:
```
colcon build --packages-select zero_copy_data_transport
ros2 launch zero_copy_data_transport intra_proc.launch.xml
```

Optional arguments:
- `image_path`: Defaults to "1920x1080.png"
    - The image filepath for the publisher to publish
- `log_to_console`: Defaults to false
    - Print logs of the data's memory address, size, and dimensions
- `log_to_csv`: Defaults to false
    - Write to a csv tracking the timestamp, latency, and size

### Metrics Script
This script can be run alongside one of the above launch files when `log_to_csv` is true.

By default, this monitors 30 runs of 10-seconds each and calculates averages for each run.

It will generate a metrics_summary.csv containing metrics including the following:
- Rate (Hz)
- Bandwidth (MiB/s)
- Latency (ms)
- CPU usage

After having launched one of the above launch files, run:
```
# Usage:
# ./collect_metrics.sh <process_name|pid> <label> [latency_csv=latency.csv] [duration=10] [runs=30]

# Example:
./collect_metrics.sh image_subscriber baseline latency.csv 10 30
```

Note that for the zero-copy runs, the process_name will be `component_container`.

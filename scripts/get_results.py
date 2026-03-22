import re
import os
from collections import defaultdict
from statistics import mean, stdev

def analyze_ns3_logs(log_file_path, *, verbose=False):
    # Data structures to hold our parsed information
    # alert_data[name] = {'start_time': float, 'veh_count': int, 'receivers': set(), 'delays': []}
    alert_data = {}
    
    # Global counters for Forwarder metrics
    duplicate_drops = 0
    processed_deferred = 0

    # Regex patterns for the three components
    producer_re = re.compile(r"\+(?P<time>[\d\.]+)s\s+(?P<node>\d+)\s+ndn\.RandomAlertProducer.*Send alert=(?P<name>\S+)\s+vehCount=(?P<vc>\d+)")
    consumer_re = re.compile(r"\+(?P<time>[\d\.]+)s\s+(?P<node>\d+)\s+ndn\.AlertConsumer.*Received alert:\s+(?P<name>\S+)")
    
    # Forwarder regex
    dup_drop_re = re.compile(r"decision=drop because duplicate alert")
    deferred_re = re.compile(r"decision=deferred until")

    with open(log_file_path, 'r') as f:
        for line in f:
            # 1. Check for Producer (Start of a packet lifecycle)
            p_match = producer_re.search(line)
            if p_match:
                name = p_match.group('name')
                alert_data[name] = {
                    'start_time': float(p_match.group('time')),
                    'veh_count': int(p_match.group('vc')),
                    'receivers': set(),
                    'delays': []
                }
                continue

            # 2. Check for Consumer (Delivery and Delay)
            c_match = consumer_re.search(line)
            if c_match:
                name = c_match.group('name')
                curr_time = float(c_match.group('time'))
                node_id = c_match.group('node')
                
                if name in alert_data:
                    # Only count unique receivers for PDR
                    if node_id not in alert_data[name]['receivers']:
                        alert_data[name]['receivers'].add(node_id)
                        # Calculate delay for this specific receiver
                        delay = curr_time - alert_data[name]['start_time']
                        alert_data[name]['delays'].append(delay)
                continue

            # 3. Check for Forwarder (Duplicate Rate)
            if dup_drop_re.search(line):
                duplicate_drops += 1
            elif deferred_re.search(line):
                processed_deferred += 1

    # Final Metric Calculations
    total_packets = len(alert_data)
    failed_packets = 0
    results = []

    if verbose:
        print(f"{'Alert Name':<40} | {'PDR (%)':<10} | {'Avg Delay (ms)':<15}")
        print("-" * 75)

    global_pdr = []
    global_delay = []

    for name, data in alert_data.items():
        # PDR Calculation
        pdr = (len(data['receivers']) / data['veh_count']) * 100 if data['veh_count'] > 0 else 0
        
        # Failure Rate tracking
        if len(data['receivers']) == 0:
            failed_packets += 1
            
        # Delay Calculation (in milliseconds)
        avg_delay = (sum(data['delays']) / len(data['delays']) * 1000) if data['delays'] else 0

        global_pdr.append(pdr)
        global_delay.append(avg_delay)

        if verbose:
            print(f"{name[:40]:<40} | {pdr:<10.2f} | {avg_delay:<15.4f}")

    # Aggregated Metrics
    failure_rate = (failed_packets / total_packets) * 100 if total_packets > 0 else 0
    duplicate_rate = (duplicate_drops / (duplicate_drops + processed_deferred)) * 100 if processed_deferred > 0 else 0

    pdr_mean = mean(global_pdr)
    delay_mean = mean(global_delay)
    delay_std = stdev(global_delay)

    if verbose:
        print("\n" + "="*30)
        print(f"SUMMARY METRICS")
        print("="*30)
        print(f"Total Unique Packets Sent: {total_packets}")
        print(f"Global Failure Rate:       {failure_rate:.2f}%")
        print(f"Global Duplicate Rate:     {duplicate_rate:.2f}%")
        print(f"PDR:                       {pdr_mean:.2f}%")
        print(f"Delay (Mean ± Std):        {delay_mean:.4f} ms ± {delay_std:.4f} ms")
        print("="*30)

    scenario, mode = log_file_path.split('/')[1].split('-')
    mode = mode.split('.')[0]
    values = f"{pdr_mean:.2f}%,{delay_mean:.4f} ms ± {delay_std:.4f} ms,{failure_rate:.2f}%,{duplicate_rate:.2f}%"
    return ','.join([scenario, mode, values])


def generate_results(filename):
    with open(filename, 'w') as file:
        file.write('Network Scenario,Vehicle Density,PDR,Delay,Failure Rate,Duplicate Rate\n')
        for filename in sorted(os.listdir('log')):
            file.write(analyze_ns3_logs('log/' + filename) + '\n')


if __name__ == "__main__":
    generate_results('results.csv')

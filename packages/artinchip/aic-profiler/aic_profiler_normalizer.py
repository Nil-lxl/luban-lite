#!/usr/bin/env python3
# Copyright (c) 2025, ArtInChip Technology Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#
# Authors:  Zequan Liang <zequan.liang@artinchip.com>

"""AIC Profiler Data Normalization Tool"""
import re
import sys
from typing import List, Tuple, Optional


class AICProfilerNormalizer:

    def __init__(self):
        self.systrace_header = [
            "# tracer: nop\n",
            "#\n"
        ]

        self.patterns = [
            # 1. Full format: AIC-1 [0] 123.456789: tracing_mark_write: B|1|func
            re.compile(r'^\s*AIC-(-?\d+)\s+\[(-?\d+)\]\s+(-?\d+)\.(-?\d+):'
                       r'\s+tracing_mark_write:\s+([BE])\|1\|(.+)$'),
            # 2. Compact full format: AIC-1[0]123.456789:tracing_mark_write:B|1|func
            re.compile(r'^\s*AIC-(-?\d+)\[(-?\d+)\](-?\d+)\.(-?\d+):'
                       r'tracing_mark_write:([BE])\|1\|(.+)$'),
            # 3. Missing AIC prefix but complete: 1 [0] 123.456789: tracing_mark_write: B|1|func
            re.compile(r'^\s*(-?\d+)\s+\[(-?\d+)\]\s+(-?\d+)\.(-?\d+):'
                       r'\s+tracing_mark_write:\s+([BE])\|1\|(.+)$'),
            # 4. Compact missing AIC prefix: 1[0]123.456789:tracing_mark_write:B|1|func
            re.compile(r'^\s*(-?\d+)\[(-?\d+)\](-?\d+)\.(-?\d+):'
                       r'tracing_mark_write:([BE])\|1\|(.+)$'),
            # 5. Simplified format with AIC: AIC-1 [0] 123.456789: B|1|func
            re.compile(r'^\s*AIC-(-?\d+)\s+\[(-?\d+)\]\s+(-?\d+)\.(-?\d+):'
                       r'\s+([BE])\|1\|(.+)$'),
            # 6. Compact simplified format: AIC-1[0]123.456789:B|1|func
            re.compile(r'^\s*AIC-(-?\d+)\[(-?\d+)\](-?\d+)\.(-?\d+):([BE])\|1\|(.+)$'),
            # 7. Simplified format: 1 [0] 123.456789: B|1|func
            re.compile(r'^\s*(-?\d+)\s+\[(-?\d+)\]\s+(-?\d+)\.(-?\d+):\s+([BE])\|1\|(.+)$'),
            # 8. Compact simplified format: 1[0]123.456789:B|1|func
            re.compile(r'^\s*(-?\d+)\[(-?\d+)\](-?\d+)\.(-?\d+):([BE])\|1\|(.+)$'),
        ]

    def has_header(self, lines: List[str]) -> bool:
        return len(lines) > 0 and lines[0].startswith("# tracer:")

    def parse_line(self, line: str) -> Optional[Tuple]:
        line = line.strip()
        if not line or line.startswith('#'):
            return None

        for pattern in self.patterns:
            match = pattern.match(line)
            if match:
                groups = match.groups()
                if len(groups) == 6:
                    tid, cpu, sec, usec, tag, func = groups
                    usec_padded = usec.ljust(6, '0')[:6]
                    tid = int(tid)
                    cpu = int(cpu)
                    sec = int(sec)
                    return (tid, cpu, sec, usec_padded, tag, func)
        return None

    def normalize_line(self, parsed_data: Tuple) -> str:
        tid, cpu, sec, usec, tag, func = parsed_data
        tid_str = f"{tid}" if tid >= 0 else f"{tid}"
        cpu_str = f"{cpu}" if cpu >= 0 else f"{cpu}"
        sec_str = f"{sec}" if sec >= 0 else f"{sec}"
        return (f"   AIC-{tid_str} [{cpu_str}] {sec_str}.{usec}: "
                f"tracing_mark_write: {tag}|1|{func}\n")

    def process(self, input_data: str) -> str:
        lines = input_data.split('\n')
        output_lines = []

        # Check and add header information
        has_header = any(line.strip().startswith("# tracer:") for line in lines[:2])
        if not has_header:
            output_lines.extend(self.systrace_header)

        for line in lines:
            line = line.strip()
            parsed = self.parse_line(line)
            if not line:  # Empty line
                continue
            elif line.startswith('#'):  # Comment line
                output_lines.append(line + '\n')
            elif parsed:  # Valid data line
                output_lines.append(self.normalize_line(parsed))
            else:  # Unrecognized line
                output_lines.append(f"# {line}\n")

        return ''.join(output_lines)

    def process_file(self, input_file: str, output_file: str):
        try:
            with open(input_file, 'r', encoding='utf-8', errors='ignore') as f:
                input_data = f.read()

            output_data = self.process(input_data)

            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(output_data)

            print(f"Processing completed: {input_file} -> {output_file}")

        except Exception as e:
            print(f"Error: {e}")


def main():
    if len(sys.argv) == 3:
        normalizer = AICProfilerNormalizer()
        normalizer.process_file(sys.argv[1], sys.argv[2])
    else:
        print("Usage: python3 aic_profiler_normalizer.py input.txt output.systrace")


if __name__ == "__main__":
    main()

# AIC Profiler Documentation

## Overview
AIC Profiler is a lightweight embedded system tool for capturing function execution timing. It outputs data in systrace format for performance analysis and debugging.

## Purpose
- Identify performance bottlenecks in code
- Trace function calls and timing in real-time systems
- Generate data viewable on standard tools like Perfetto

## Usage

### Initialization

aic_profiler_config_t config;
aic_profiler_config_init(&config);  // Set defaults
aic_profiler_init(&config);         // Start profiler


### Profiling
- Mark code sections with macros:

  AIC_PROFILER_BEGIN;
  // Code to measure
  AIC_PROFILER_END;

- For interrupts, use `AIC_PROFILER_IRQ_BEGIN` and `AIC_PROFILER_IRQ_END`

### Shutdown
Always clean up to ensure data is saved and files are closed:

aic_profiler_flush();    // Write remaining data
aic_profiler_uninit();   // Close file and free resources


### Viewing Results
1. Upload the output file (default: `/udisk/profiler.systrace`) to [ui.perfetto.dev](https://ui.perfetto.dev/)
2. **Use the Python normalization script if:**
   - The output file has formatting issues or isn't recognized by Perfetto
   - You need to convert raw profiler data into valid systrace format
   - Example: `python3 aic_profiler_normalizer.py input.txt output.systrace`

## Key Notes
- Configurable buffer size and output location
- Supports Linux and RT-Thread environments
- Call `uninit()` to prevent data loss and file handle leaks
```

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Kconfig to autoconf.h mapping generation script

Features:
1. Recursively parse Kconfig and all sub-files introduced by source
2. Extract all config/menuconfig LPKG_* configurations and their types
3. Group by menu / endmenu ranges
4. Generate CONFIG_* mappings wrapped with #ifdef
   - bool type  -> #define CONFIG_XXX 1
   - non-bool type -> #define CONFIG_XXX LPKG_XXX
"""

import os
import re
import sys


class KconfigParser:
    def __init__(self):
        # sections maintains file parsing order; each section records its menu and config list
        self.sections = []  # [{'menu': str|None, 'configs': [(name, type), ...]}, ...]
        self.menu_stack = []

    def _current_menu(self):
        """Return the current menu name at the top of the stack, or None if not in any menu"""
        return self.menu_stack[-1] if self.menu_stack else None

    def _ensure_section(self):
        """Ensure a corresponding section exists when the menu state changes"""
        current = self._current_menu()
        if not self.sections or self.sections[-1]['menu'] != current:
            self.sections.append({'menu': current, 'configs': []})

    def _find_config_type(self, lines, start_idx):
        """Search for the type (bool/string/int/hex) within a few lines after the config definition"""
        j = start_idx + 1
        while j < len(lines) and j < start_idx + 10:
            line = lines[j].strip()
            # Stop searching when the next config/menuconfig/choice/menu/endmenu is encountered
            if re.match(r'^(config|menuconfig|choice|menu|endmenu)\b', line):
                break
            type_match = re.match(r'^(bool|string|int|hex)\b', line)
            if type_match:
                return type_match.group(1)
            j += 1
        return 'unknown'

    def _resolve_source_path(self, base_dir, source_path):
        """Resolve the source path: if it starts with packages/third-party/zephyr-bluetooth/,
        strip the prefix and look it up relative to the current file's directory;
        otherwise look it up directly relative to the current directory."""
        prefix = "packages/third-party/zephyr-bluetooth/"
        if source_path.startswith(prefix):
            source_path = source_path[len(prefix):]

        candidate = os.path.join(base_dir, source_path)
        if os.path.exists(candidate):
            return candidate

        # If still not found, try relative to the working directory
        candidate = os.path.join(os.getcwd(), source_path)
        if os.path.exists(candidate):
            return candidate

        return os.path.join(base_dir, source_path)

    def parse_file(self, filepath):
        """Recursively parse a single Kconfig file"""
        if not os.path.exists(filepath):
            print(f"// Warning: file not found: {filepath}", file=sys.stderr)
            return

        base_dir = os.path.dirname(filepath)
        with open(filepath, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        i = 0
        while i < len(lines):
            line = lines[i]
            stripped = line.strip()

            # menu "Name"
            if stripped.startswith('menu '):
                menu_name = stripped[5:].strip().strip('"')
                self.menu_stack.append(menu_name)
                self._ensure_section()
                i += 1
                continue

            # endmenu
            if stripped == 'endmenu':
                if self.menu_stack:
                    self.menu_stack.pop()
                self._ensure_section()
                i += 1
                continue

            # source "path/to/Kconfig"
            if stripped.startswith('source '):
                source_path = stripped[7:].strip().strip('"')
                resolved = self._resolve_source_path(base_dir, source_path)
                self.parse_file(resolved)
                # After source returns, ensure the current section matches the current menu state
                self._ensure_section()
                i += 1
                continue

            # config LPKG_XXX or menuconfig LPKG_XXX
            match = re.match(r'^(menu)?config\s+(LPKG_\w+)', stripped)
            if match:
                config_name = match.group(2)
                config_type = self._find_config_type(lines, i)
                self._ensure_section()
                self.sections[-1]['configs'].append((config_name, config_type))
                i += 1
                continue

            i += 1

    def _format_config(self, name, ctype):
        """Format the mapping for a single configuration"""
        config_name = name.replace('LPKG_', 'CONFIG_', 1)
        if ctype == 'bool':
            return f'#ifdef {name}\n#define {config_name} 1\n#endif'
        else:
            return f'#ifdef {name}\n#define {config_name} {name}\n#endif'

    def generate(self):
        """Generate the final autoconf.h mapping text"""
        output_lines = []

        output_lines.append(f'/*')
        output_lines.append(f' * Copyright (c) 2026-2026, ArtInChip Technology Co., Ltd')
        output_lines.append(f' *')
        output_lines.append(f' * SPDX-License-Identifier: Apache-2.0')
        output_lines.append(f' */')
        output_lines.append(f'')

        output_lines.append(f'#ifndef LPKG_ZEPHYR_BLUETOOTH_AUTOCONF_H')
        output_lines.append(f'#define LPKG_ZEPHYR_BLUETOOTH_AUTOCONF_H')
        output_lines.append(f'')
        output_lines.append(f'#include <rtconfig.h>')
        output_lines.append(f'#include <aic_errno.h>')
        output_lines.append(f'')

        for section in self.sections:
            menu = section['menu']
            configs = section['configs']
            if not configs:
                continue

            if menu:
                output_lines.append(f'')
                output_lines.append(f'/* {menu} */')
                for name, ctype in configs:
                    output_lines.append(self._format_config(name, ctype))
                output_lines.append(f'/* end of {menu} */')
                output_lines.append(f'')
            else:
                for name, ctype in configs:
                    output_lines.append(self._format_config(name, ctype))

        output_lines.append(f'#endif /* LPKG_ZEPHYR_BLUETOOTH_AUTOCONF_H */')

        return '\n'.join(output_lines)


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <Kconfig_file> [output.h]", file=sys.stderr)
        sys.exit(1)

    kconfig_path = sys.argv[1]
    parser = KconfigParser()
    parser.parse_file(kconfig_path)

    result = parser.generate()

    if len(sys.argv) >= 3:
        output_path = sys.argv[2]
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(result + '\n')
        print(f"Generated: {output_path}")
    else:
        print(result)


if __name__ == '__main__':
    main()

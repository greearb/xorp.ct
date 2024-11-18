#!/usr/bin/env python3
import re
from sys import argv

def build_target_template(src_file, dest_file, firewall, click):
    """
    Create new fea.tp file from fea.tp.raw depending on build config
    Skip firewall dependecy and click section if disabled in CMake
    """
    print(f"{src_file} -> {dest_file}: {firewall=}, {click=}")
    with open(src_file, mode='r', encoding='utf-8') as src:
        with open(dest_file, mode='w', encoding='utf-8') as dest:
            firewall_ln = re.compile(".*firewall.*")
            opened_brackets = 0
            opened_bracket_ln = re.compile(".*{.*")
            closed_bracket_ln = re.compile(".*}.*")
            click_ln = re.compile(".*click.*")
            out_lines = []
            for line in src.readlines():
                if opened_brackets > 0:
                    # Enter only within click section
                    if opened_bracket_ln.match(line):
                        opened_brackets += 1
                    if closed_bracket_ln.match(line):
                        opened_brackets -= 1
                    # Skip any line until opened_brackets decrement back to 0
                    continue
                if (not firewall) and firewall_ln.match(line):
                    # Skip any line containing "firewall"
                    continue
                if (not click) and click_ln.match(line):
                    opened_brackets = 1
                    # Skip any line after "click" until next closing bracket
                    # reduce currently opened section
                    continue
                out_lines.append(line)
            if out_lines[-1] == '\n':
                # Trim trailing newline
                out_lines = out_lines[:-1]
            dest.writelines(out_lines)
    return 0


def main():
    if len(argv) < 5:
        return 1
    source_file = argv[1]
    destination_file = argv[2]
    firewall_enable = (argv[3] == 'ON')
    click_enable = (argv[4] == 'ON')
    return build_target_template(source_file, destination_file,
                                 firewall_enable, click_enable)


if __name__ == '__main__':
    main()

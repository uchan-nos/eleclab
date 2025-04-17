#!/usr/bin/python3

import collections
from dataclasses import dataclass
import datetime
from pathlib import Path
import re
import subprocess

# それぞれの関数の情報を表す型
@dataclass
class FuncInfo:
    name: str
    stack_usage: int
    calls: list
    stack_total: int

SU_LINE_PAT = re.compile(r'^[^:]+:\d+:\d+:(?P<id>\S+)\s+(?P<su>\d+)\s+(?P<type>\w+)')
CFLOW_LINE_PAT = re.compile(r'^(?P<indent>\s*)(?P<id>[_0-9a-zA-Z]+)\(\).*$')

ECHO_CH32FUN_C_MK = '''
include ../env.mk
all:
	@echo $(CH32V003FUN)/ch32fun/ch32fun.c
'''

verbose = False

def read_latest_su_lines():
    su_list = Path('/tmp').glob('msmpdbg.*.su')
    latest_su = max(su_list, key=lambda x: x.stat().st_mtime)
    latest_su_dt = datetime.datetime.fromtimestamp(latest_su.stat().st_mtime)
    print(f'Reading stack usage from {latest_su} (mtime={latest_su_dt})')
    with open(latest_su) as f:
        lines = f.readlines()
    return lines

def get_stack_usage(su_lines):
    '''各行は次の形式
    msmpdbg.c:330:6:StartTransmit   0       static
    '''
    stack_usage = {}
    for line in su_lines:
        m = SU_LINE_PAT.match(line)
        if not m:
            raise ValueError(f'Invalid line format: {line}')
        fi = FuncInfo(m.group('id'), int(m.group('su')), set(), None)
        stack_usage[m.group('id')] = fi
    return stack_usage

def get_ch32fun_path():
    # ../env.mk に含まれる ch32fun.c のパスを取得
    ch32fun_path = subprocess.run(['make', '-f', '-', 'all'], input=ECHO_CH32FUN_C_MK, capture_output=True, text=True)
    if ch32fun_path.returncode != 0:
        raise RuntimeError('Failed to get ch32fun.c path')
    return ch32fun_path.stdout.strip()

def generate_cflow_lines(target_cfiles):
    args = target_cfiles + [get_ch32fun_path()]
    print('Generating cflow by using', ' '.join(args))
    cflow = subprocess.run(['cflow'] + args, capture_output=True, text=True)
    return cflow.stdout.splitlines()

# コールグラフを構築し、トップの関数名を返す
def get_call_graph(funcs, cflow_lines):
    # cflow を呼び出し、標準出力を取得
    call_stack = None
    indent = -1
    indent_width = None
    funcs_not_found = []
    for line in cflow_lines:
        '''各行は次の形式
        Funcname() <void FuncName (int arg1, int arg2) at src.c:123>:
        '''
        m = CFLOW_LINE_PAT.match(line)
        if not m:
            raise ValueError(f'Invalid line format: {line}')

        func_name = m.group('id')
        if func_name not in funcs:
            funcs[func_name] = FuncInfo(func_name, None, set(), None)
            funcs_not_found.append(func_name)
        func = funcs[func_name]
        new_indent = len(m.group('indent'))

        if indent == -1:
            call_stack = [func.name]
            indent = new_indent
            continue

        if indent_width is None:
            indent_width = new_indent - indent
        new_indent = new_indent // indent_width

        if indent < new_indent:
            call_stack.append(func.name)
            funcs[call_stack[-2]].calls.add(func.name)
        elif indent == new_indent:
            call_stack[-1] = func.name
            funcs[call_stack[-2]].calls.add(func.name)
        else:
            while new_indent < indent:
                call_stack.pop()
                indent -= 1
            call_stack[-1] = func.name
            funcs[call_stack[-2]].calls.add(func.name)
        indent = new_indent
    
    if verbose:
        print('Functions without stack usage info:')
        print('  ' + '\n  '.join(funcs_not_found))
        print('Stack usage of these functions assumed to be 0')
    return call_stack[0]

def calc_stack_total(func_name, funcs):
    func = funcs[func_name]
    if func.stack_total is not None:
        return func.stack_total

    sub_stack_max = 0
    for call in func.calls:
        calc_stack_total(call, funcs)
        if funcs[call].stack_total > sub_stack_max:
            sub_stack_max = funcs[call].stack_total
    func.stack_total = (0 if func.stack_usage is None else func.stack_usage) + sub_stack_max

def calc_print_width(func_name, funcs, indent):
    width_max = 2 * indent + len(func_name)
    for call in funcs[func_name].calls:
        sub_width = calc_print_width(call, funcs, indent + 1)
        if sub_width > width_max:
            width_max = sub_width
    return width_max

def print_stack_usage_tree(func_name, funcs, indent, print_width):
    func = funcs[func_name]
    indent_spaces = '  ' * indent
    spaces = ' ' * (print_width - len(indent_spaces) - len(func_name))
    print(f'{indent_spaces}{func_name}{spaces}  [{func.stack_usage} total={func.stack_total}]')
    for call in func.calls:
        print_stack_usage_tree(call, funcs, indent + 1, print_width)

def main():
    su_lines = read_latest_su_lines()
    funcs = get_stack_usage(su_lines)
    cflow_lines = generate_cflow_lines(['msmpdbg.c', 'msmp_recorder.c'])
    main_func_name = get_call_graph(funcs, cflow_lines)
    calc_stack_total(main_func_name, funcs)
    width = calc_print_width(main_func_name, funcs, 0)
    if verbose:
        print_stack_usage_tree(main_func_name, funcs, 0, width)

    print(f'Estimated total stack usage: {funcs[main_func_name].stack_total} bytes')

if __name__ == '__main__':
    main()
#!/usr/bin/env python

import argparse
import io
import json
import multiprocessing
import os
import re
import subprocess
import string
import sys

threading_factor = int(multiprocessing.cpu_count() * 1.5)

parser = argparse.ArgumentParser(description='Executes 2nd pass of XTU analysis')
parser.add_argument('-b', required=True, dest='buildlog', metavar='build.json', help='Use a JSON Compilation Database')
parser.add_argument('-g', required=True, dest='buildgraph', metavar='build-graph.json', help='Use a JSON Build Dependency Graph')
parser.add_argument('-p', metavar='preanalyze-dir', dest='xtuindir', help='Use directory for reading preanalyzation data (default=".xtu")', default='.xtu')
parser.add_argument('-o', metavar='output-dir', dest='xtuoutdir', help='Use directory for output analyzation results (default=".xtu-out")', default='.xtu-out')
parser.add_argument('-e', metavar='enabled-checker', nargs='+', dest='enabled_checkers', help='List all enabled checkers')
parser.add_argument('-d', metavar='disabled-checker', nargs='+', dest='disabled_checkers', help='List all disabled checkers')
parser.add_argument('-j', metavar='threads', dest='threads', help='Number of threads used (default=' + str(threading_factor) + ')', default=threading_factor)
parser.add_argument('-v', dest='verbose', action='store_true', help='Verbose output of every command executed')
parser.add_argument('--clang-path', metavar='clang-path', dest='clang_path', help='Set path of clang binaries to be used (default taken from CLANG_PATH environment variable)', default=os.environ.get('CLANG_PATH', '.'))
parser.add_argument('--ccc-analyzer-path', metavar='ccc-analyzer-path', dest='ccc_path', help='Set path of ccc-analyzer to be used (default is current directory)', default='.')
mainargs = parser.parse_args()

clang_path = os.path.abspath(mainargs.clang_path)
if mainargs.verbose :
    print 'XTU uses clang dir: ' + clang_path

ccc_path = os.path.abspath(mainargs.ccc_path)
if mainargs.verbose :
    print 'XTU uses ccc-analyzer dir: ' + ccc_path

analyzer_params = []
if mainargs.enabled_checkers:
    analyzer_params += [ '-analyzer-checker', mainargs.enabled_checkers ]
if mainargs.disabled_checkers:
    analyzer_params += [ '-analyzer-disable-checker', mainargs.disable_checkers ]
analyzer_params += [ '-analyzer-config', 'xtu-dir=' + os.path.abspath(mainargs.xtuindir) ]
analyzer_params += [ '-analyzer-stats' ]

analyzer_env = {}
analyzer_env['CLANG'] = os.path.join(clang_path, 'clang')
analyzer_env['OUT_DIR'] = os.path.abspath(mainargs.xtuindir)
analyzer_env['CCC_ANALYZER_HTML'] = os.path.abspath(mainargs.xtuoutdir)
analyzer_env['CCC_ANALYZER_ANALYSIS'] = ' '.join(analyzer_params)
analyzer_env['CCC_ANALYZER_OUTPUT_FORMAT'] = 'plist-multi-file'
#analyzer_env['CCC_ANALYZER_TIMEOUT'] = str(60 * int(args.timeout))
analyzer_env['IS_INTERCEPTED'] = 'true'

buildlog_file = open(mainargs.buildlog, 'r')
buildlog = json.load(buildlog_file)
buildlog_file.close()

buildgraph_file = open(mainargs.buildgraph, 'r')
buildgraph = json.load(buildgraph_file)
buildgraph_file.close()

src_pattern = re.compile('.*\.(cc|c|cxx|cpp)$', re.IGNORECASE)
cmd_2_order = {}
build_steps = 0
for step in buildlog :
    if src_pattern.match(step['file']) :
        cmd_2_order[step['command']] = build_steps
    build_steps += 1

def get_compiler_and_arguments(cmd) :
    had_command = False
    args = []
    for arg in cmd.split() :
        if had_command :
            args.append(arg)
        if not had_command and arg.find('=') == -1 :
            had_command = True
            compiler = arg
    return compiler, args

def analyze(directory, command) :
    old_environ = os.environ
    old_workdir = os.getcwd()
    compiler, args = get_compiler_and_arguments(command)
    os.environ.update(analyzer_env)
    os.environ['COMPILER'] = compiler
    os.chdir(directory)
    ccc_cmd = os.path.join(ccc_path, 'ccc-analyzer') + ' ' + string.join(args, ' ')
    if mainargs.verbose :
        print ccc_cmd
    subprocess.call(ccc_cmd, shell=True)
    os.chdir(old_workdir)
    os.environ.update(old_environ)

try:
    os.makedirs(os.path.abspath(mainargs.xtuoutdir))
except OSError:
    print 'Output directory %s already exists!' % os.path.abspath(mainargs.xtuoutdir)
    sys.exit(1)

ccc_workers = multiprocessing.Pool(processes=mainargs.threads)
for step in buildlog :
    if step['command'] in cmd_2_order :
        ccc_workers.apply_async(analyze, [step['directory'], step['command']])
ccc_workers.close()
ccc_workers.join()

#TODO correct execution order

os.system('rm -vf ' + os.path.join(os.path.abspath(mainargs.xtuindir), 'visitedFunc.txt'))
try:
    os.removedirs(os.path.abspath(mainargs.xtuoutdir))
    print 'Removing directory %s because it contains no reports' % os.path.abspath(mainargs.xtuoutdir)
except OSError:
    pass


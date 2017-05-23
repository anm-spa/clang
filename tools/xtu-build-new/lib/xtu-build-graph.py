#!/usr/bin/python
# -*- coding: utf-8 -*-

import time
import copy
import re
import argparse
import itertools
from collections import namedtuple
from collections import defaultdict
import json

parser = argparse.ArgumentParser(description='generate build dependency graph')
parser.add_argument('-b', required=True, dest='commands_file',
                    help=("absolute path to compile_commands.json "
                          "(including file name)"))
parser.add_argument('-c', dest='cfg_file', help='path to cfg.txt')
parser.add_argument('-d', dest='defined_fns_file',
                    help='path to defined function file')
parser.add_argument('-e', dest='extern_fns_file', help='path to external funs')
parser.add_argument('-o', dest='out_file', help='output file')

args = parser.parse_args()


InOut = namedtuple("InOut", "into out")
Edge = namedtuple("Edge", "fr to val fn")


removable_edges = dict()
visited = set()
oldvisited = set()
onstack = set()


def cyclic(graph):
    visited = set()
    path = [object()]
    path_set = set(path)
    stack = [iter(graph.keys())]
    while stack:
        for v in stack[-1]:
            if v in path_set:
                return True
            elif v not in visited:
                visited.add(v)
                path.append(v)
                path_set.add(v)
                stack.append(iter(graph.get(v, ()).out))
                break
        else:
            path_set.remove(path.pop())
            stack.pop()
    return False


def edgesort(e1, e2):
    return e1.val > e2.val


def getEdgeVal(edge):
    return edge.val


def remove_circles_dfs(graph):
    global visited, oldvisited
    path = [object()]
    path_set = set(path)
    stack = [iter(graph.keys())]
    while stack:
        for v in stack[-1]:
            if v in path_set:
                removable_edges[path[-1]].out.add(v)
                removable_edges[v].into.add(path[-1])
            elif v not in visited:
                visited.add(v)
                path.append(v)
                path_set.add(v)
                stack.append(iter(graph.get(v, ()).out - oldvisited))
                break
        else:
            path_set.remove(path.pop())
            stack.pop()
    oldvisited |= visited
    visited = set()


def dfs(graph, start):
    global visited, oldvisited, onstack
    if start in oldvisited:
        return

    stack = [start]
    path = [object()]
    path_set = set(path)
    while stack:
        vertex = stack.pop()
        if vertex not in visited:
            visited.add(vertex)
            path.append(vertex)
            path_set.add(vertex)

        for node in (graph[vertex].out & path_set):
            removable_edges[vertex].out.add(node)
            removable_edges[node].into.add(vertex)
        stack.extend(graph[vertex].out - visited - oldvisited)
    oldvisited |= visited
    visited = set()


def topological_order(graph):  # works only on DAG
    topological_order = []
    top_seed = [node for node in graph if len(graph[node].into) == 0]
    while top_seed:
        node = top_seed.pop()
        topological_order.append(node)
        for m in graph[node].out:
            graph[m].into.remove(node)
            if len(graph[m].into) == 0:
                top_seed.append(m)
        graph.pop(node)
    return topological_order


def main():
    # -------------- obtain function-to-file mapping --------------#
    print('Obtaining function-to-file mapping')
    t = time.clock()
    # sys.stdout.flush()

    tmpdir = ".xtu/"

    ast_regexp = re.compile("^/ast/(?:\w)+")

    fns = dict()
    external_map = dict()

    if args.defined_fns_file:
        defined_fns_filename = args.defined_fns_file
    else:
        defined_fns_filename = tmpdir + "definedFns.txt"

    with open(defined_fns_filename, "r") as defined_fns_file:
        for line in defined_fns_file:
            funcname, filename, funsize = line.strip().split(' ')
            if funcname.startswith('!'):
                funcname = funcname[1:]
            fns[funcname] = filename
            fullname = filename + "::" + funcname
            fullname = re.sub(ast_regexp, "", fullname).split('@')[0]

    if args.extern_fns_file:
        extern_fns_filename = args.extern_fns_file
    else:
        extern_fns_filename = tmpdir + "externalFns.txt"

    with open(extern_fns_filename, "r") as extern_fns_file:
        for line in extern_fns_file:
            line = line.strip()
            if line in fns and line not in external_map:
                external_map[line] = fns[line]

    # -------------- analyze call graph to find analysis order --------------#

    cfg = dict()
    func_set = set()
    print(time.clock() - t)
    print('Obtaining analysis order')
    t = time.clock()
    # sys.stdout.flush()

    callees_glob = set()

    # Read call graph
    if args.cfg_file:
        cfg_filename = args.cfg_file
    else:
        cfg_filename = tmpdir + "cfg.txt"

    with open(cfg_filename, "r") as cfg_file:
        for line in cfg_file:
            funcs = line.strip().split(' ')
            key = funcs[0]
            arch = key.split("@")[-1]
            key = re.sub("@" + arch, "", key)
            func_set.add(key)
            filename, func = key.split("::")
            filename = filename.split("@")[0]
            callees = set()
            for callee in funcs[1:]:
                if callee.startswith("::"):
                    fname = filename + callee.split("@")[0]
                    callees.add(fname)
                    func_set.add(fname)
                elif callee in external_map:
                    arch = callee.split("@")[-1]
                    fname = re.sub(ast_regexp, "", external_map[callee]) + \
                        "::" + callee.split("@")[0]
                    callees.add(fname)
                    func_set.add(fname)
            if callees:
                cfg[key] = callees
                callees_glob |= callees

    # Read compile_commands.json

    src_pattern = re.compile(".*\.(C|c|cc|cpp|cxx|ii|m|mm)$")
    with open(args.commands_file, "r") as build_args_file:
        build_json = json.load(build_args_file)

    commandlist = [command for command in build_json
                   if src_pattern.match(command['file'])]

    compile_commands_id = {commandlist[i]['command']: i
                           for i in range(0, len(commandlist))}
    command_id_to_compile_command_id = []
    command_id_to_file = {}

    sorted_commands = sorted(commandlist)
    file_to_command_ids = defaultdict(set)
    command_id = 0
    for buildcommand in sorted_commands:
        command_id_to_compile_command_id.append(
                compile_commands_id[buildcommand['command']])
        file_to_command_ids[buildcommand['file']].add(command_id)
        command_id_to_file[command_id] = buildcommand['file']
        command_id += 1

    print(time.clock() - t)
    print("build build_graph")
    t = time.clock()
    # Create build_commands dependency graph based on function calls
    # (and containing files)

    build_graph = defaultdict(InOut)
    for fid in range(0, command_id):
        build_graph[fid] = InOut(set(), set())

    for caller, callees in list(cfg.items()):
        callerfile = caller.split('::')[0]
        for callerbuild_id in file_to_command_ids[callerfile]:
            for callee in callees:
                calleefile = callee.split('::')[0]
                for calleebuild_id in file_to_command_ids[calleefile]:
                    if calleebuild_id != callerbuild_id:
                        build_graph[callerbuild_id].out.add(calleebuild_id)
                        build_graph[calleebuild_id].into.add(callerbuild_id)
    # eliminate `circles from build_graph
    build_graph_copy = copy.deepcopy(build_graph)
    print(time.clock() - t)
    print("eliminate circles")
    t = time.clock()

    for node in build_graph_copy.keys():
        removable_edges[node] = InOut(set(), set())

    for node in build_graph_copy.keys():
        remove_circles_dfs(build_graph_copy)

    build_graph = {
                    key: InOut(build_graph[key].into -
                               removable_edges[key].into,
                               build_graph[key].out - removable_edges[key].out)
                    for key in list(build_graph.keys())
    }
    print(time.clock() - t)

    # Some checks to make sure the algorithm is valid
    assert not cyclic(build_graph)

    build_graph_copy = copy.deepcopy(build_graph)

    if args.out_file:
        out_file = args.out_file
    else:
        out_file = tmpdir + "build_dependency.json"

    print("write build_dependency graph to " + out_file)
    with open(out_file, "w") as dependency_file:
        list_graph = []
        for n in build_graph:
            for m in build_graph[n].out:
                    list_graph.append((command_id_to_compile_command_id[n],
                                       command_id_to_compile_command_id[m]))
        dependency_file.write(json.dumps(list_graph))

    # topological order of build_graph
#   file_order = topological_order(build_graph)
#   print("write topological order of build commands to "+ tmpdir +"order.txt")
#   with open(tmpdir + "order.txt", "w") as order_file:
#        for file_id in file_order:
#            order_file.write(sorted_commands[file_id]['command'])
#            order_file.write("\n")


if __name__ == "__main__":
    main()

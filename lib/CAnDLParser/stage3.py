#!/usr/bin/pypy

whitelist = ["Distributive", "HoistSelect", "SCoP", "Experiment"]

def partial_evaluator(syntax, handler, *extras):
    handler_result = handler(syntax, *extras)
    if handler_result:
        return handler_result
    return tuple(partial_evaluator(s, handler, *extras) if type(s) is tuple else s for s in syntax)

def rebase(prefix, suffix):
    if not prefix:
        return suffix
    elif suffix[0] == "slotrange":
        return (suffix[0], rebase(prefix, suffix[1]), suffix[2], suffix[3])
    elif suffix[0] in ["slotmember", "slotindex"]:
        return (suffix[0], rebase(prefix, suffix[1]), suffix[2])
    elif suffix[0] == "slotbase":
        return ("slotmember", prefix, suffix[1])
    raise Exception("Error, \"" + suffix[0] + "\" is not allowed in suffix.")

def evaluate_remove_rename_rebase(syntax, renames={}, prefix=None):
    if syntax[0] == "rename":
        child      = syntax[1]
        newprefix  = None       if len(syntax)%2==0 else syntax[-1]
        renamevars = syntax[2:] if len(syntax)%2==0 else syntax[2:-1]
        newrenames = dict(zip(renamevars[1::2], renamevars[0::2]))

        return partial_evaluator(partial_evaluator(child, evaluate_remove_rename_rebase, newrenames, newprefix),
                                                          evaluate_remove_rename_rebase, renames, prefix)

    elif syntax[0] in ["slotmember", "slotindex"]:
        if syntax in renames:
            return renames[syntax]
        else:
            return (syntax[0], partial_evaluator(syntax[1], evaluate_remove_rename_rebase, renames, prefix), syntax[2])

    elif syntax[0] == "slotbase":
        if syntax in renames:
            return renames[syntax]
        else:
            return rebase(prefix, syntax)

def evaluate_remove_for_with(syntax, specs, vardict={}, collectvars=[]):
    if syntax[0] == "inheritance":
        new_vardict = {}
        for i in range(2,len(syntax),2):
            rangevalue = partial_evaluator(syntax[i+1], evaluate_remove_for_with, specs, vardict, collectvars)

            if rangevalue[0] == "baseconst":
                new_vardict[syntax[i]] = rangevalue[1]
            else:
                raise Exception("Free variables remain in for assignment.")

        if not new_vardict:
            new_vardict = vardict

        return partial_evaluator(specs[syntax[1]], evaluate_remove_for_with, specs, new_vardict, collectvars)

    elif syntax[0] in ["forall", "forsome"]:
        rangestart = partial_evaluator(syntax[3], evaluate_remove_for_with, specs, vardict, collectvars)
        rangestop  = partial_evaluator(syntax[4], evaluate_remove_for_with, specs, vardict, collectvars)

        if rangestart[0] == rangestop[0] == "baseconst":

            branches = []

            for i in range(rangestart[1],rangestop[1]):

                new_vardict = dict(vardict)
                new_vardict[syntax[2]] = i
                branches.append(partial_evaluator(syntax[1], evaluate_remove_for_with, specs, new_vardict, collectvars))

            return ("conjunction" if syntax[0] == "forall" else "disjunction",)+tuple(branches)
        raise Exception("Free variables remain in for loop range start.")

    elif syntax[0] == "forone":
        rangevalue = partial_evaluator(syntax[3], evaluate_remove_for_with, specs, vardict, collectvars)

        if rangevalue[0] == "baseconst":

            new_vardict = dict(vardict)
            new_vardict[syntax[2]] = rangevalue[1]
            return partial_evaluator(syntax[1], evaluate_remove_for_with, specs, new_vardict, collectvars)
        raise Exception("Free variables remain in for assignment.")

    elif syntax[0] == "if":
        leftvalue  = partial_evaluator(syntax[1], evaluate_remove_for_with, specs, vardict, collectvars)
        rightvalue = partial_evaluator(syntax[2], evaluate_remove_for_with, specs, vardict, collectvars)

        if leftvalue[0] == "baseconst" and rightvalue[0] == "baseconst":
            if leftvalue[1] == rightvalue[1]:
                return partial_evaluator(syntax[3], evaluate_remove_for_with, specs, vardict, collectvars)
            else:
                return partial_evaluator(syntax[4], evaluate_remove_for_with, specs,vardict, collectvars)
        raise Exception("Free variables remain in conditional.")

    elif syntax[0] == "default":
        defaultvalue = partial_evaluator(syntax[3], evaluate_remove_for_with, specs, vardict, collectvars)

        if syntax[2] in vardict or syntax[2] in collectvars:
            return partial_evaluator(syntax[1], evaluate_remove_for_with, specs, vardict, collectvars)
        elif defaultvalue[0] == "baseconst":
            new_vardict = dict(vardict)
            new_vardict[syntax[2]] = defaultvalue[1]
            return partial_evaluator(syntax[1], evaluate_remove_for_with, specs, new_vardict, collectvars)
        raise Exception("Free variables remain in default value.")

    elif syntax[0] == "collect":
        new_collectvars = collectvars[:] + [syntax[1]]
        return syntax[:3] + (partial_evaluator(syntax[3], evaluate_remove_for_with, specs, vardict, new_collectvars),)

    elif syntax[0] == "basevar":
        if syntax[1] in vardict:
            return ("baseconst", vardict[syntax[1]])
        elif syntax[1] in collectvars:
            return syntax
        raise Exception("Free variable \""+syntax[1]+"\" remain in static calculation.")

    elif syntax[0] in ["addvar", "subvar"]:
        if syntax[2] in vardict:
            return evaluate_remove_for_with((syntax[0][:3]+"const", syntax[1], vardict[syntax[2]]), specs, vardict)
        raise Exception("Free variables remain in static calculation of "+syntax[0]+".")

    elif syntax[0] in ["addconst", "subconst"]:
        leftvalue = evaluate_remove_for_with(syntax[1], specs, vardict)

        if leftvalue[0] == "baseconst":
            return (leftvalue[0], leftvalue[1] + syntax[2] * {"add":+1,"sub":-1}[syntax[0][:-5]])
        else:
            raise Exception("Free variables remain in static calculation.")

def evaluate_remove_trivials(syntax):
    if syntax[0] in ["conjunction", "disjunction"]:
        is_trivial = False
        result     = syntax[:1]
        for s in [partial_evaluator(s, evaluate_remove_trivials) for s in syntax[1:]]:
            if s[0] == {"con":"false","dis":"true"}[syntax[0][:3]]:
                is_trivial = True
            elif s[0] != {"con":"true","dis":"false"}[syntax[0][:3]]:
                result = result + (s,)
        if is_trivial:
            return ({"con":"false","dis":"true"}[syntax[0][:3]],)
        elif len(result) == 1:
            return ({"con":"true","dis":"false"}[syntax[0][:3]],)
        else:
            return result if len(result) > 2 else result[1]

def evaluate_flatten_connectives(syntax):
    if syntax[0] in ["conjunction", "disjunction"]:
        result = syntax[:1]

        for child in (partial_evaluator(s, evaluate_flatten_connectives) for s in syntax[1:]):
            if child[0] == syntax[0]:
                result = result + child[1:]
            else:
                result = result + (child,)
        return result

def evaluate_bisect_connectives(syntax):
    if syntax[0] in ["conjunction", "disjunction"]:
        if len(syntax[1:]) == 0:
            return ({"con":"false","dis":"true"}[syntax[0][:3]],)
        elif len(syntax[1:]) == 1:
            return partial_evaluator(syntax[1], evaluate_bisect_connectives)
        elif len(syntax[1:]) == 2:
            return syntax[:1]+(partial_evaluator(syntax[1], evaluate_bisect_connectives),
                               partial_evaluator(syntax[2], evaluate_bisect_connectives))
        else:
            return syntax[:1]+(partial_evaluator(syntax[1], evaluate_bisect_connectives),
                               partial_evaluator(syntax[:1]+syntax[2:], evaluate_bisect_connectives))

def replace_variables(syntax, replaces):
    if syntax[0] in ["slotbase", "slotmember", "slotindex"]:
        return replaces[syntax] if syntax in replaces else syntax

def optimize_delay_aliases(syntax, slotlist):
    if syntax[0] == "conjunction":
        replaces = {}
        for a,b in (s[1][1:] for s in syntax[1:] if s[0] == "atom" and s[1][0] == "ConstraintSame"):
            aflat, bflat = generate_cpp_slot(a), generate_cpp_slot(b)
            if aflat in slotlist and bflat in slotlist:
                if slotlist.index(aflat) < slotlist.index(bflat):
                    replaces[b] = a
                if slotlist.index(bflat) < slotlist.index(aflat):
                    replaces[a] = b

        return syntax[:1] + tuple(s if s[0] == "atom" and s[1][0] == "ConstraintSame" else
                                  partial_evaluator(
                                  partial_evaluator(s, replace_variables,      replaces),
                                                       optimize_delay_aliases, slotlist) for s in syntax[1:])

def indent_code(prefix, code):
    current_indent = 0
    while code[:1] == " " and current_indent < len(prefix):
        code = code[1:]
        current_indent += 1

    return prefix + code.replace("\n", "\n"+" "*(len(prefix) - current_indent))

def generate_cpp_type(syntax):
    if syntax[0] in ["conjunction", "disjunction"]:
        return {"conjunction":"ConstraintAnd", "disjunction":"ConstraintOr"}[syntax[0]]
    elif syntax[0] == "atom":
        return syntax[1][0]
    raise Exception("Error, \"" + syntax[0] + "\" is not allowed in type generator.")

def generate_cpp_slot(syntax):
    if syntax[0] == "slotbase":
        return syntax[1]
    elif syntax[0] == "slotmember":
        return generate_cpp_slot(syntax[1]) + "." + syntax[2]
    elif syntax[0] == "slotindex" and syntax[2][0] in ["baseconst", "basevar"]:
        return generate_cpp_slot(syntax[1]) + "[" + str(syntax[2][1]) + "]"
    raise Exception("Error, \"" + syntax[0] + "\" is not allowed in single slot.")

def generate_cpp_slotlist(syntax):
    if syntax[0] == "slotbase":
        return (syntax[1],)
    elif syntax[0] == "slotmember":
        return tuple(prefix+"."+syntax[2] for prefix in generate_cpp_slotlist(syntax[1]))
    elif syntax[0] == "slotindex" and syntax[2][0] in ["baseconst", "basevar"]:
        return tuple(prefix+"["+str(syntax[2][1])+"]" for prefix in generate_cpp_slotlist(syntax[1]))
    elif syntax[0] == "slotrange" and syntax[2][0] == "baseconst" and syntax[3][0] == "baseconst":
        return tuple(prefix+"["+str(i)+"]" for prefix in generate_cpp_slotlist(syntax[1]) for i in range(syntax[2][1], syntax[3][1]))
    elif syntax[0] == "slottuple":
        return sum((generate_cpp_slotlist(s) for s in syntax[1:]), ())
    raise Exception("Error, \"" + syntax[0] + "\" is not allowed in slot list.")

def getatom(counter, typename):
    if typename in counter:
        result = "atom{}_[{}]".format(counter[typename][0], counter[typename][1])
        counter[typename] = (counter[typename][0], counter[typename][1]+1)
    else:
        result = "atom{}_[{}]".format(len(counter), 0)
        counter[typename] = (len(counter), 1)
    return result;

def prune_types(slots,result,code,counter):
    for slot in slots:
        if result[slot][0].startswith("remove_reference"):
            atom  = getatom(counter, "IndirSolverAtom")
            code += "{} = {{{}}};\n".format(atom, result[slot][1])
            result[slot] = ("IndirSolverAtom", atom)
    return slots,result,code

def unique_list(inlist):
    tempset  = set()
    templist = []
    for elem in inlist:
        if elem not in tempset:
            tempset.add(elem)
            templist.append(elem)
    return templist

def code_generation_core(syntax, counter):
    if syntax[0] == "atom":
        classname = "Backend{}".format(syntax[1][0][10:])
        if syntax[1][0][10:13] in ["DFG","CFG","PDG"] and syntax[1][0][13:20] in ["Dominat","Postdom"]:
            atom = getatom(counter, "my_shared_ptr<{}>".format(classname))
            code = "{} = {{{{0,1,1}}, wrap}};\n".format(atom)
        elif syntax[1][0][10:13] in ["DFG","CFG","PDG"] and syntax[1][0][13:20] == "Blocked":
            atom = getatom(counter, "my_shared_ptr<{}>".format(classname))
            code = "{} = {{{{1,1,1}}, wrap}};\n".format(atom)
        elif len(syntax[1]) > 2:
            atom = getatom(counter, "my_shared_ptr<{}>".format(classname))
            code = "{} = {{wrap}};\n".format(atom)
        else:
            if classname not in counter:
                atom = getatom(counter, classname)
                code = "{} = {{wrap}};\n".format(atom)
            else:
                atom = "atom{}_[0]".format(counter[classname][0])
                code = ""

        slots = [generate_cpp_slot(s) for s in syntax[1][1:2]+syntax[1][3:1:-1]]

        if syntax[1][0][10:13] in ["DFG","CFG","PDG"] and syntax[1][0][13:20] in ["Dominat","Postdom"]:
            result = {slot:("MultiVectorSelector<Backend{},{}>".format(syntax[1][0][10:], i+1), "{}, <[0]>".format(atom)) for i,slot in enumerate(slots)}
        elif syntax[1][0][10:13] in ["DFG","CFG","PDG"] and syntax[1][0][13:20] == "Blocked":
            result = {slot:("MultiVectorSelector<Backend{},{}>".format(syntax[1][0][10:], i), "{}, <[0]>".format(atom)) for i,slot in enumerate(slots)}
        elif len(syntax[1]) == 2:
            result = {slot:("Backend{}".format(syntax[1][0][10:]), atom) for i,slot in enumerate(slots)}
        else:
            result = {slot:("ScalarSelector<Backend{},{}>".format(syntax[1][0][10:], i), atom) for i,slot in enumerate(slots)}

        return slots,result,code

    elif syntax[0] == "ConstraintOpcode":
        opcode = syntax[2][:1].upper()+syntax[2][1:]
        if opcode[:3] == "Gep": opcode = "GEP"+opcode[3:]
        if opcode[-2:] == "or": opcode = opcode[:-2]+"Or"
        if opcode[-3:] == "and": opcode = opcode[:-3]+"And"
        if opcode[-3:] == "add": opcode = opcode[:-3]+"Add"
        if opcode[-3:] == "sub": opcode = opcode[:-3]+"Sub"
        if opcode[-3:] == "mul": opcode = opcode[:-3]+"Mul"
        if opcode[-3:] == "div": opcode = opcode[:-3]+"Div"
        if opcode[-3:] == "cmp": opcode = opcode[:-3]+"Cmp"
        if opcode[-3:] == "ext": opcode = opcode[:-3]+"Ext"
        if opcode[-4:] == "cast": opcode = opcode[:-4]+"Cast"
        if opcode[-5:] == "shift": opcode = opcode[:-5]+"Shift"
        if opcode[-6:] == "vector": opcode = opcode[:-6]+"Vector"
        if opcode[-7:] == "element": opcode = opcode[:-7]+"Element"

        classname = "Backend{}Inst".format(opcode)

        if classname not in counter:
            atom = getatom(counter, classname)
            code = "{} = {{wrap}};\n".format(atom)
        else:
            atom = "atom{}_[0]".format(counter[classname][0])
            code = ""

        slots = [generate_cpp_slot(s) for s in syntax[1:2]]

        result = {slot:(classname, atom) for i,slot in enumerate(slots)}

        return slots,result,code

    elif syntax[0] == "GeneralizedDominance":
        atom        = getatom(counter, "my_shared_ptr<BackendPDGDominate>")
        slotlists   = [generate_cpp_slotlist(s) for s in syntax[1:2]+syntax[3:1:-1]]
        code        = "{} = {{{{{},{},{}}}, wrap}};\n".format(atom, len(slotlists[0]), len(slotlists[1]), len(slotlists[2]))
        slots       = unique_list([slot for slotlist in slotlists for slot in slotlist])
        result      = {slot:("MultiVectorSelector<BackendPDGDominate,{}>".format(j), "{}, <[{}]>".format(atom, i))
                       for j,slotlist in enumerate(slotlists) for i,slot in enumerate(slotlist)}
        return slots,result,code

    elif syntax[0] == "GeneralizedSame":
        atom        = getatom(counter, "my_shared_ptr<BackendSameSets>")
        slotlists   = [generate_cpp_slotlist(s) for s in syntax[1:2]+syntax[3:1:-1]]
        code        = "{} = {{{{{}}}}};\n".format(atom, len(slotlists[0]))
        slots       = unique_list([slot for slotlist in slotlists for slot in slotlist])
        result      = {slot:("MultiVectorSelector<BackendSameSets,{}>".format(j), "{}, <[{}]>".format(atom, i))
                       for j,slotlist in enumerate(slotlists) for i,slot in enumerate(slotlist)}
        return slots,result,code

    elif syntax[0] in ["conjunction", "disjunction"]:
        part_results = [code_generation_core(s, counter) for s in syntax[1:]]
        code         = "".join([part[2] for part in part_results])
        slots        = unique_list([slot for part in part_results for slot in part[0]])
        result       = {slot:[part[1][slot] for part in part_results if slot in part[1]] for slot in slots}

        if syntax[0] == "conjunction":
            for slot in slots:
                newresult = result[slot][0]

                for a,b in result[slot][1:]:
                    classname    = indent_code("BackendAnd<", newresult[0]+",\n"+a+">")
                    atom         = getatom(counter, classname)
                    code        += "{} = {{{}}};\n".format(atom, "{"+newresult[1]+"}, {"+b+"}")
                    classname    = "remove_reference<decltype({}[0])>::type".format(atom[:atom.index("[")])
                    newresult    = (classname, atom)

                result[slot] = newresult

        if syntax[0] == "disjunction":
            choices       = max([0]+[len(result[slot]) for slot in slots])
            sliced_result = [{slot:result[slot][i] for slot in slots} for i in range(choices)]

            newresult = sliced_result[0]

            for slic in sliced_result[1:]:
                shared_data  = getatom(counter, "my_shared_ptr<pair<unsigned,unsigned>>")
                code        += "{} = {{0,0}};\n".format(shared_data);

                for slot in slots:
                    classname  = indent_code("BackendOr<", newresult[slot][0]+",\n"+slic[slot][0]+">")
                    atom       = getatom(counter, classname)
                    code      += "{} = {{{}}};\n".format(atom, "{"+newresult[slot][1]+"}, {"+slic[slot][1]+"}, {"+shared_data+"}")
                    classname  = "remove_reference<decltype({}[0])>::type".format(atom[:atom.index("[")])
                    newresult[slot] = (classname, atom)

            result = newresult

        return prune_types(slots,result,code,counter)

    elif syntax[0] == "collect":
        atom = getatom(counter, "my_shared_ptr<BackendCollect>")

        part_slots, part_result, code = code_generation_core(syntax[3], counter)

        local_slots_idx  = [i for i,slot in enumerate(part_slots) if "["+syntax[1]+"]"     in slot]
        global_slots_idx = [i for i,slot in enumerate(part_slots) if "["+syntax[1]+"]" not in slot]

        local_parts  = [part_result[part_slots[idx]] for idx in local_slots_idx]
        global_parts = [part_result[part_slots[idx]] for idx in global_slots_idx]

        mergedatom = atom[4:atom.index("[")] + atom[atom.index("[")+1:atom.index("]")] + "_"

        code += "\n".join(["vector<unique_ptr<SolverAtom>> globals{};".format(mergedatom)]
                         +["globals{}.emplace_back(unique_ptr<SolverAtom>(new {}({})));".format(mergedatom, part[0], part[1]) for part in global_parts]
                         +["vector<unique_ptr<SolverAtom>> locals{};".format(mergedatom)]
                         +["locals{}.emplace_back(unique_ptr<SolverAtom>(new {}({})));".format(mergedatom, part[0], part[1]) for part in local_parts]
                         +["{} = {{{{{{{},{}}}}}, move(globals{}), move(locals{})}};".format(atom, len(global_parts), len(local_parts), mergedatom, mergedatom)])+"\n"

        local_slots  = [part_slots[idx].replace("["+syntax[1]+"]", "["+str(n)+"]") for n in range(syntax[2]) for idx in local_slots_idx]
        global_slots = [part_slots[idx] for idx in global_slots_idx]

        result      = {slot:("MultiVectorSelector<BackendCollect,0>", "{}, <[{}]>".format(atom, i)) for i,slot in enumerate(global_slots)}
        result.update({slot:("MultiVectorSelector<BackendCollect,1>", "{}, <[{}]>".format(atom, i)) for i,slot in enumerate(local_slots)})

        return prune_types(global_slots+local_slots,result,code,counter)
        return global_slots+local_slots, result, code

    raise Exception("Error, \"" + syntax[0] + "\" is not allowed in atoms collection.")

def postprocess_copyconstructions(code):
    atomic_def_lines = [line for line in code.split("\n") if line.startswith("atom")]
    code_atomic_defs = [(line,line.split("[")[0]+line.split(" = ")[1]) for line in atomic_def_lines]
    duplication_dict = {}

    dictionary = {}
    for n,line in enumerate(code_atomic_defs):
        if line[1] in dictionary:
            duplication_dict[line[0]] = line[0].split(" = ")[0] + " = " + dictionary[line[1]].split(" = ")[0] + ";"
        else:
            dictionary[line[1]] = line[0]

    return "\n".join([duplication_dict[line] if line in duplication_dict else line for line in code.split("\n")])

def check_differences(base, a1, a2, a3, a4, size):
    diff1 = (y-x for i in range(size) for x,y in zip(base[a1+i],base[a2+i]))
    diff2 = (y-x for i in range(size) for x,y in zip(base[a3+i],base[a4+i]))

    return all([x == y for x,y in zip(diff1,diff2)])

def compare_sliced(inlist, a1, a2, size):
    for i in range(size):
        if inlist[a1+i] != inlist[a2+i]:
            return False
    return True

def calculate_loop_block(stripindizes, onlyindizes):
    repeatstart  = 0
    repeatsize   = 1
    repeatamount = 1
    codesaved    = 0

    for m in range(1,512):
        for n in range(len(stripindizes)-2*m+1):
            if compare_sliced(stripindizes, n, n+m, m):
                newrepeatamount = 2
                for k in range(2,128):
                    if n+(k+1)*m > len(stripindizes):
                        break;
                    if (compare_sliced(stripindizes, n+(k-1)*m, n+k*m, m) and
                        check_differences(onlyindizes, n+(k-2)*m, n+(k-1)*m, n+(k-1)*m, n+(k+0)*m, m)):
                        newrepeatamount += 1
                    else:
                        break
                if (newrepeatamount-1)*m > codesaved:
                    repeatstart  = n
                    repeatsize   = m
                    repeatamount = newrepeatamount
                    codesaved    = (newrepeatamount-1)*m

    return repeatstart, repeatsize, repeatamount, codesaved

# This does not actually check that indizes are a linear progression for more than two repetitions!
def loop_block(block, depth=0):
    if depth > 4: return block
    stripindizes = [hash(tuple(line[::2])) for line in block]
    onlyindizes  = [[int(n) for n in line[1::2]] for line in block]

    repeatstart, repeatsize, repeatamount, codesaved = calculate_loop_block(stripindizes, onlyindizes)

    if codesaved:
        block_before = block[:repeatstart]
        block_after  = block[repeatstart+repeatamount*repeatsize:]

        modified_block = block[repeatstart:repeatstart+repeatsize]

        for k,line in enumerate(modified_block):
            for i in range(2,len(line),2):
                difference = int(block[repeatstart+repeatsize+k][i-1])-int(block[repeatstart+k][i-1])
                if difference > 1:
                    line[i] = "+{}*{}{}".format("ijklm"[depth], difference, line[i])
                elif difference < -1:
                    line[i] = "-{}*{}{}".format("ijklm"[depth], -difference, line[i])
                elif difference == 1:
                    line[i] = "+{}{}".format("ijklm"[depth], line[i])
                elif difference == -1:
                    line[i] = "-{}{}".format("ijklm"[depth], line[i])

        return (loop_block(block_before, depth)
               +[["for(unsigned {0} = 0; {0} < {1}; {0}++) {{".format("ijklm"[depth], repeatamount)]]
               +[["    "+line[0]]+line[1:] for line in loop_block(modified_block, depth+1)]
               +[["}"]]
               +loop_block(block_after, depth))

    return block

def postprocess_add_loops_one_block(block):
    block = [[part for word in line.split(']') for part in word.split('[')] for line in block]
    block = [line if len(line) <= 1 else [line[0]+"["]+line[1:-1]+["]"+line[-1]] for line in block]
    for line in block:
        if len(line) > 1:
            line[2:-2:2] = ["]"+e+"[" for e in line[2:-2:2]]
    block = loop_block(block)
    return ["".join(line) for line in block]

def postprocess_add_loops(code):
    grouped = itertools.groupby(code.split('\n'), (lambda x: x.startswith("atom")))
    grouped = [postprocess_add_loops_one_block(list(b)) if a else list(b) for a,b in grouped]
    return "\n".join(line for group in grouped for line in group)

def generate_fast_cpp_specification(syntax, specs):
    constr = partial_evaluator(syntax[2], evaluate_remove_for_with, specs)
    constr = partial_evaluator(constr,    evaluate_remove_rename_rebase)
    constr = partial_evaluator(constr,    evaluate_remove_trivials)
    constr = partial_evaluator(constr,    evaluate_flatten_connectives)
    constr = partial_evaluator(constr,    evaluate_bisect_connectives)
    constr = partial_evaluator(constr,    evaluate_remove_trivials)

    atom_counter = {}
    slots, result, code = code_generation_core(constr, atom_counter)

    constr = partial_evaluator(constr, optimize_delay_aliases, slots)

    atom_counter = {}
    slots2, result, code = code_generation_core(constr, atom_counter)

    code = postprocess_copyconstructions(code)
    code = postprocess_add_loops(code)

    code = "\n".join(["{} atom{}_[{}];".format(typename, atom_counter[typename][0], atom_counter[typename][1]) for typename in atom_counter]
                    +[code.rstrip()]
                    +["vector<pair<string,unique_ptr<SolverAtom>>> constraint({});".format(len(slots))]
                    +["constraint[{}] = make_pair(\"{}\", unique_ptr<SolverAtom>(new {}({})));".format(n, slot, result[slot][0], result[slot][1]) for n,slot in enumerate(slots)])

    code = code.replace("<[", "").replace("]>", "")

    return "\n".join(["IdiomSpecification Detect{}(llvm::Function& function, unsigned max_solutions)".format(syntax[1])]
                    +["{"]
                    +["    FunctionWrap wrap(function);"]
                    +[indent_code("    ", code)]
                    +["    auto result = Solution::Find(move(constraint), function, max_solutions);"]
                    +["    return result;"]
                    +["}"])

def generate_cpp_code(syntax_list):
    includes  = ["BackendSpecializations", "BackendDirectClasses", "BackendSelectors"]
    specs     = {spec[1] : spec[2] for spec in syntax_list}

    return "\n".join(["#include \"llvm/CAnDLSolver/{}.hpp\"".format(s) for s in includes]
                    +["#include \"llvm/CAnDLParser/Solution.hpp\""]
                    +["using namespace std;"]
                    +["#pragma GCC optimize (\"O0\")"]
                    +["#pragma clang optimize off"]
                    +["using IdiomSpecification = vector<Solution>;"]
                    +["template<typename T>"]
                    +["class my_shared_ptr : public shared_ptr<T>"]
                    +["{"]
                    +["public:"]
                    +["    my_shared_ptr() = default;"]
                    +["    my_shared_ptr<T>& operator=(T t) { shared_ptr<T>::operator=(make_shared<T>(move(t))); return *this; }"]
                    +["    my_shared_ptr<T>& operator=(const my_shared_ptr<T>& t) { return *this = *t; }"]
                    +["};"]
                    +[generate_fast_cpp_specification(syntax, specs) for syntax in syntax_list if syntax[1] in whitelist])

def print_syntax_tree(syntax, indent=0):
    if type(syntax) is str or type(syntax) is int:
        print("| "*indent+str(syntax))
    elif type(syntax) is tuple and type(syntax[0]) is str:
        print("| "*indent+str(syntax[0]))
        for s in syntax[1:]:
            print_syntax_tree(s, indent+1)
    elif type(syntax) is tuple:
        for s in syntax:
            print_syntax_tree(s, indent+1)

import sys
import itertools

sys.setrecursionlimit(10000)

print(generate_cpp_code(eval(sys.stdin.read())))

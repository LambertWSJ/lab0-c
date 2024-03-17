import gdb
import os
import sys
import shutil
from functools import reduce

class DumpDivide(gdb.Command):
    def reset(self):
        self.png_path = None
        self.list_type = "pending"

    def __init__(self) -> None:
        super().__init__("dump_sort", gdb.COMMAND_USER)
        self.reset()

    def parse_args(self, args: list):
        argc = len(args)
        i = 0
        while i < argc:
            arg = args[i]
            if arg == "-type":
                if i + 1 >= argc:
                    print("error: not assign list type")
                    exit(1)
                elif args[i + 1].startswith("-"):
                    print("error: please set list type instead of flag")
                    exit(1)
                self.list_type = args[i + 1]
            if arg == "-o":
                if i + 1 >= argc:
                    print("error: not assign output png file name")
                    exit(1)
                elif args[i + 1].startswith("-"):
                    print("error: please set png output file name instead of flag")
                    exit(1)
                elif args[i + 1].startswith("$"):
                    path = str(gdb.parse_and_eval(args[i + 1])).replace("\"", "")
                    self.png_path : str = path
                    i += 1
                    continue
                self.png_path:str = args[i + 1]
            i += 1

    def dot_gen_origin(self, sort_list: list) -> str:
        dot_cmd = "digraph {\n"
        dot_cmd += "\trankdir=\"LR\"\n"
        dot_cmd += "\thead[shape=none];\n\n"
        nodes = []
        
        for val in sort_list:
            dot_cmd += f"\tnode{val}[label=\"{val}\", shape=\"box\"]\n"
            nodes.append(f"node{val}")

        dot_cmd += reduce(lambda acc, node: acc + node + "->",nodes[:-1], "\thead->")
        dot_cmd += f"{nodes[-1]}[color=blue];\n"
        nodes.reverse()
        dot_cmd += reduce(lambda acc, node: acc + node + "->", nodes, "\t")
        dot_cmd += f"head[color=red];\n"

        dot_cmd += "}"
        return dot_cmd

    def dot_gen_pending(self, sort_lists: list) -> str:
        sz = len(sort_lists)
        i = 0
        anchors=""
        dot_cmd = "digraph {\n"
        dot_cmd += "\trankdir=\"LR\"\n"
        dot_cmd += "\tnull[shape=none];\n"
        # dot_cmd += "\thead[shape=none];\n\n"
        for lst in sort_lists:
            head = True
            for val in lst:
                if head:
                    anchors += f"node{val}->"
                    head = False
                dot_cmd += f"\tnode{val}[label=\"{val}\", shape=\"box\"]\n"

        dot_cmd += "\n"
        # dot_cmd += "\thead->" + anchors + "null[color=blue];\n"
        dot_cmd += "\t" + anchors + "null[color=blue];\n"
        while i < sz:
            lst = sort_lists[i]
            lst_len = len(lst)
            if lst_len > 1:
                dot_cmd += "\t{ rank=\"same\"; "
                j = 0
                while j < lst_len:
                    val = lst[j]
                    dot_cmd += f"node{val} -> " if j + 1 < lst_len else f"node{val}"
                    j += 1
                dot_cmd +="[color=red] }\n"
            i += 1
        dot_cmd += "}"
        return dot_cmd

    def invoke(self, args: str, from_tty: bool) -> None:
        self.reset()
        div_list   = []
        sort_lists = []

        args = args.split(" ")
        self.parse_args(args)

        if self.list_type == "pending":
            gdb.execute(f"p $pend = {args[0]}")
            while True:
                if str(gdb.parse_and_eval("node_val($pend)")) == "0x0":
                    break
                else:
                    gdb.execute("p $list = $pend")
                    while True:
                        if str(gdb.parse_and_eval("node_val($list)")) == "0x0":
                            break
                        else:
                            val = gdb.parse_and_eval("node_val($list)")
                            div_list.append(val['value'].string())
                            gdb.execute("p $list = $list->next")
                gdb.execute("p $pend = $pend->prev")
                sort_lists.append(div_list)
                div_list = []
        elif self.list_type == "origin": # origin kernel linked list
            gdb.execute(f"p $h = {args[0]}, $node = $h->next")
            while True:
                if gdb.parse_and_eval("$node == $h") == 1:
                    print("loop end :)")
                    break
                else:
                    val = gdb.parse_and_eval("node_val($node)")
                    div_list.append(val['value'].string())
                    gdb.execute("p $node = $node->next")
            sort_lists.append(div_list)
            div_list = []

        else:
            print("error: Unknown list type")
            exit(1)


        for i, lst in enumerate(sort_lists):
            print(f"[{len(lst):3}] list{i}: {lst} ")

        if shutil.which("dot") is None:
            print("not found dot\n")
            print("Please install graphviz first\n")
            if sys.platform == "linux":
                print("sudo apt install graphviz")
            elif sys.platform == "darwin":
                print("brew install graphviz")
            return

        if self.png_path is None:
            return

        if os.path.basename(self.png_path).endswith(".png") is False:
            print(
                "warning: image file extension is not png, will auto replace or add '.png' at end of filename"
            )

            e = self.png_path.rfind(".")
            if e != -1:
                self.png_path = self.png_path.replace(self.png_path[e:], ".png")
            else:
                self.png_path += ".png"

        dir_path = os.path.dirname(self.png_path)
        if dir_path not in ['', '.']:
            try:
                os.makedirs(dir_path, exist_ok=True)
            except PermissionError:
                print(f"PermissionError: {self.pan_path}")
                exit(1)




        if self.list_type == 'origin':
            dot_cmd = self.dot_gen_origin(sort_lists[0])
        else:
            dot_cmd = self.dot_gen_pending(sort_lists)

        # store to dot script
        print("store to dot script")
        dot_f:str = os.path.splitext(self.png_path)[0] + ".dot"
        with open(dot_f, "w") as f:
            f.write(dot_cmd)
        print(f"{dot_f} - [{self.png_path}]")
        os.system(f"dot {dot_f} -Tpng:cairo:gd > {self.png_path}")

# initial command
DumpDivide()

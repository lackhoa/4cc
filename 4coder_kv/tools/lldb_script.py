# To run this file, run in lldb:
# command script import ~/AutoDraw/tools/lldb_script.py

# todo I want this: https://stackoverflow.com/questions/28510221/can-i-tell-lldb-to-remove-the-active-breakpoint

# todo https://stackoverflow.com/questions/23812021/how-to-define-a-stop-hook-for-lldb-in-lldbinit

import lldb
import os

def __lldb_init_module(debugger, internal_dict):
    for i in range(debugger.GetNumTargets()):
        target = debugger.GetTargetAtIndex(i)
        target.DeleteAllBreakpoints()
    debugger.HandleCommand('command alias py command script import ~/AutoDraw/tools/lldb_script.py')
    debugger.HandleCommand('command source ~/AutoDraw/tools/lldb_breakpoints.txt')
    # debugger.HandleCommand(f'target stop-hook add --one-liner "target symbols add libgame.dylib.dSYM"')
    debugger.HandleCommand(f'type summary add --python-function {__name__}.print_v3 v3')
    debugger.HandleCommand(f'type summary add --python-function {__name__}.print_v2 v2')
    debugger.HandleCommand(f'env DYLD_INSERT_LIBRARIES=/usr/local/Cellar/llvm/17.0.6/lib/clang/17/lib/darwin/libclang_rt.asan_osx_dynamic.dylib')  # todo hard-coded
    debugger.HandleCommand("settings set stop-line-count-before 10")
    debugger.HandleCommand("settings set stop-line-count-after 10")
    debugger.HandleCommand(f'command script add --overwrite -f {__name__}.disable_current_breakpoint bd')
    debugger.HandleCommand(f'command script add --overwrite -f {__name__}.goto_line goto')
    debugger.HandleCommand('process kill --force')

def print_v3(value, internal_dict, options):
   x = value.GetChildMemberWithName('x').GetValue()
   y = value.GetChildMemberWithName('y').GetValue()
   z = value.GetChildMemberWithName('z').GetValue()
   return f'[{x}, {y}, {z}]'

def print_v2(value, internal_dict, options):
   x = value.GetChildMemberWithName('x').GetValue()
   y = value.GetChildMemberWithName('y').GetValue()
   return f'[{x}, {y}]'

def goto_line(debugger, command, *args):
    """My own version of Ctrl-F10"""
    debugger.HandleCommand(f"tb {command}")  # command is actually the argument... don't understand
    debugger.HandleCommand(f'continue')

def disable_current_breakpoint(debugger, command, *args):
    target = None
    thread = None

    if len(args) == 3:
        # New (2015 & later) lldb invocation style where we're given the execution context
        exe_ctx = args[0]
        result = args[1]
        target = exe_ctx.GetTarget()
        thread = exe_ctx.GetThread()

    if thread == None:
        print(result, "error: process is not paused, or has not been started yet.", file=result)
        result.SetStatus (lldb.eReturnStatusFailed)
        return

    if thread.GetStopReason() != lldb.eStopReasonBreakpoint:
        print(result, "error: not stopped at a breakpoint.", file=result)
        result.SetStatus (lldb.eReturnStatusFailed)
        return

    if thread.GetStopReasonDataCount() != 2:
        print(result, "error: Unexpected number of StopReasonData returned, expected 2, got %d" % thread.GetStopReasonDataCount(), file=result)
        result.SetStatus (lldb.eReturnStatusFailed)
        return

    break_num    = thread.GetStopReasonDataAtIndex(0)
    location_num = thread.GetStopReasonDataAtIndex(1)

    if break_num == 0 or location_num == 0:
        print(result, "error: Got invalid breakpoint number or location number", file=result)
        result.SetStatus (lldb.eReturnStatusFailed)
        return

    bkpt = target.FindBreakpointByID (break_num)
    if location_num > bkpt.GetNumLocations():
        print(result, "error: Invalid location number", file=result)
        result.SetStatus (lldb.eReturnStatusFailed)
        return

    bkpt_loc = bkpt.GetLocationAtIndex(location_num - 1)
    if bkpt_loc.IsValid() != True:
        print(result, "error: Got invalid BreakpointLocation", file=result)
        result.SetStatus (lldb.eReturnStatusFailed)
        return

    bkpt_loc.SetEnabled(False)  # note: don't know how to delete breakpoint yet!
    print("Breakpoint %d.%d disabled." % (break_num, location_num), file=result)
    return

# target = lldb.debugger.GetSelectedTarget()
# target.GetProcess().GetSelectedThread().GetSelectedFrame().GetLineEntry().GetFileSpec()

# Commands you might forget:
# "py" to run my python script
# "thread backtrace" to examine the call stack
# "f" to view current line
# "diplay" to watch variable
# "jump/j" to skip over assertions

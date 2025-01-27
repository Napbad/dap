import os.path
import shlex
import subprocess
from pathlib import Path

from colorama import Fore, Style

from test_res import success
from util import get_dap_files


def test_control(dap_main, source_runtime_dir, d_debug = False):
    arg = " -i"
    """
    Test the control of the code.
    """
    dap_files_to_test = get_dap_files("./")
    for dap_file in dap_files_to_test:
        # Correct the command list by removing the extra spaces around -s
        command_list = [
            Path(os.path.abspath(dap_main)).__str__(),
            "-s",
            source_runtime_dir,
            os.path.abspath(dap_file),
            "-n",
            os.path.abspath(Path(dap_file).parent) + "/build",
            arg.strip()  # Also remove any extra spaces from the arg if present
        ]
        result = subprocess.run(shlex.join(command_list), capture_output=True, text=True, shell=True)

        print(Fore.BLUE + f"Executing: {dap_file} {arg}" + Style.RESET_ALL)
        print(result.args)


        if d_debug:
            print(Fore.RED + result.stderr + Style.RESET_ALL)
            print(Fore.GREEN + result.stdout + Style.RESET_ALL)
        if dap_file.__contains__("for_test.dap"):
            if not (result.stdout.__contains__("")):
                print(Fore.RED, "FAIL: with" , result.args, "\nexpect in stdout:" + "abcdefg" + Style.RESET_ALL)
                print(Fore.YELLOW + "stdout:" + Style.RESET_ALL)
                print(Fore.YELLOW + result.stdout + Style.RESET_ALL)
            else:
                print(Fore.GREEN + "PASS!" + Style.RESET_ALL)
        elif dap_file.__contains__("if_test.dap"):
            print(Fore.GREEN + "PASS" + Style.RESET_ALL)
        success()

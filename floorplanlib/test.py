from collections import namedtuple
import filecmp
import os
import shutil
import subprocess
import sys


FloorplanTestConfig = namedtuple('FloorplanTestConfig', ['name', 'commandline_args'])


def check_result(test_name):
    errors = []

    expected_dir = os.path.join('test/expected', test_name)
    actual_dir = os.path.abspath(os.path.join('test/actual', test_name))

    for filename in sorted(os.listdir(expected_dir)):
        expected_filename = os.path.join(expected_dir, filename)
        actual_filename = os.path.join(actual_dir, filename)

        if not os.path.exists(actual_filename):
            errors.append(f'file not created: {filename}')
        else:
            with open(expected_filename) as f:
                expected_content = f.read()
            with open(actual_filename) as f:
                actual_content = f.read()
            
            expected_content = expected_content.format(actual_dir=actual_dir)

            if actual_content != expected_content:
                description = 'different nb of lines'
                for nb, (l1, l2) in enumerate(zip(actual_content.splitlines(), expected_content.splitlines())):
                    if l1 != l2:
                        description = f'first differing line is {nb+1}:\n   actual:   {l1.strip()}\n   expected: {l2.strip()}'
                        break
                errors.append(f'file content differs: {filename}: {description}')

    return errors


def run(test, expect_fail=False):
    actual = os.path.join('test/actual', test.name)
    expected = os.path.join('test/expected', test.name)
    args = test.commandline_args + ['--out', actual]

    if os.path.exists(actual):
        shutil.rmtree(actual)

    print(f'{test.name:<35s}: ', end='')
    try:
        output = subprocess.check_output(['python3', 'create.py'] + args, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        if expect_fail:
            print(f'ok')
            return True
        else:
            print(f'ERROR (call failed)')
            for line in e.output.decode('utf-8').splitlines():
                print(f'   {line}')
            return False

    if expect_fail:
        print(f'ERROR (call expected to fail but did not fail)')
        return False

    # check output
    errors = check_result(test.name)

    if errors:
        print(f'ERROR')
        for error in errors:
            for line in error.splitlines():
                print(f'   {line}')
        return False
    else:
        print(f'ok')
        return True


def main():
    TESTS = [
                FloorplanTestConfig(
                    name='gainestown_4x4_explicit_size',
                    commandline_args=[
                        '--cores', '4x4', '--corex', '4.31mm', '--corey', '2.08mm',
                        '--subcore-template', 'gainestown_core.flp'
                    ]
                ),
                FloorplanTestConfig(
                    name='gainestown_4x4_wrong_explicit_size',
                    commandline_args=[
                        '--cores', '4x4', '--corex', '1mm', '--corey', '2mm',
                        '--subcore-template', 'gainestown_core.flp'
                    ]
                ),
                FloorplanTestConfig(
                    name='gainestown_2x2',
                    commandline_args=[
                        '--cores', '2x2', '--subcore-template', 'gainestown_core.flp'
                    ]
                ),
                FloorplanTestConfig(
                    name='gainestown_4x4',
                    commandline_args=[
                        '--cores', '4x4', '--subcore-template', 'gainestown_core.flp'
                    ]
                ),
                FloorplanTestConfig(
                    name='gainestown_8x8',
                    commandline_args=[
                        '--cores', '8x8', '--subcore-template', 'gainestown_core.flp'
                    ]
                ),

    ]

    EXPECT_TO_FAIL_TESTS = [
        FloorplanTestConfig(
            name='subcore_template_does_not_exist',
            commandline_args=[
                '--cores', '8x8', '--corex', '4.31mm', '--corey', '2.08mm',
                '--subcore-template', 'gainestown_coreXX.flp'
            ]
        ),
        FloorplanTestConfig(
            name='2d_subcore_template_wrong_size',
            commandline_args=[
                '--cores', '8x8', '--corex', '4.0mm', '--corey', '2.0mm',
                '--subcore-template', 'gainestown_coreXX.flp'
            ]
        ),
    ]

    fails = 0
    total = 0
    seen_names = set()
    for test in TESTS:
        assert test.name not in seen_names
        seen_names.add(test.name)
        if not run(test):
            fails += 1
        total += 1

    for test in EXPECT_TO_FAIL_TESTS:
        assert test.name not in seen_names
        seen_names.add(test.name)
        if not run(test, expect_fail=True):
            fails += 1
        total += 1

    print('-'*50)
    if fails == 0:
        print(f'all {total} tests ok')
    else:
        print(f'{fails}/{total} tests failed')
        sys.exit(1)


if __name__ == '__main__':
    main()

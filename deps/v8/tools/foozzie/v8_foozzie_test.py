# Copyright 2016 the V8 project authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import sys
import unittest

import v8_foozzie
import v8_suppressions

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
FOOZZIE = os.path.join(BASE_DIR, 'v8_foozzie.py')
TEST_DATA = os.path.join(BASE_DIR, 'testdata')

class UnitTest(unittest.TestCase):
  def testDiff(self):
    # TODO (machenbach): Mock out suppression configuration. id:3862
    suppress = v8_suppressions.get_suppression(
        'x64', 'ignition', 'x64', 'ignition_turbo')
    one = ''
    two = ''
    diff = None, None
    self.assertEquals(diff, suppress.diff(one, two))

    one = 'a \n  b\nc();'
    two = 'a \n  b\nc();'
    diff = None, None
    self.assertEquals(diff, suppress.diff(one, two))

    # Ignore line before caret, caret position and error message.
    one = """
undefined
weird stuff
      ^
somefile.js: TypeError: undefined is not a function
  undefined
"""
    two = """
undefined
other weird stuff
            ^
somefile.js: TypeError: baz is not a function
  undefined
"""
    diff = None, None
    self.assertEquals(diff, suppress.diff(one, two))

    one = """
Still equal
Extra line
"""
    two = """
Still equal
"""
    diff = '- Extra line', None
    self.assertEquals(diff, suppress.diff(one, two))

    one = """
Still equal
"""
    two = """
Still equal
Extra line
"""
    diff = '+ Extra line', None
    self.assertEquals(diff, suppress.diff(one, two))

    one = """
undefined
somefile.js: TypeError: undefined is not a constructor
"""
    two = """
undefined
otherfile.js: TypeError: undefined is not a constructor
"""
    diff = """- somefile.js: TypeError: undefined is not a constructor
+ otherfile.js: TypeError: undefined is not a constructor""", None
    self.assertEquals(diff, suppress.diff(one, two))


def cut_verbose_output(stdout):
  return '\n'.join(stdout.split('\n')[2:])


def run_foozzie(first_d8, second_d8):
  return subprocess.check_output([
    sys.executable, FOOZZIE,
    '--random-seed', '12345',
    '--first-d8', os.path.join(TEST_DATA, first_d8),
    '--second-d8', os.path.join(TEST_DATA, second_d8),
    '--first-config', 'ignition',
    '--second-config', 'ignition_turbo',
    os.path.join(TEST_DATA, 'fuzz-123.js'),
  ])


class SystemTest(unittest.TestCase):
  def testSyntaxErrorDiffPass(self):
    stdout = run_foozzie('test_d8_1.py', 'test_d8_2.py')
    self.assertEquals('# V8 correctness - pass\n', cut_verbose_output(stdout))

  def testDifferentOutputFail(self):
    with open(os.path.join(TEST_DATA, 'failure_output.txt')) as f:
      expected_output = f.read()
    with self.assertRaises(subprocess.CalledProcessError) as ctx:
      run_foozzie('test_d8_1.py', 'test_d8_3.py')
    e = ctx.exception
    self.assertEquals(v8_foozzie.RETURN_FAIL, e.returncode)
    self.assertEquals(expected_output, cut_verbose_output(e.output))

#!/usr/bin/env python2

# Copyright (c) 2009 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

contents = r"""
#include <stdio.h>

void prog1(void)
{
  printf("Hello from make-prog1.py\n");
}
"""

open(sys.argv[1], 'w').write(contents)

sys.exit(0)

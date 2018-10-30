#!/usr/bin/env python
""" A command line utility that examines the PDG SCC to loop mapping as
exported by the IteratorRecognition LLVM pass.

The utility accepts the name of a file containing the condensation to loop
mapping in JSON format and outputs information and performs various sanity
checks on it.
"""

from __future__ import print_function

import sys
import os
import argparse
import logging
import json


def count_sccs(data):
    """ Count the number of SCC condensations.

    Args:
        data (binary): The input data in JSON format.
    """

    condensations = data['condensations']
    print('condensations: {}'.format(len(condensations)))


def is_valid_scc_to_loop_mapping(data, max_loops_per_condensation):
    """ Check if each SCC condensation is mapped to at most N loops.

    Args:
        data (binary): The input data in JSON format.
        max_loops_per_condendation (integer): The max number of loops per SCC
        condensation mapping.
    """

    status = True
    condensations = data['condensations']

    for c in condensations:
        if len(c['loops']) > max_loops_per_condensation:
            status = False
            print(
                'condensation: {} mapping to {} loops exceeds allowed maximum'.
                format(c['condensation'], len(c['loops'])))

    return status


#

if __name__ == '__main__':
    status = 0

    parser = argparse.ArgumentParser(
        description=
        'Extract information and perform checks on PDG SCC condensation to loop mapping.'
    )
    parser.add_argument('-q', '--quiet', action='store_true')
    parser.add_argument(
        '-f',
        '--file',
        metavar='FILENAME',
        nargs='?',
        type=argparse.FileType('r'),
        default=sys.stdin)
    parser.add_argument('-c', '--count-sccs', action='store_true')
    parser.add_argument(
        '-n',
        '--max-loops-per-condensation',
        metavar='N',
        type=int,
        nargs='?',
        default=-1,
        const=1)
    args = parser.parse_args()

    print = logging.info
    logging.basicConfig(
        level=logging.WARNING if args.quiet else logging.INFO,
        format='%(message)s')

    data = json.load(args.file)

    if args.count_sccs:
        count_sccs(data)

    if args.max_loops_per_condensation > -1 and not is_valid_scc_to_loop_mapping(
            data, args.max_loops_per_condensation):
        status = 1

    sys.exit(status)

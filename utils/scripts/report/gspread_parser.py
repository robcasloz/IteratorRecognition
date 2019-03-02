#!/usr/bin/env python
"""

"""

from __future__ import print_function

import sys
import os
import logging
import logging.config
import json
import yaml
import gspread
import time
import re

from argparse import ArgumentParser


def setup_logging(default_path='logging.yaml',
                  default_level=logging.INFO,
                  env_key='LOG_CFG'):
    """Setup logging configuration
    """
    path = default_path
    value = os.getenv(env_key, None)
    if value:
        path = value
    if os.path.exists(path):
        with open(path, 'rt') as f:
            config = yaml.safe_load(f.read())
        logging.config.dictConfig(config)
    else:
        logging.basicConfig(level=default_level)


class ResultParser(object):
    def __init__(self, logger=None):
        self.logger = logger or logging.getLogger(__name__)

    def parse():
        filename = '<google sheet name>'

        #OAuth login
        json_key = json.load(open('oauth.json'))
        """
            JSON in the form:
                {
                "private_key_id": "",
                "private_key": "",
                "client_email": "",
                "client_id": "",
                "type": "service_account"
                }
        """
        scope = ['https://spreadsheets.google.com/feeds']
        credentials = ServiceAccountCredentials(json_key['client_email'],
                                                json_key['private_key'], scope)
        gc = gspread.authorize(credentials)
        if gc:
            self.logger.info('OAuth succeeded')
        else:
            self.logger.warn('Oauth failed')

        now = time.strftime("%c")

        # get data from ruby script
        response = True
        if response:
            self.logger.info('Data collected')
        else:
            self.logger.warn('Could not collect data')

        csv = response.stdout
        csv = re.sub('/|"|,[0-9][0-9][0-9]Z|Z', '', csv)
        csv_lines = csv.split('\n')

        #get columns and rows for cell list
        column = len(csv_lines[0].split(","))
        row = 1
        for line in csv_lines:
            row += 1

        #create cell range
        columnletter = chr((column - 1) + ord('A'))
        cell_range = 'A1:%s%s' % (columnletter, row)

        #open the worksheet and create a new sheet
        wks = gc.open(filename)
        if wks:
            self.logger.info('%s file opened for writing', filename)
        else:
            self.logger.warn('%s file could not be opened', filename)

        sheet = wks.add_worksheet(title=now, rows=(row + 2), cols=(column + 2))
        cell_list = sheet.range(cell_range)

        #create values list
        csv = re.split("\n|,", csv)
        for item, cell in zip(csv, cell_list):
            cell.value = item

        # Update in batch
        if sheet.update_cells(cell_list):
            self.logger.info('upload to %s sheet in %s file done', now,
                             filename)
        else:
            self.logger.warn('upload to %s sheet in %s file failed', now,
                             filename)


if __name__ == '__main__':
    setup_logging()

    parser = ArgumentParser(
        description='Mark SCCs in graphs specified GraphViz DOT format')
    parser.add_argument(
        '-f',
        '--files',
        dest='dotfiles',
        nargs='*',
        required=True,
        help='GraphViz DOT files')
    parser.add_argument(
        '-m',
        '--mode',
        dest='mode',
        choices=['colour', 'subgraph'],
        default='subgraph',
        help='visual marking method of multinode SCCs')
    parser.add_argument(
        '-s',
        '--suffix',
        dest='suffix',
        default='scc',
        help='output file supplementary suffix')

    args = parser.parse_args()

    #

    sys.exit(0)

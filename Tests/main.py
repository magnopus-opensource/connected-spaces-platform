import argparse
import os
import requests
import sys
import threading
import time

from selenium import webdriver
from selenium.webdriver.common.by import By

import http_server


def start_http_server(server: http_server.LocalHttpServer):
    server.start()


def get_chrome_version():
    version = ''

    if sys.platform.startswith('win32'):
        # Get the current Chrome version by querying the registry
        stream = os.popen('reg query "HKEY_CURRENT_USER\Software\Google\Chrome\BLBeacon" /v version')
        version = stream.read()
        version = version[version.index('REG_SZ') + len('REG_SZ'):]
        version = version.strip()
    else:
        raise Exception('Unsupported platform!')
    
    return version


def fetch_chromedriver(version):
    # Delete old chromedriver if it exists
    chromedriver_filename = ''

    if sys.platform.startswith('win32'):
        chromedriver_filename = 'chromedriver.exe'
    
    if os.path.exists(chromedriver_filename):
        os.remove(chromedriver_filename)

    base_url = f'https://chromedriver.storage.googleapis.com/{version}/chromedriver_'
    url = ''

    if sys.platform.startswith('win32'):
        url = f'{base_url}win32.zip'
    
    with requests.get(url, stream=True) as r:
        print ("Downloading chromedriver...")
        r.raise_for_status()

        with open('chromedriver.archive', 'wb') as f:
            for chunk in r.iter_content(chunk_size=8192):
                f.write(chunk)
    
    # Extract chromedriver archive
    print('Extracting chromedriver...')

    if sys.platform.startswith('win32'):
        os.system('tar -xf chromedriver.archive')
    
    print('Done!')

    os.remove('chromedriver.archive')


if __name__ == '__main__':
    chrome_version = get_chrome_version()

    # Trim the patch version and download chromedriver
    chrome_version = chrome_version[:chrome_version.rindex('.')]
    chromedriver_latest_url = f'https://chromedriver.storage.googleapis.com/LATEST_RELEASE_{chrome_version}'
    chromedriver_latest_version = requests.get(chromedriver_latest_url).text
    fetch_chromedriver(chromedriver_latest_version)

    parser = argparse.ArgumentParser()
    parser.add_argument('--gtest_output')
    parser.add_argument('-t', '--run-test', action='append')
    parser.add_argument('--serve-only', action='store_true')
    args = parser.parse_args()

    server = http_server.LocalHttpServer()

    print("server about to start")
    start_http_server(server)
    print("server started")
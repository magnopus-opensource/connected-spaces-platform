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

    if args.serve_only:
        start_http_server(server)
    else:
        t = threading.Thread(target=start_http_server, args=(server,))
        t.start()

        time.sleep(2.0)

        opts = webdriver.ChromeOptions()
        opts.headless = True
        opts.add_experimental_option('excludeSwitches', ['enable-logging'])
        caps = webdriver.DesiredCapabilities.CHROME
        caps['goog:loggingPrefs'] = { 'driver': 'ALL', 'browser': 'ALL' }
        
        test_results = ""

        with webdriver.Chrome(options=opts, desired_capabilities=caps) as driver:
            url = 'http://localhost:8080/index.html'

            if args.run_test != None:
                url = url + '?test=' + args.run_test[0]

                for i in range(1, len(args.run_test)):
                    url = url + '&test=' + args.run_test[i]

            driver.get(url)

            # Wait for test results to be shown while piping through console log messages
            console_loop = True
            
            while console_loop:
                for entry in driver.get_log('browser'):
                    msg = entry['message']
                    msg_file_index = msg.index(' ')
                    msg_file = msg[0:msg_file_index]
                    msg_line_index = msg.index(' ', msg_file_index + 1)
                    msg_line = msg[msg_file_index + 1:msg_line_index]
                    msg_content = msg[msg_line_index + 1:]

                    if len(msg_content) == 0:
                        msg_content = msg

                    # Trim surrounding quotes
                    if msg_content[0] == '"':
                        msg_content = msg_content[1:-1]
                    
                    msg_content = msg_content.encode().decode('unicode_escape')

                    if entry['level'] == 'SEVERE':
                        print('ERROR:', msg_content, '[', msg_file, msg_line, ']')

                        if ('FatalError' in msg_content):
                            console_loop = False
                            break
                    else:
                        print(msg_content)
                
                try:
                    elem = driver.find_element(By.ID, 'test_results')
                    test_results = elem.text
                    break
                except Exception as err:
                    continue
            
            time.sleep(2)

            for entry in driver.get_log('browser'):
                msg = entry['message']
                msg_file_index = msg.index(' ')
                msg_file = msg[0:msg_file_index]
                msg_line_index = msg.index(' ', msg_file_index + 1)
                msg_line = msg[msg_file_index + 1:msg_line_index]
                msg_content = msg[msg_line_index + 1:]

                # Trim surrounding quotes
                if msg_content[0] == '"':
                    msg_content = msg_content[1:-1]
                
                msg_content = msg_content.encode().decode('unicode_escape')

                if entry['level'] == 'SEVERE':
                    print('ERROR:', msg_content, '[', msg_file, msg_line, ']')

                    if ('FatalError' in msg_content):
                        break
                else:
                    print(msg_content)

            driver.close()
            driver.quit()

        server.stop()
        t.join()

        if args.gtest_output != None:
            gtest_output = args.gtest_output.split(':', 1)

            if gtest_output[0] != 'xml':
                print('ERROR: Unknown GTest output format requested!')
            else:
                with open(gtest_output[1], 'w') as results_file:
                    results_file.write('<?xml version="1.0" encoding="UTF-8"?>')
                    results_file.write(test_results)
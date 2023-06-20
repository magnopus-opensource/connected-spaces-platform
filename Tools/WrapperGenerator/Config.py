config = {
    'cpp_export_macro': 'CSP_API',
    'c_export_macro': 'CSP_C_API',
    'no_export_macro': 'CSP_NO_EXPORT',
    'async_result_macro': 'CSP_ASYNC_RESULT',
    'async_result_with_progress_macro': 'CSP_ASYNC_RESULT_WITH_PROGRESS',
    'event_macro': 'CSP_EVENT',
    'out_macro': 'CSP_OUT',
    'in_out_macro': 'CSP_IN_OUT',
    'interface_macro': 'CSP_INTERFACE',
    'no_dispose_macro': 'CSP_NO_DISPOSE',
    'enum_flags_macro': 'CSP_FLAGS',
    'public_include_directory': '../../Library/include/',
    'template_directory': 'Templates/',
    'output_directory': 'Output/'
}



########################
#  DO NOT TOUCH THIS!  #
########################

import os

script_directory = os.path.dirname(os.path.realpath(__file__))
config['public_include_directory'] = f"{ script_directory }/{ config['public_include_directory'] }"
config['template_directory'] = f"{ script_directory }/{ config['template_directory'] }"
config['output_directory'] = f"{ script_directory }/{ config['output_directory'] }"
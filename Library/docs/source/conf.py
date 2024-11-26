# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))

from recommonmark.transform import AutoStructify

# -- Project information -----------------------------------------------------

project = 'Connected Spaces Platform'
copyright = 'Magnopus'
author = 'Magnopus'

# The full version, including alpha/beta/rc tags
release = 'v1.0.0'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    # there may be others here already, e.g. 'sphinx.ext.mathjax'
    'breathe',
    'exhale',
    'sphinx_rtd_theme',
    'sphinx_rtd_dark_mode',
    'sphinx_markdown_tables',
    'recommonmark'
]

# Setup the breathe extension
breathe_projects = {
    "Connected Spaces Platform": "../DoxygenOutput/xml"
}
breathe_default_project = "Connected Spaces Platform"

# Setup the exhale extension
exhale_args = {
    "containmentFolder":     "./api",
    "rootFileName":          "library_root.rst",
    "rootFileTitle":         "API DOCUMENTATION",
    "doxygenStripFromPath":  "..",
    "createTreeView":        True,
}

# Tell sphinx what the primary language being documented is.
primary_domain = 'cpp'

# Tell sphinx what the pygments highlight language should be.
highlight_language = 'cpp'

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#

# Default theme to dark mode (togglable on the page back to light mode)
default_dark_mode = True

html_theme = 'sphinx_rtd_theme'

html_logo = '_static/logo.png'

html_title = 'Connected Spaces Platform'

html_css_files = ["custom.css"]

# Don't show the 'View page source' link at the top
html_show_sourcelink = False

html_theme_options = {
    'logo_only': True,
    'collapse_navigation': False,
    'navigation_depth': -1
}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# Sphinx runs this for every module imported, app = instance of Sphinx
# This enables AutoStructify and recommonmark for markdown and enables embedded rst in md
def setup(app):
    app.add_config_value('recommonmark_config', {
        'auto_toc_tree_section': 'Contents',
        'enable_eval_rst': True,
    }, True)
    app.add_transform(AutoStructify)

rem Ensure Python prerequisites are met
python -m pip install --user -r requirements.txt

rem Remove existing documentation and intermediate files
@RD /S /Q "build"
@RD /S /Q "DoxygenOutput"
@RD /S /Q "source/api"

rem Generate XML formatted documentation from source via doxygen
doxygen Doxyfile

rem Generate HTML output via Python & Sphinx
make html
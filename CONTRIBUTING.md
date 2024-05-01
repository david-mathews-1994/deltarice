# Contributor Guide

Thank you for being interested in working on this software! This project is open-source under the [MIT License[(https://opensource.org/license/MIT) and we welcome contributions from the community. Please do so in the form of bug reports, feature requests, and pull requests.

If you want to contribute, see the following resources. 

- [Source Code](https://github.com/david-mathews-1994/deltarice/tree/master/src)
- [Installation Script](https://github.com/david-mathews-1994/deltarice/blob/master/setup.py)
- [Issue Tracker](https://github.com/david-mathews-1994/deltarice/issues)

## How to report a bug

Report bugs on the [Issue Tracker](https://github.com/david-mathews-1994/deltarice/issues).

When filing an issue, make sure to answer these questions:

- Which operating system and Python version are you using?
- Which version of this project are you using?
- What did you do?
- What did you expect to see?
- What did you see instead?

It is far easier to patch a bug with complete knowledge of the commands that were run so if possible please put the whole script/code in the bug report, or at least the relevant section, that would be ideal.

## How to request a feature

Request features on the [Issue Tracker](https://github.com/david-mathews-1994/deltarice/issues). Please be sure to describe the feature and if you have ideas as to how to implement let us know!

## How to set up your development environment

The same dependencies are required for development as they are for installation of the software (see [requirements.txt](https://github.com/david-mathews-1994/deltarice/blob/master/requirements.txt)), with the addition of the HDF5 C++ libraries for compilation. 
The library in particular is libhdf5-serial-dev that will need to be installed in order to compile the code. Note that for a MPI-enabled version of HDF5, this may be swapped for the specific non-serial version that you are interested in using. Note that this is
not an officially supported use-case of the library at this time, but we would be interested in supporting it. 

### How to test the project

Testing this project can be done via running the sample scripts provided here: [Python Example](https://github.com/david-mathews-1994/deltarice/blob/master/pythonExample.py], [C/C++ Example](https://github.com/david-mathews-1994/deltarice/blob/master/testCode.c).

## How to submit changes

Open a pull request to submit changes to this project.

Your pull request needs to meet the following guidelines for acceptance:

- The two test scripts need to pass without issue. 
- Ideally add additionally tests for your changes, either in the form of a new script, a git automated test, or a modification of the current scripts. 
- If your changes add functionality, update the documentation accordingly.
- Additionally this software MUST retain backwards compatibility. As large quantities of data are presently being stored with this method, it MUST remain backwards compatible with existing files.

It is recommended to open an issue before starting work on any major changes. This will allow a chance to talk it over with the owners and validate your approach.

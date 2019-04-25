README
======
This repository stores the documents of Huawei HiQ software.

Conf and utilize Sphinx for documentation
-----------------------------------------

We use Sphinx to organize coding documentations.
[The official Sphinx manual is linked here](https://www.sphinx-doc.org/en/master/contents.html).
The Chinese version can be found [here](https://zh-sphinx-doc.readthedocs.io/en/latest/index.html).
Below, we document some necessary steps that have been performed in practice.

Install and configure Sphinx on a local computer
------------------------------------------------

To install or update Sphinx, run the following command in a prompt terminal with Python 3.5+ and pip pre-installed,
.. code-block:: bash

    pip install -U sphinx
    

For this project, you will also need to install `pandoc` and `sphinx_rtd_theme` for markups of equations and the HTML theme:
.. code-block:: bash

    pip install pandoc
    pip install sphinx_rtd_theme

***Note***: depending on your Python environment, Sphinx might require other packages to be installed.
In most cases, you can learn what packages are required from error messages spilled out while compiling the Sphinx documents of the project.
Possible missing packages include `six`, `markupsafe`, `jinja2`, `pygments`, `docutils`, `snowballstemmer`, `pytz`, `babel`, and `alabaster`.

The config file of Huawei HiQ can be found at `/conf.py`.
Note that Sphinx was configured to generate static HTML files for ReadTheDocs website hosting and to be synchronized and linked with the [Github source repository](https://github.com/Huawei-HiQ/HiQsimulator/).
For now, we will ignore this setting and only allow local document generation using the Makefile script.
Main docs are written in  *reStructuredText* documents (`*.rst`).

In general, if no configuration was created, one can use `sphinx-quickstart` to initialize.
Five files and folders will be generated in the working directory: `Makefile`, `_build`, `_static`, `conf.py`, `index.rst`.
- `Makefile`: commands to compile Sphinx documents.
- `_build`: folder to store generated files.
- `_static`: folder to store non-source files, like figures.
- `config.py`: config file of the project.
- `index.rst`: root table of content of documents. All other documents generated will be linked in this file.

Of course, one can customize these files afterwards and not all files are necessarily tracked by git.
Comments or docStrings in reStructuredText formation from Python programs can be collected and reorganized to generate documents as configured.

Compile to generate static documents
------------------------------------
Sphinx allow one to generate documents in `HTML`, `LaTeX`, `PDF`, `epub`, `man` (UNIX Manual Pages), and so on.
To generate `HTML` documents, for instance, you can run the following in the root or documentation directory:
.. code-block:: bash
    make html

The generated documents will be placed under the `/_build/` directory.
Open the `index.html` in a web browser, you can find the table of content and the front page of the generated documents.

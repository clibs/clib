import glob
import json
import os
import os.path
import subprocess
import sys

doc_sections = [
    'summary',
    'description',
    # 'functions',
    'example',
    'author',
    'maintainer',
    'license',
    'see_also',
    ]

doc_extract_bin = 'tools/doc_extract'
depends_bin = 'tools/ccan_depends'


def get_source_path(module):
    return 'ccan/{module}'.format(module=module)


def get_dependencies(module):
    args = [depends_bin, get_source_path(module)]
    try:
        dependencies = subprocess.check_output(args)
    except subprocess.CalledProcessError:
        return {}
    else:
        return {d.replace('ccan/', 'clibs/'): '*'
                for d in dependencies.split('\n')
                if d != ''}


def get_src(module):
    src_path = get_source_path(module)
    files = glob.glob('{src_path}/*.c'.format(src_path=src_path))
    files.extend(glob.glob('{src_path}/*.h'.format(src_path=src_path)))
    return files


def get_summary(module):
    args = [doc_extract_bin, 'summary', "{src_path}/_info".format(src_path=get_source_path(module))]
    try:
        return subprocess.check_output(args).strip()
    except subprocess.CalledProcessError:
        return None


def ccan2repo(module, dst_path):
    dst_path = dst_path + '/' + module
    src_path = 'ccan/{module}'.format(module=module)

    if not os.path.isfile('{src_path}/_info'.format(src_path=src_path)):
        print "WARNING: {module} does not have a _info file".format(module=module)
        return

    try:
        os.mkdir(dst_path)
    except OSError:
        pass

    # Create package.json
    package = {
        "name": module,
        "version": "master",
        "repo": "rustyrussell/ccan",
        "src": get_src(module),
    }

    summary = get_summary(module)
    if summary:
        package['description'] = summary
    else:
        print "WARNING: {module} does not have a summary".format(module=module)

    dependencies = get_dependencies(module)
    if dependencies:
        package['dependencies'] = dependencies

    packagejson = open(dst_path + "/package.json", "w")
    packagejson.write(json.dumps(package, indent=2, sort_keys=True))
    packagejson.close()

    # Create source files
    # for src_fname in get_src(module):
    #    dst_fname = src_fname.replace(src_path, module)
    #    open(dst_fname, 'w').write(open(src_fname, 'r').read())

    # Create README.rst
    readme = open(dst_path + "/README.rst", "w")
    readme.write('This repository is a mirror of www.github.com/rustyrussell/ccan/tree/master/ccan/{module}\n\n'.format(module=module))
    for section in doc_sections:
        args = [doc_extract_bin, section, "{src_path}/_info".format(src_path=src_path)]

        try:
            text = subprocess.check_output(args).strip()
        except subprocess.CalledProcessError:
            continue

        if text == '': continue

        if section == 'example':
            indent = ' ' * 4
            text = ".. code-block:: c\n\n" + indent + ('\n' + indent).join(text.split('\n'))

        readme.writelines([
            section.title() + '\n',
            len(section) * '-' + '\n',
            text + '\n' * 2,
        ])
    readme.close()


ccan2repo(sys.argv[1], sys.argv[2])

#!/usr/bin/env python2
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2024 ArtInChip Technology Co., Ltd

import xml.etree.ElementTree as etree
from xml.etree.ElementTree import SubElement
from building import *
from utils import *
from utils import _make_path_relative
from utils import xml_indent
import platform
import shutil
import glob
import rtconfig


def is_subpath(parent, child):
    parent = os.path.normpath(parent)
    child = os.path.normpath(child)
    common_prefix = os.path.commonprefix([parent, child])
    return common_prefix == parent and child != parent


def remove_sdk_path(l_path, l_opt, root_path, aic_root, prj_out_dir):
    value = l_opt.attrib['value']
    value = value.replace('"', '')
    value = value.replace(root_path, '')
    # Remove old sdk path (luban-lite/*)
    # and don't remove custom's path (luban-lite/output/* or out of luban-lite/)
    d_path = os.path.normpath(os.path.join(aic_root, value))
    if d_path == aic_root:
        l_path.remove(l_opt)
    if os.path.exists(d_path) and is_subpath(aic_root, d_path) and \
       not is_subpath(prj_out_dir, d_path):
        l_path.remove(l_opt)


# Eclipse Project Type:
#
# sdk    update
# ---------------
# Fasle  Fasle      // (1) Create eclipse link, point to origin file.
# Fasle  True       // (2) Update eclipse link, when rerun menuconfig.
# True   Fasle      // (3) Copy file to eclipse project folder.
# True   True       // Not support.


def TargetEclipse(env, sdk=False, update=False):
    aic_root = os.path.normpath(os.environ["AIC_ROOT"])
    prj_out_dir = os.environ["PRJ_OUT_DIR"]
    if sdk:
        prj_eclipse_dir = os.path.join(aic_root, prj_out_dir + "../project_eclipse_sdk")
    else:
        prj_eclipse_dir = os.path.join(aic_root, prj_out_dir + "../project_eclipse")
    prj_eclipse_dir = os.path.normpath(prj_eclipse_dir)
    template_dir = os.path.join(aic_root, "tools/scripts/template/eclipse")
    template_dir = os.path.normpath(template_dir)
    proj = ProjectInfo(env)
    rel_path = os.path.relpath(prj_eclipse_dir, aic_root)
    rel_path_list = rel_path.split(os.sep)
    parent_lvl = len(rel_path_list)

    # (0) select and check template file
    if update:
        print('Update eclipse setting...')
        template_project = os.path.join(prj_eclipse_dir, '.project')
        template_cproject = os.path.join(prj_eclipse_dir, '.cproject')
        template_language_settings = os.path.join(prj_eclipse_dir, '.settings',
                                                  'language.settings.xml')
        template_core_prefs = os.path.join(prj_eclipse_dir, '.settings',
                                           'org.eclipse.cdt.core.prefs')
    else:
        print('Generate eclipse setting...')
        template_project = os.path.join(template_dir, 'template.project')
        template_cproject = os.path.join(template_dir, 'template.cproject')
        template_language_settings = os.path.join(template_dir, 'template.language.settings.xml')
        template_core_prefs = os.path.join(template_dir, 'template.org.eclipse.cdt.core.prefs')

    files = [template_project, template_cproject, template_language_settings, template_core_prefs]
    for f in files:
        if not os.path.exists(f):
            print('{} file not found!'.format(f))
            exit(100)

    # (1) generate '.project'
    # (1.1) read xml from template
    project = etree.parse(template_project)
    l_root = project.getroot()
    l_linkedResources = l_root.find('linkedResources')
    if update:
        if (l_linkedResources != None):
            # Remove old sdk file link (luban-lite/*)
            # and don't remove custom's file link (luban-lite/output/* or out of luban-lite/)
            for link in l_linkedResources.findall('link'):
                l_name = link.find('name').text
                l_type = link.find('type').text
                l_url = link.find('locationURI').text
                l_path = os.path.join(prj_eclipse_dir, l_name)
                l_des_path = os.path.join(aic_root, l_name)
                if os.path.exists(l_des_path) and is_subpath(aic_root, l_des_path) and \
                   not is_subpath(prj_out_dir, l_des_path):
                    l_linkedResources.remove(link)
        else:
            l_linkedResources = SubElement(l_root, 'linkedResources')
    else:
        if l_linkedResources != None:
            l_root.remove(l_linkedResources)
        l_linkedResources = SubElement(l_root, 'linkedResources')

    # (1.2) modify project name
    l_name = l_root.find("./name")
    l_name.text = rtconfig.SOC

    if sdk:
        print('Copy .c .h file...')
        # (1.3) create 'dir'
        #if os.path.exists(prj_eclipse_dir):
        #    shutil.rmtree(prj_eclipse_dir)
        tmp_dirs = []
        tmp_h_dirs = []

        # (1.4) copy '.c file'
        for f in proj['FILES']:
            des_f = os.path.relpath(f, aic_root)
            des_f = os.path.join(prj_eclipse_dir, des_f)
            des_d = os.path.dirname(des_f)
            if des_d not in tmp_dirs:
                tmp_dirs.append(des_d)
                try:
                    os.makedirs(des_d)
                except:
                    pass
            shutil.copy(f, des_f)

        # (1.5) copy '.h file'
        for f in proj['HEADERS']:
            src_d = os.path.dirname(f)
            des_f = os.path.relpath(f, aic_root)
            des_f = os.path.join(prj_eclipse_dir, des_f)
            des_d = os.path.dirname(des_f)
            if des_d not in tmp_dirs:
                tmp_dirs.append(des_d)
                try:
                    os.makedirs(des_d)
                except:
                    pass
            if des_d not in tmp_h_dirs:
                tmp_h_dirs.append(des_d)
                for f in glob.iglob(src_d + '/*.h'):
                    des_f = os.path.basename(f)
                    des_f = os.path.join(des_d, des_f)
                    shutil.copy(f, des_f)
    else:
        # (1.3) create 'dir' link
        for d in proj['DIRS']:
            d = os.path.relpath(d, aic_root)
            d = d.replace('\\', '/')
            l_link = SubElement(l_linkedResources, 'link')
            l_name = SubElement(l_link, 'name')
            l_name.text = d
            l_type = SubElement(l_link, 'type')
            l_type.text = '2'
            l_location = SubElement(l_link, 'locationURI')
            l_location.text = 'virtual:/virtual'

        # (1.4) create '.c file' link
        for f in proj['FILES']:
            f = os.path.relpath(f, aic_root)
            f = f.replace('\\', '/')
            l_link = SubElement(l_linkedResources, 'link')
            l_name = SubElement(l_link, 'name')
            l_name.text = f
            l_type = SubElement(l_link, 'type')
            l_type.text = '1'
            l_location = SubElement(l_link, 'locationURI')
            l_location.text = 'PARENT-{}-PROJECT_LOC/'.format(parent_lvl) + f

        # (1.5) create '.h file' link
        for f in proj['HEADERS']:
            f = os.path.relpath(f, aic_root)
            f = f.replace('\\', '/')
            l_link = SubElement(l_linkedResources, 'link')
            l_name = SubElement(l_link, 'name')
            l_name.text = f
            l_type = SubElement(l_link, 'type')
            l_type.text = '1'
            l_location = SubElement(l_link, 'locationURI')
            l_location.text = 'PARENT-{}-PROJECT_LOC/'.format(parent_lvl) + f

    # (1.6) write to '.project'
    if not os.path.exists(prj_eclipse_dir):
        os.makedirs(prj_eclipse_dir)
    out = open(os.path.join(prj_eclipse_dir, '.project'), 'w')
    out.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    xml_indent(l_root)
    out.write(etree.tostring(l_root, encoding='utf-8').decode('utf-8'))
    out.close()

    # (2) generate '.cproject'
    # (2.1) read xml from template
    cproject = etree.parse(template_cproject)
    l_root = cproject.getroot()

    # (2.2) 'debug' configuration
    l_configuration = l_root.find(".//configuration[@buildProperties='org.eclipse.cdt.build.core.buildArtefactType=org.eclipse.cdt.build.core.buildArtefactType.exe,org.eclipse.cdt.build.core.buildType=org.eclipse.cdt.build.core.buildType.debug']")

    # (2.2.1) 'target.other' configuration
    l_target = l_configuration.find(".//option[@superClass='ilg.gnumcueclipse.managedbuild.cross.riscv.option.target.other']")
    l_target.attrib['value'] = rtconfig.DEVICE

    # (2.2.2) 'assembler.defs' configuration
    l_defs = l_configuration.find(".//option[@superClass='ilg.gnumcueclipse.managedbuild.cross.riscv.option.assembler.defs']")
    # remove old Option
    for l_opt in l_defs.findall('listOptionValue'):
        l_defs.remove(l_opt)
    for d in proj['CPPDEFINES']:
        SubElement(l_defs, 'listOptionValue', builtIn="false", value=d)
    flags = env['ASFLAGS'].split()
    for flg in flags:
        if flg.startswith('-D'):
            SubElement(l_defs, 'listOptionValue', builtIn="false", value=flg.replace('-D', ''))

    # (2.2.3) 'assembler.include.paths' configuration
    l_path = l_configuration.find(".//option[@superClass='ilg.gnumcueclipse.managedbuild." +
                                  "cross.riscv.option.assembler.include.paths']")
    # remove old Option
    if sdk:
        root_path = "${ProjDirPath}\\"
    else:
        root_path = "${ProjDirPath}" + "\.." * parent_lvl + "\\"
    for l_opt in l_path.findall('listOptionValue'):
        if update:
            remove_sdk_path(l_path, l_opt, root_path, aic_root, prj_out_dir)
        else:
            l_path.remove(l_opt)
    for d in proj['CPPPATH']:
        d = os.path.relpath(d, aic_root)
        d = '"' + root_path + d + '"'
        SubElement(l_path, 'listOptionValue', builtIn="false", value=d)

    # (2.2.4) 'c.compiler.defs' configuration
    l_defs = l_configuration.find(".//option[@superClass='ilg.gnumcueclipse.managedbuild.cross.riscv.option.c.compiler.defs']")
    # remove old Option
    for l_opt in l_defs.findall('listOptionValue'):
        l_defs.remove(l_opt)
    for d in proj['CPPDEFINES']:
        SubElement(l_defs, 'listOptionValue', builtIn="false", value=d)

    # (2.2.5) 'c.compiler.include.paths' configuration
    l_path = l_configuration.find(".//option[@superClass='ilg.gnumcueclipse.managedbuild" +
                                  ".cross.riscv.option.c.compiler.include.paths']")
    # remove old Option
    for l_opt in l_path.findall('listOptionValue'):
        if update:
            remove_sdk_path(l_path, l_opt, root_path, aic_root, prj_out_dir)
        else:
            l_path.remove(l_opt)
    for d in proj['CPPPATH']:
        d = os.path.relpath(d, aic_root)
        d = '"' + root_path + d + '"'
        SubElement(l_path, 'listOptionValue', builtIn="false", value=d)

    # (2.2.6) 'c.linker.scriptfile' configuration
    l_path = l_configuration.find(".//option[@superClass='ilg.gnumcueclipse.managedbuild.cross.riscv.option.c.linker.scriptfile']")
    # remove old Option
    for l_opt in l_path.findall('listOptionValue'):
        l_path.remove(l_opt)
    flags = env['LINKFLAGS'].split()
    if '-T' in flags:
        index = flags.index('-T')
        src_f = flags[index + 1]
        rel_f = os.path.relpath(src_f, aic_root)
        f = '"' + root_path + rel_f + '"'
        if sdk:
            print('Copy .ld file...')
            # copy .ld file
            des_f = os.path.join(prj_eclipse_dir, rel_f)
            des_d = os.path.dirname(des_f)
            try:
                os.makedirs(des_d)
            except:
                pass
            shutil.copy(src_f, des_f)
        SubElement(l_path, 'listOptionValue', builtIn="false", value=f)

    # (2.2.7) 'c.linker.libs' configuration
    l_path = l_configuration.find(".//option[@superClass='ilg.gnumcueclipse.managedbuild.cross.riscv.option.c.linker.libs']")
    # remove old Option
    for l_opt in l_path.findall('listOptionValue'):
        l_path.remove(l_opt)
    if env.has_key('LIBS'):
        libs = env['LIBS']
    else:
        libs = []
    flags = env['LINKFLAGS'].split()
    for flg in flags:
        if flg.startswith('-l'):
            libs.append(flg.replace('-l', ''))
    libs = set(libs)
    for lib in libs:
        SubElement(l_path, 'listOptionValue', builtIn="false", value=lib)

    # (2.2.8) 'c.linker.libs' configuration
    l_path = l_configuration.find(".//option[@superClass='ilg.gnumcueclipse.managedbuild.cross.riscv.option.c.linker.paths']")
    # remove old Option
    for l_opt in l_path.findall('listOptionValue'):
        l_path.remove(l_opt)
    if env.has_key('LIBPATH'):
        libpath = env['LIBPATH']
        for src_d in libpath:
            rel_d = os.path.relpath(src_d, aic_root)
            if sdk:
                print('Copy lib file...')
                base_d = 'libs'
                d = '"' + root_path + base_d + '"'
                # copy lib file
                des_d = os.path.join(prj_eclipse_dir, base_d)
                try:
                    os.makedirs(des_d)
                except:
                    pass
                for f in glob.iglob(src_d + '/*'):
                    if f.endswith(".a"):
                        des_f = os.path.basename(f)
                        des_f = os.path.join(des_d, des_f)
                        shutil.copy(f, des_f)
            else:
                d = '"' + root_path + rel_d + '"'
            SubElement(l_path, 'listOptionValue', builtIn="false", value=d)

    # (2.2.9) post-build command 'make image'
    prj_chip = os.environ["PRJ_CHIP"]
    prj_board = os.environ["PRJ_BOARD"]
    post_cmd = ''
    if sdk:
        # copy  'target/chip/board/pack/*'
        print('Copy pack file...')
        src_d = os.path.join(aic_root, 'target', prj_chip, prj_board, 'pack')
        des_d = os.path.join(prj_eclipse_dir, 'target', prj_chip, prj_board, 'pack')
        try:
            os.makedirs(des_d)
        except:
            pass
        for f in glob.iglob(src_d + '/*'):
            des_f = os.path.basename(f)
            des_f = os.path.join(des_d, des_f)
            if os.path.isdir(f):
                if os.path.exists(des_f):
                    shutil.rmtree(des_f)
                shutil.copytree(f, des_f)
            else:
                shutil.copy(f, des_f)
        # copy  'bsp/artinchip/sys/chip/*.pbp'
        print('Copy .pbp file...')
        src_d = os.path.join(aic_root, 'bsp/artinchip/sys', prj_chip)
        des_d = os.path.join(prj_eclipse_dir, 'bsp/artinchip/sys', prj_chip)
        try:
            os.makedirs(des_d)
        except:
            pass
        for f in glob.iglob(src_d + '/*.pbp'):
            des_f = os.path.basename(f)
            des_f = os.path.join(des_d, des_f)
            shutil.copy(f, des_f)
        # copy script
        print('Copy scripts file...')
        des_d = os.path.join(prj_eclipse_dir, 'tools/scripts')
        try:
            os.makedirs(des_d)
        except:
            pass
        src_d = os.path.join(aic_root, 'tools/scripts/')
        for f in glob.iglob(src_d + '/*.py'):
            des_f = os.path.basename(f)
            des_f = os.path.join(des_d, des_f)
            shutil.copy(f, des_f)
        for f in glob.iglob(src_d + '/*.exe'):
            des_f = os.path.basename(f)
            des_f = os.path.join(des_d, des_f)
            shutil.copy(f, des_f)
        for f in glob.iglob(src_d + '/*.dll'):
            des_f = os.path.basename(f)
            des_f = os.path.join(des_d, des_f)
            shutil.copy(f, des_f)
        # copy bin
        print('Copy bin file...')
        src_d = os.path.join(aic_root, 'tools/env/tools/bin')
        des_d = os.path.join(prj_eclipse_dir, 'tools/bin')
        src_d = os.path.normpath(src_d);
        des_d = os.path.normpath(des_d);
        if not os.path.exists(des_d):
            if platform.system() == 'Linux':
                shutil.copytree(src_d, des_d)
            elif platform.system() == 'Windows':
                shutil.copytree('\\\\?\\' + src_d, '\\\\?\\' + des_d)
        # copy Python38
        print('Copy Python38 file...')
        src_d = os.path.join(aic_root, 'tools/env/tools/Python38')
        des_d = os.path.join(prj_eclipse_dir, 'tools/Python38')
        src_d = os.path.normpath(src_d);
        des_d = os.path.normpath(des_d);
        if not os.path.exists(des_d):
            if platform.system() == 'Linux':
                shutil.copytree(src_d, des_d)
            elif platform.system() == 'Windows':
                shutil.copytree('\\\\?\\' + src_d, '\\\\?\\' + des_d)
        # copy post_build.bat
        print('Copy post_build bat file...')
        src_d = os.path.join(prj_out_dir, 'post_build.bat')
        des_d = os.path.join(prj_eclipse_dir, 'tools')
        src_d = os.path.normpath(src_d)
        des_d = os.path.normpath(des_d)
        shutil.copy(src_d, des_d)
        # copy toolchain
        print('Copy toolchain file...')
        src_d = os.path.join(aic_root, 'toolchain')
        des_d = os.path.join(prj_eclipse_dir, 'toolchain')
        src_d = os.path.normpath(src_d);
        des_d = os.path.normpath(des_d);
        if not os.path.exists(des_d):
            if platform.system() == 'Linux':
                shutil.copytree(src_d, des_d)
            elif platform.system() == 'Windows':
                shutil.copytree('\\\\?\\' + src_d, '\\\\?\\' + des_d)
        # copy img src dir
        print('Copy image file...')
        dirs = []
        for i in range(2):
            if os.environ.has_key("img{}_srcdir".format(i)):
                dirs.append(os.environ["img{}_srcdir".format(i)])
        for src_d in dirs:
            des_d = os.path.relpath(src_d, aic_root)
            des_d = os.path.join(prj_eclipse_dir, des_d)
            src_d = os.path.normpath(src_d);
            des_d = os.path.normpath(des_d);
            if not os.path.exists(des_d):
                if platform.system() == 'Linux':
                    shutil.copytree(src_d, des_d)
                elif platform.system() == 'Windows':
                    shutil.copytree('\\\\?\\' + src_d, '\\\\?\\' + des_d)
        # post-build
        if os.environ.has_key("eclipse_sdk_post_build"):
            post_cmd = os.environ["eclipse_sdk_post_build"]
    else:
        # post-build
        if os.environ.has_key("eclipse_post_build"):
            post_cmd = os.environ["eclipse_post_build"]
    l_configuration.attrib['postbuildStep'] = post_cmd

    # (2.2.10) pre-build command
    prj_chip = os.environ["PRJ_CHIP"]
    prj_board = os.environ["PRJ_BOARD"]
    pre_cmd = ''
    if sdk:
        # pre-build
        if os.environ.has_key("eclipse_sdk_pre_build"):
            pre_cmd = os.environ["eclipse_sdk_pre_build"]
    else:
        # pre-build
        if os.environ.has_key("eclipse_pre_build"):
            pre_cmd = os.environ["eclipse_pre_build"]
    l_configuration.attrib['prebuildStep'] = pre_cmd

    # (2.2.11) user data directory
    prj_chip = os.environ["PRJ_CHIP"]
    prj_board = os.environ["PRJ_BOARD"]
    if sdk and os.environ.has_key("aic_fs_image_dir"):
        print('Copy image data directory...')
        aic_fs_image_dir = os.environ["aic_fs_image_dir"]
        src_d = os.path.join(aic_root, aic_fs_image_dir)
        des_d = os.path.join(prj_eclipse_dir, aic_fs_image_dir)
        src_d = os.path.normpath(src_d);
        des_d = os.path.normpath(des_d);
        if not os.path.exists(des_d):
            if platform.system() == 'Linux':
                shutil.copytree(src_d, des_d)
            elif platform.system() == 'Windows':
                shutil.copytree('\\\\?\\' + src_d, '\\\\?\\' + des_d)

    # (2.2.12) 'excluding'
    l_path = l_configuration.find(".//sourceEntries")
    if sdk:
        l_entry = l_path.find("entry")
        l_entry.attrib['excluding'] = "toolchain|tools"
    else:
        if l_path != None:
            l_configuration.remove(l_path)

    # (2.3) write to '.cproject'
    out = open(os.path.join(prj_eclipse_dir, '.cproject'), 'w')
    out.write('<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n')
    out.write('<?fileVersion 4.0.0?>')
    xml_indent(l_root)
    out.write(etree.tostring(l_root, encoding='utf-8').decode('utf-8'))
    out.close()
    #print(etree.tostring(l_root, encoding='utf-8').decode('utf-8'))

    # (3) generate '.settings/language.settings.xml'
    d = os.path.join(prj_eclipse_dir, ".settings")
    if not os.path.exists(d):
        os.makedirs(d)
    des = os.path.join(d, "language.settings.xml")
    src = template_language_settings
    if not update:
        shutil.copy(src, des)

    # (4) generate '.settings/org.eclipse.cdt.core.prefs'
    d = os.path.join(prj_eclipse_dir, ".settings")
    if not os.path.exists(d):
        os.makedirs(d)
    src_f = template_core_prefs
    des_f = os.path.join(d, "org.eclipse.cdt.core.prefs")
    with open(src_f, 'r') as f:
        template_prefs_str = f.read()
    if sdk:
        template_prefs_str = template_prefs_str.replace(r'\\..\\..\\..', '')
        template_prefs_str = template_prefs_str.replace(r'tools\\env\\tools\\bin', r'tools\\bin')
    with open(des_f, 'w') as f:
        f.write(template_prefs_str)

    # (2.3) write to 'post_build.bat'
    prj_chip = os.environ["PRJ_CHIP"]
    if sdk:
        src = os.path.join(prj_eclipse_dir, './tools/post_build.bat')
    else:
        src = os.path.join(prj_out_dir, './post_build.bat')
    if os.path.exists(src):
        post_build = None
        with open(src, 'r') as f:
            post_build = f.read()
        if sdk and post_build:
            post_build = post_build.replace(r';', '\n')
            des = prj_eclipse_dir + '/Debug/' + prj_chip
            post_build = post_build.replace(r'${ProjDirPath}/Debug/${ProjName}', des)
            des = prj_eclipse_dir + '/toolchain/bin/riscv64-unknown-elf-'
            post_build = post_build.replace(r'riscv64-unknown-elf-', des)
            des = prj_eclipse_dir + '/toolchain/riscv64-unknown-elf/bin/objcopy.exe'
            post_build = post_build.replace(r'${cross_prefix}${cross_objcopy}${cross_suffix}', des)
            des = prj_eclipse_dir + '/Debug/' + prj_chip
            post_build = post_build.replace(r'${ProjName}', des)
            des = prj_eclipse_dir
            post_build = post_build.replace(r'${ProjDirPath}', des)
        elif post_build:
            post_build = post_build.replace(r';', '\n')
            des = aic_root + '/toolchain/riscv64-unknown-elf/bin/objcopy.exe'
            post_build = post_build.replace(r'${cross_prefix}${cross_objcopy}${cross_suffix}', des)
            des = aic_root + '/tools/env/tools/Python38'
            post_build = post_build.replace(r'${ProjDirPath}/tools/Python38', des)
            des = aic_root + '/tools/env/tools/bin'
            post_build = post_build.replace(r'${ProjDirPath}/tools/bin', des)
            des = aic_root + '/' + prj_out_dir + prj_chip + '.elf'
            post_build = post_build.replace(r'${ProjDirPath}/Debug/${ProjName}.elf', des)
            des = aic_root + '/' + prj_out_dir
            post_build = post_build.replace(r'${ProjDirPath}/Debug/', des)
            des = aic_root + '/toolchain/bin/riscv64-unknown-elf-'
            post_build = post_build.replace(r'riscv64-unknown-elf-', des)
            post_build = post_build.replace(r'${ProjDirPath}', aic_root)
            des = aic_root + '/' + prj_out_dir + prj_chip + '.elf'
            post_build = post_build.replace(r'${ProjName}.elf', des)
            des = aic_root + '/' + prj_out_dir + prj_chip + '.bin'
            post_build = post_build.replace(r'${ProjName}.bin', des)
        with open(src, 'w') as f:
            if post_build:
                f.write(post_build)
            else:
                print("post_build.bat is invaild")
    else:
        print("post_build.bat is invaild")

    print('done!')
    exit(0)

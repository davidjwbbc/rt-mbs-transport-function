# License: 5G-MAG Public License (v1.0)
# Copyright: (C) 2024 British Broadcasting Corporation
#
# For full license terms please see the LICENSE file distributed with this
# program. If this file is missing then the license can be retrieved from
# https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view


libapp_dep = open5gs_project.get_variable('libapp_dep')
libcrypt_dep = open5gs_project.get_variable('libcrypt_dep')
libsbi_dep = open5gs_project.get_variable('libsbi_dep')
libsbi_openapi_dep = open5gs_project.get_variable('libsbi_openapi_dep')
open5gs_sysconfdir = open5gs_project.get_variable('open5gs_sysconfdir')
srcinc = open5gs_project.get_variable('srcinc')
libdir = open5gs_project.get_variable('libdir')
python3_exe = open5gs_project.get_variable('python3_exe')
mkdir_p = open5gs_project.get_variable('mkdir_p')
install_conf = open5gs_project.get_variable('install_conf')
sbi_openapi_inc = open5gs_project.get_variable('libsbi_openapi_model_inc')
app_type = 'MBSTF'


fs = import('fs')

libmbstf_dist_sources = files('''
    common.cc
    common.hh
    Context.cc
    Context.hh
    EventHandler.hh
    MBSTFEventHandler.cc
    MBSTFEventHandler.hh
    Open5GSEvent.hh
    Open5GSFSM.hh
    Open5GSNetworkFunction.cc
    Open5GSNetworkFunction.hh
    Open5GSSBIClient.cc
    Open5GSSBIClient.hh
    Open5GSSBIRequest.cc
    Open5GSSBIRequest.hh
    Open5GSSBIResponse.cc
    Open5GSSBIResponse.hh
    Open5GSSBIServer.cc
    Open5GSSBIServer.hh
    Open5GSStateMachine.cc
    Open5GSStateMachine.hh
    Open5GSTimer.hh
    Open5GSYamlDocument.hh
    Open5GSYamlIter.cc
    Open5GSYamlIter.hh
    TimerFunc.hh
'''.split())

version_conf = configuration_data()
version_conf.set_quoted('MBSTF_NAME', meson.project_name())
version_conf.set_quoted('MBSTF_VERSION', meson.project_version())
version_conf.set_quoted('MBSTF_TYPE', app_type)
libmbstf_gen_sources = [configure_file(output : 'mbstf-version.h', configuration : version_conf)]

libmbstf_sources = libmbstf_dist_sources + libmbstf_gen_sources

libmbstf = static_library('mbstf',
    sources : libmbstf_sources,
    include_directories : [sbi_openapi_inc],
    dependencies : [libsbi_dep,
                    libsbi_openapi_dep,
                    libapp_dep,
                    libcrypt_dep],
    install : false)

libmbstf_dep = declare_dependency(
    link_with : libmbstf,
    dependencies : [libsbi_dep,
                    libsbi_openapi_dep,
                    libapp_dep,
                    libcrypt_dep])

mbstf_sources = files('''
    App.cc
    App.hh
'''.split()) + open5gs_project.get_variable('app_main_c')

mbstf_config_source = '''
    mbstf.yaml
'''.split()

mbstf_include = include_directories('.')

executable('open5gs-mbstfd',
    sources : mbstf_sources,
    c_args : '-DDEFAULT_CONFIG_FILENAME="@0@/mbstf.yaml"'.format(open5gs_sysconfdir),
    cpp_args : '-DDEFAULT_CONFIG_FILENAME="@0@/mbstf.yaml"'.format(open5gs_sysconfdir),
    include_directories : [srcinc],
    dependencies : [libmbstf_dep],
    install_rpath : libdir,
    install : true)

meson.add_install_script(python3_exe, '-c', mkdir_p.format(open5gs_sysconfdir))
conf_configuration = configuration_data()
foreach conf_file : mbstf_config_source
    gen = configure_file(input : conf_file + '.in', configuration : conf_configuration, output : conf_file)
    meson.add_install_script(python3_exe, '-c', install_conf.format(gen.full_path(), open5gs_sysconfdir))
endforeach

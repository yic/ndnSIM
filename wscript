## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os
from waflib import Logs, Utils, Options, TaskGen, Task
from waflib.Errors import WafError

import wutils

def options(opt):
    opt.add_option('--enable-ndn-plugins',
                   help=("Enable NDN plugins (may require patching).  topology plugin enabled by default"),
                   dest='enable_ndn_plugins')

    opt.add_option('--disable-ndn-plugins',
                   help=("Enable NDN plugins (may require patching).  topology plugin enabled by default"),
                   dest='disable_ndn_plugins')

REQUIRED_BOOST_LIBS = ['graph']

def required_boost_libs(conf):
    conf.env.REQUIRED_BOOST_LIBS += REQUIRED_BOOST_LIBS

def configure(conf):
    conf.env['ENABLE_NDNSIM']=False;

    if not conf.env['LIB_BOOST']:
        conf.report_optional_feature("ndnSIM", "ndnSIM", False,
                                     "Required boost libraries not found")
        Logs.error ("ndnSIM will not be build as it requires boost libraries of version at least 1.46")
        conf.env['MODULES_NOT_BUILT'].append('ndnSIM')
        return
    else:
        present_boost_libs = []
        for boost_lib_name in conf.env['LIB_BOOST']:
            if boost_lib_name.startswith("boost_"):
                boost_lib_name = boost_lib_name[6:]
            if boost_lib_name.endswith("-mt"):
                boost_lib_name = boost_lib_name[:-3]
            present_boost_libs.append(boost_lib_name)

        missing_boost_libs = [lib for lib in REQUIRED_BOOST_LIBS if lib not in present_boost_libs]
        
        if missing_boost_libs != []:
            conf.report_optional_feature("ndnSIM", "ndnSIM", False,
                                         "ndnSIM requires boost libraries: %s" % ' '.join(missing_boost_libs))
            conf.env['MODULES_NOT_BUILT'].append('ndnSIM')

            Logs.error ("ndnSIM will not be build as it requires boost libraries: %s" % ' '.join(missing_boost_libs))
            Logs.error ("Please upgrade your distribution or install custom boost libraries (http://ndnsim.net/faq.html#boost-libraries)")
            return
            
        boost_version = conf.env.BOOST_VERSION.split('_')
        if int(boost_version[0]) < 1 or int(boost_version[1]) < 46:
            conf.report_optional_feature("ndnSIM", "ndnSIM", False,
                                         "ndnSIM requires at least boost version 1.46")
            conf.env['MODULES_NOT_BUILT'].append('ndnSIM')

            Logs.error ("ndnSIM will not be build as it requires boost libraries of version at least 1.46")
            Logs.error ("Please upgrade your distribution or install custom boost libraries (http://ndnsim.net/faq.html#boost-libraries)")
            return

    conf.env['NDN_plugins'] = ['topology']
    if Options.options.enable_ndn_plugins:
        conf.env['NDN_plugins'] = conf.env['NDN_plugins'] + Options.options.enable_ndn_plugins.split(',')

    if Options.options.disable_ndn_plugins:
        conf.env['NDN_plugins'] = conf.env['NDN_plugins'] - Options.options.disable_ndn_plugins.split(',')

    conf.env['ENABLE_NDNSIM']=True;
    conf.env['MODULES_BUILT'].append('ndnSIM')

    conf.report_optional_feature("ndnSIM", "ndnSIM", True, "")

def build(bld):
    deps = ['core', 'network', 'point-to-point']
    deps.append ('internet') # Until RttEstimator is moved to network module
    if bld.env['ENABLE_PYTHON_BINDINGS']:
        deps.append ('visualizer')

    if 'topology' in bld.env['NDN_plugins']:
        deps.append ('topology-read')
        deps.append ('mobility')

    if 'mobility' in bld.env['NDN_plugins']:
        deps.append ('mobility')

    module = bld.create_ns3_module ('ndnSIM', deps)
    module.module = 'ndnSIM'
    module.features += ' ns3fullmoduleheaders'
    module.uselib = 'BOOST BOOST_IOSTREAMS'

    headers = bld (features='ns3header')
    headers.module = 'ndnSIM'

    if not bld.env['ENABLE_NDNSIM']:
        bld.env['MODULES_NOT_BUILT'].append('ndnSIM')
        return

    module.source = bld.path.ant_glob(['model/**/*.cc',
                                       'apps/*.cc',
                                       'utils/**/*.cc',
                                       'helper/**/*.cc',
                                       ])
    module.full_headers = [p.path_from(bld.path) for p in bld.path.ant_glob([
                           'utils/**/*.h',
                           'model/**/*.h',
                           'apps/**/*.h',
                           'helper/**/*.h',
                           ])]

    headers.source = [
        "helper/ndn-stack-helper.h",
        "helper/ndn-app-helper.h",
        "helper/ndn-header-helper.h",
        "helper/ndn-face-container.h",
        "helper/ndn-global-routing-helper.h",

        "apps/ndn-app.h",

        "model/ndn-l3-protocol.h",
        "model/ndn-face.h",
        "model/ndn-app-face.h",
        "model/ndn-net-device-face.h",
        "model/ndn-interest.h",
        "model/ndn-content-object.h",
        "model/ndn-name-components.h",
        "model/ndn-name.h",

        "model/cs/ndn-content-store.h",

        "model/fib/ndn-fib.h",
        "model/fib/ndn-fib-entry.h",

        "model/pit/ndn-pit.h",
        "model/pit/ndn-pit-entry.h",
        "model/pit/ndn-pit-entry-incoming-face.h",
        "model/pit/ndn-pit-entry-outgoing-face.h",

        "model/fw/ndn-forwarding-strategy.h",
        "model/fw/ndn-fw-tag.h",

        # "utils/batches.h",
        "utils/ndn-limits.h",
        "utils/ndn-rtt-estimator.h",
        # "utils/weights-path-stretch-tag.h",
        "utils/ndn-drop-tail-queue.h",

        ]

    if 'topology' in bld.env['NDN_plugins']:
        headers.source.extend ([
            "plugins/topology/rocketfuel-weights-reader.h",
            "plugins/topology/annotated-topology-reader.h",
            ])
        module.source.extend (bld.path.ant_glob(['plugins/topology/*.cc']))
        module.full_headers.extend ([p.path_from(bld.path) for p in bld.path.ant_glob(['plugins/topology/**/*.h'])])

    if 'mobility' in bld.env['NDN_plugins']:
        headers.source.extend ([
            "plugins/mobility/spring-mobility-model.h",
            "plugins/mobility/spring-mobility-helper.h",
            ])
        module.source.extend (bld.path.ant_glob(['plugins/mobility/*.cc']))
        module.full_headers.extend ([p.path_from(bld.path) for p in bld.path.ant_glob(['plugins/mobility/**/*.h'])])

    # bld.install_files('${INCLUDEDIR}/%s%s/ns3/ndnSIM' % (wutils.APPNAME, wutils.VERSION), ndnSIM_headers, relative_trick=True)
    # bld.install_files('$PREFIX/include', ndnSIM_headers)

    tests = bld.create_ns3_module_test_library('ndnSIM')
    tests.source = bld.path.ant_glob('test/*.cc')

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse ('examples')

    bld.recurse ('tools')

    bld.ns3_python_bindings()


@TaskGen.feature('ns3fullmoduleheaders')
@TaskGen.after_method('process_rule')
def apply_ns3fullmoduleheaders(self):
    # ## get all of the ns3 headers
    ns3_dir_node = self.bld.path.find_or_declare("ns3")

    mode = getattr(self, "mode", "install")

    for filename in set(self.to_list(self.full_headers)):
        src_node = self.path.find_resource(filename)
        if src_node is None:
            raise WafError("source ns3 header file %s not found" % (filename,))
        dst_node = ns3_dir_node.find_or_declare(src_node.path_from(self.bld.path.find_dir('src')))
        assert dst_node is not None

        relpath = src_node.parent.path_from(self.bld.path.find_dir('src'))

        task = self.create_task('ns3header')
        task.mode = getattr(self, 'mode', 'install')
        if task.mode == 'install':
            self.bld.install_files('${INCLUDEDIR}/%s%s/ns3/%s' % (wutils.APPNAME, wutils.VERSION, relpath),
                                   [src_node])
            task.set_inputs([src_node])
            task.set_outputs([dst_node])
        else:
            task.header_to_remove = dst_node


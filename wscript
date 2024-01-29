## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    module = bld.create_ns3_module('sprite', ['internet', 'config-store','stats'])
    module.source = [
        'model/BLANC.cpp',
        'model/BLANC++.cpp',
        'model/SpeedyM.cpp',
	    'model/blanc-header.cc',
	    'helper/blanc-app-helper.cpp'
        ]


    headers = bld(features='ns3header')
    headers.module = 'applications'
    headers.source = [
        'model/BLANC.hpp',
        'model/BLANC++.hpp',
        'model/SpeedyM.hpp',
	    'model/blanc-header.h',
	    'helper/blanc-app-helper.hpp'
        ]
    
    if (bld.env['ENABLE_EXAMPLES']):
        bld.recurse('examples')

    bld.ns3_python_bindings()

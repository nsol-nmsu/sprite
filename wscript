## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    module = bld.create_ns3_module('sprite', ['internet', 'config-store','stats'])
    module.source = [
        'model/PCN-App-Base.cpp',
        'model/BLANC.cpp',
        'model/Sprite.cpp',
        'model/SpeedyM.cpp',
	    'model/blanc-header.cc',
        'model/synch.cpp',
        'model/BLANC-sync.cpp',
        'model/Sprite-sync.cpp',
        'model/SpeedyM-sync.cpp',
	    'helper/blanc-app-helper.cpp'
        ]


    headers = bld(features='ns3header')
    headers.module = 'sprite'
    headers.source = [
        'model/PCN-App-Base.hpp',
        'model/BLANC.hpp',
        'model/Sprite.hpp',
        'model/SpeedyM.hpp',
	    'model/blanc-header.h',
        'model/synch.hpp',
        'model/BLANC-sync.hpp',
        'model/Sprite-sync.hpp',
        'model/SpeedyM-sync.hpp',
	    'helper/blanc-app-helper.hpp'
        ]
    
    if (bld.env['ENABLE_EXAMPLES']):
        bld.recurse('examples')

    bld.ns3_python_bindings()

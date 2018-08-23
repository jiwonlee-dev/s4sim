# Copyright (c) 2010-2013 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2012-2014 Mark D. Hill and David A. Wood
# Copyright (c) 2009-2011 Advanced Micro Devices, Inc.
# Copyright (c) 2006-2007 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Ali Saidi
#          Brad Beckmann
#import numpy as numpy 
import optparse
import sys

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import addToPath, fatal

addToPath('gem5-stable_2015_09_03/configs/common')
addToPath('gem5-stable_2015_09_03/configs/ruby')

import Ruby

from FSConfig import *
from SysPaths import *
from Benchmarks import *
import Simulation
import CacheConfig
import MemConfig
from Caches import *
import Options

import os


# Check if KVM support has been enabled, we might need to do VM
# configuration if that's the case.
have_kvm_support = 'BaseKvmCPU' in globals()
def is_kvm_cpu(cpu_class):
    return have_kvm_support and cpu_class != None and \
        issubclass(cpu_class, BaseKvmCPU)

def cmd_line_template():
    if options.command_line and options.command_line_file:
        print "Error: --command-line and --command-line-file are " \
              "mutually exclusive"
        sys.exit(1)
    if options.command_line:
        return options.command_line
    if options.command_line_file:
        return open(options.command_line_file).read().strip()
    return None

def build_test_system(np):
    cmdline = cmd_line_template()
    if buildEnv['TARGET_ISA'] == "alpha":
        test_sys = makeLinuxAlphaSystem(test_mem_mode, bm[0], options.ruby,
                                        cmdline=cmdline)
    elif buildEnv['TARGET_ISA'] == "mips":
        test_sys = makeLinuxMipsSystem(test_mem_mode, bm[0], cmdline=cmdline)
    elif buildEnv['TARGET_ISA'] == "sparc":
        test_sys = makeSparcSystem(test_mem_mode, bm[0], cmdline=cmdline)
    elif buildEnv['TARGET_ISA'] == "x86":
        test_sys = makeLinuxX86System(test_mem_mode, options.num_cpus, bm[0],
                options.ruby, cmdline=cmdline)
    elif buildEnv['TARGET_ISA'] == "arm":
        test_sys = makeArmSystem(test_mem_mode, options.machine_type,
                                 options.num_cpus, bm[0], options.dtb_filename,
                                 bare_metal=options.bare_metal,
                                 cmdline=cmdline,
                                 external_memory=options.external_memory_system)
        if options.enable_context_switch_stats_dump:
            test_sys.enable_context_switch_stats_dump = True
    else:
        fatal("Incapable of building %s full system!", buildEnv['TARGET_ISA'])

    # Set the cache line size for the entire system
    test_sys.cache_line_size = options.cacheline_size

    # Create a top-level voltage domain
    test_sys.voltage_domain = VoltageDomain(voltage = options.sys_voltage)

    # Create a source clock for the system and set the clock period
    test_sys.clk_domain = SrcClockDomain(clock =  options.sys_clock,
            voltage_domain = test_sys.voltage_domain)

    # Create a CPU voltage domain
    test_sys.cpu_voltage_domain = VoltageDomain()

    # Create a source clock for the CPUs and set the clock period
    test_sys.cpu_clk_domain = SrcClockDomain(clock = options.cpu_clock,
                                             voltage_domain =
                                             test_sys.cpu_voltage_domain)

    if options.kernel is not None:
        test_sys.kernel = binary(options.kernel)

    if options.script is not None:
        test_sys.readfile = options.script

    if options.lpae:
        test_sys.have_lpae = True

    if options.virtualisation:
        test_sys.have_virtualization = True

    #change the bootloader here
    #print "change boot loader"
    #print test_sys.boot_loader
    test_sys.boot_loader = options.issd_bootloader 
    #print test_sys.boot_loader



    test_sys.init_param = options.init_param

    # For now, assign all the CPUs to the same clock domain
    test_sys.cpu = [TestCPUClass(clk_domain=test_sys.cpu_clk_domain, cpu_id=i)
                    for i in xrange(np)]

    if is_kvm_cpu(TestCPUClass) or is_kvm_cpu(FutureClass):
        test_sys.vm = KvmVM()

    if options.ruby:
        # Check for timing mode because ruby does not support atomic accesses
        if not (options.cpu_type == "detailed" or options.cpu_type == "timing"):
            print >> sys.stderr, "Ruby requires TimingSimpleCPU or O3CPU!!"
            sys.exit(1)

        Ruby.create_system(options, True, test_sys, test_sys.iobus,
                           test_sys._dma_ports)

        # Create a seperate clock domain for Ruby
        test_sys.ruby.clk_domain = SrcClockDomain(clock = options.ruby_clock,
                                        voltage_domain = test_sys.voltage_domain)

        # Connect the ruby io port to the PIO bus,
        # assuming that there is just one such port.
        test_sys.iobus.master = test_sys.ruby._io_port.slave

        for (i, cpu) in enumerate(test_sys.cpu):
            #
            # Tie the cpu ports to the correct ruby system ports
            #
            cpu.clk_domain = test_sys.cpu_clk_domain
            cpu.createThreads()
            cpu.createInterruptController()

            cpu.icache_port = test_sys.ruby._cpu_ports[i].slave
            cpu.dcache_port = test_sys.ruby._cpu_ports[i].slave

            if buildEnv['TARGET_ISA'] == "x86":
                cpu.itb.walker.port = test_sys.ruby._cpu_ports[i].slave
                cpu.dtb.walker.port = test_sys.ruby._cpu_ports[i].slave

                cpu.interrupts.pio = test_sys.ruby._cpu_ports[i].master
                cpu.interrupts.int_master = test_sys.ruby._cpu_ports[i].slave
                cpu.interrupts.int_slave = test_sys.ruby._cpu_ports[i].master

    else:
        if options.caches or options.l2cache:
            # By default the IOCache runs at the system clock
            test_sys.iocache = IOCache(addr_ranges = test_sys.mem_ranges)
            test_sys.iocache.cpu_side = test_sys.iobus.master
            test_sys.iocache.mem_side = test_sys.membus.slave
        elif not options.external_memory_system:
            test_sys.iobridge = Bridge(delay='50ns', ranges = test_sys.mem_ranges)
            test_sys.iobridge.slave = test_sys.iobus.master
            test_sys.iobridge.master = test_sys.membus.slave

        # Sanity check
        if options.fastmem:
            if TestCPUClass != AtomicSimpleCPU:
                fatal("Fastmem can only be used with atomic CPU!")
            if (options.caches or options.l2cache):
                fatal("You cannot use fastmem in combination with caches!")

        if options.simpoint_profile:
            if not options.fastmem:
                # Atomic CPU checked with fastmem option already
                fatal("SimPoint generation should be done with atomic cpu and fastmem")
            if np > 1:
                fatal("SimPoint generation not supported with more than one CPUs")

        for i in xrange(np):
            if options.fastmem:
                test_sys.cpu[i].fastmem = True
            if options.simpoint_profile:
                test_sys.cpu[i].addSimPointProbe(options.simpoint_interval)
            if options.checker:
                test_sys.cpu[i].addCheckerCpu()
            test_sys.cpu[i].createThreads()

        CacheConfig.config_cache(options, test_sys)
        MemConfig.config_mem(options, test_sys)

    return test_sys

def build_drive_system(np):
    # driver system CPU is always simple, so is the memory
    # Note this is an assignment of a class, not an instance.
    DriveCPUClass = AtomicSimpleCPU
    drive_mem_mode = 'atomic'
    DriveMemClass = SimpleMemory

    cmdline = cmd_line_template()
    if buildEnv['TARGET_ISA'] == 'alpha':
        drive_sys = makeLinuxAlphaSystem(drive_mem_mode, bm[1], cmdline=cmdline)
    elif buildEnv['TARGET_ISA'] == 'mips':
        drive_sys = makeLinuxMipsSystem(drive_mem_mode, bm[1], cmdline=cmdline)
    elif buildEnv['TARGET_ISA'] == 'sparc':
        drive_sys = makeSparcSystem(drive_mem_mode, bm[1], cmdline=cmdline)
    elif buildEnv['TARGET_ISA'] == 'x86':
        drive_sys = makeLinuxX86System(drive_mem_mode, np, bm[1],
                                       cmdline=cmdline)
    elif buildEnv['TARGET_ISA'] == 'arm':
        drive_sys = makeArmSystem(drive_mem_mode, options.machine_type, np,
                                  bm[1], options.dtb_filename, cmdline=cmdline)

    # Create a top-level voltage domain
    drive_sys.voltage_domain = VoltageDomain(voltage = options.sys_voltage)

    # Create a source clock for the system and set the clock period
    drive_sys.clk_domain = SrcClockDomain(clock =  options.sys_clock,
            voltage_domain = drive_sys.voltage_domain)

    # Create a CPU voltage domain
    drive_sys.cpu_voltage_domain = VoltageDomain()

    # Create a source clock for the CPUs and set the clock period
    drive_sys.cpu_clk_domain = SrcClockDomain(clock = options.cpu_clock,
                                              voltage_domain =
                                              drive_sys.cpu_voltage_domain)

    drive_sys.cpu = DriveCPUClass(clk_domain=drive_sys.cpu_clk_domain,
                                  cpu_id=0)
    drive_sys.cpu.createThreads()
    drive_sys.cpu.createInterruptController()
    drive_sys.cpu.connectAllPorts(drive_sys.membus)
    if options.fastmem:
        drive_sys.cpu.fastmem = True
    if options.kernel is not None:
        drive_sys.kernel = binary(options.kernel)

    if is_kvm_cpu(DriveCPUClass):
        drive_sys.vm = KvmVM()

    drive_sys.iobridge = Bridge(delay='50ns',
                                ranges = drive_sys.mem_ranges)
    drive_sys.iobridge.slave = drive_sys.iobus.master
    drive_sys.iobridge.master = drive_sys.membus.slave

    # Create the appropriate memory controllers and connect them to the
    # memory bus
    drive_sys.mem_ctrls = [DriveMemClass(range = r)
                           for r in drive_sys.mem_ranges]
    for i in xrange(len(drive_sys.mem_ctrls)):
        drive_sys.mem_ctrls[i].port = drive_sys.membus.master

    drive_sys.init_param = options.init_param

    return drive_sys

# Add options
parser = optparse.OptionParser()

# Add iSSD command line options
parser.add_option('--issd_bootloader')
parser.add_option('--nand_type', type="string", default="slc" , help="Flash memory Type ")
parser.add_option('--nand_storage_image', help="Nand image file")
parser.add_option('--nand_num_chn', type="int" , default=2, help="Number of NAND Controller Channels ")
parser.add_option('--nand_pkgs_per_chn', help="Number of packeages on each channel ")
parser.add_option('--nand_dies_per_pkg', help="Number of dies is a package")
parser.add_option('--nand_planes_per_die', help="Number of planes in each sie/chip ")
parser.add_option('--nand_blocks_per_plane', help="Number of blocks on each plane ")
parser.add_option('--nand_pages_per_block', help="Number of pages in a block ")
parser.add_option('--nand_page_size', help="Page size : expressed in terms of DS(data shift) , nand_page_size=9 is 512 bytes in a page (1<<9) ")

#latencies
parser.add_option('--nand_ltcy_rcmd_issue', help="Issue read command to NAND die latency")
parser.add_option('--nand_ltcy_pcmd_issue', help="Issue program command to NAND die latency")
parser.add_option('--nand_ltcy_ecmd_issue', help="Issue erase command to NAND die latency")
parser.add_option('--nand_ltcy_read_page', help="Read page latency")
parser.add_option('--nand_ltcy_program_page', help="Program page latency")
parser.add_option('--nand_ltcy_erase_block', help="Erase block latency")

parser.add_option('--nand_ltcy_read_msb_page', help="-")
parser.add_option('--nand_ltcy_read_lsb_page', help="-")
parser.add_option('--nand_ltcy_program_msb_page', help="-")
parser.add_option('--nand_ltcy_program_lsb_page', help="-")
parser.add_option('--nand_ltcy_read_csb_page', help="-")
parser.add_option('--nand_ltcy_program_csb_page', help="-")
parser.add_option('--nand_ltcy_read_chsb_page', help="-")
parser.add_option('--nand_ltcy_read_clsb_page', help="-")
parser.add_option('--nand_ltcy_program_chsb_page', help="-")
parser.add_option('--nand_ltcy_program_clsb_page', help="-")


Options.addCommonOptions(parser)
Options.addFSOptions(parser)

# Add the ruby specific and protocol specific options
if '--ruby' in sys.argv:
    Ruby.define_options(parser)

(options, args) = parser.parse_args()

if args:
    print "Error: script doesn't take any positional arguments"
    sys.exit(1)

# system under test can be any CPU
(TestCPUClass, test_mem_mode, FutureClass) = Simulation.setCPUClass(options)

# Match the memories with the CPUs, based on the options for the test system
TestMemClass = Simulation.setMemClass(options)

if options.benchmark:
    try:
        bm = Benchmarks[options.benchmark]
    except KeyError:
        print "Error benchmark %s has not been defined." % options.benchmark
        print "Valid benchmarks are: %s" % DefinedBenchmarks
        sys.exit(1)
else:
    if options.dual:
        bm = [SysConfig(disk=options.disk_image, rootdev=options.root_device,
                        mem=options.mem_size, os_type=options.os_type),
              SysConfig(disk=options.disk_image, rootdev=options.root_device,
                        mem=options.mem_size, os_type=options.os_type)]
    else:
        bm = [SysConfig(disk=options.disk_image, rootdev=options.root_device,
                        mem=options.mem_size, os_type=options.os_type)]

np = options.num_cpus

test_sys = build_test_system(np)
if len(bm) == 2:
    drive_sys = build_drive_system(np)
    root = makeDualRoot(True, test_sys, drive_sys, options.etherdump)
elif len(bm) == 1:
    root = Root(full_system=True, system=test_sys)
else:
    print "Error I don't know how to create more than 2 systems."
    sys.exit(1)

if options.timesync:
    root.time_sync_enable = True

if options.frame_capture:
    VncServer.frame_capture = True

# Add iSSD specific components 
test_sys.NVMeInterface  = iSSDHostInterface(gic = test_sys.realview.gic , pio_addr=0x2d100000, pio_size=0x8000 ,int_num=36)
test_sys.NVMeInterface.pio = test_sys.membus.master
test_sys.NVMeInterface.dma = test_sys.membus.slave
test_sys.NVMeInterface.checkDoorBell_latency = '10ns'

test_sys.realview.timer0.int_num0 = 57
test_sys.realview.timer0.int_num1 = 57


num_chn = options.nand_num_chn
nand_ctrl_count = int(options.nand_num_chn / 2)
nand_ctrl_base = 0x2d108000 
NandCtrl = []
index_num = 0
chn_index = 0 

print "media type : %s" % options.nand_type 

for nc in xrange(nand_ctrl_count):
        
    nand_ctrl = iSSDNandCtrl(gic = test_sys.realview.gic , pio_addr=nand_ctrl_base, pio_size=0x1000 ,int_num=58 )
    print "Nand Ctrl base address %x" % nand_ctrl_base

    nand_ctrl_base = nand_ctrl_base + 0x1000

    nand_ctrl.pio = test_sys.membus.master
    nand_ctrl.dma = test_sys.membus.slave
    nand_ctrl.imgFile = options.nand_storage_image
	
    nand_ctrl.index_num = index_num
    index_num = index_num + 1

    nand_ctrl.chn_0_index = chn_index
    chn_index = chn_index + 1
    nand_ctrl.chn_1_index = chn_index
    chn_index = chn_index + 1 

    if options.nand_type :
        nand_ctrl.media_type = options.nand_type
    if options.nand_num_chn :
        nand_ctrl.numChannel = options.nand_num_chn
    if options.nand_pkgs_per_chn :
        nand_ctrl.numPackages = options.nand_pkgs_per_chn
    if options.nand_dies_per_pkg:
        nand_ctrl.numDies = options.nand_dies_per_pkg 
    if options.nand_planes_per_die:
        nand_ctrl.numPlanes = options.nand_planes_per_die
    if options.nand_blocks_per_plane :
        nand_ctrl.numBlocks = options.nand_blocks_per_plane
    if options.nand_pages_per_block :
        nand_ctrl.numPages = options.nand_pages_per_block
    if options.nand_page_size :
        nand_ctrl.DS = options.nand_page_size 


    if options.nand_ltcy_rcmd_issue  :
        nand_ctrl.nand_ltcy_rcmd_issue  = options.nand_ltcy_rcmd_issue  
    if options.nand_ltcy_pcmd_issue  :
        nand_ctrl.nand_ltcy_pcmd_issue  = options.nand_ltcy_pcmd_issue 
    if options.nand_ltcy_ecmd_issue  :
        nand_ctrl.nand_ltcy_ecmd_issue  = options.nand_ltcy_ecmd_issue  
    if options.nand_ltcy_read_page  :
        nand_ctrl.nand_ltcy_read_page  = options.nand_ltcy_read_page  
    if options.nand_ltcy_program_page  :
        nand_ctrl.nand_ltcy_program_page  = options.nand_ltcy_program_page  
    if options.nand_ltcy_erase_block  :
        nand_ctrl.nand_ltcy_erase_block  = options.nand_ltcy_erase_block  


    if options.nand_ltcy_read_msb_page  :
        nand_ctrl.nand_ltcy_read_msb_page  = options.nand_ltcy_read_msb_page  
    if options.nand_ltcy_read_lsb_page  :
        nand_ctrl.nand_ltcy_read_lsb_page  = options.nand_ltcy_read_lsb_page  
    if options.nand_ltcy_program_msb_page  :
        nand_ctrl.nand_ltcy_program_msb_page  = options.nand_ltcy_program_msb_page  
    if options.nand_ltcy_program_lsb_page  :
        nand_ctrl.nand_ltcy_program_lsb_page  = options.nand_ltcy_program_lsb_page  
    if options.nand_ltcy_read_csb_page  :
        nand_ctrl.nand_ltcy_read_csb_page  = options.nand_ltcy_read_csb_page  
    if options.nand_ltcy_program_csb_page  :
        nand_ctrl.nand_ltcy_program_csb_page  = options.nand_ltcy_program_csb_page  
    if options.nand_ltcy_read_chsb_page  :
        nand_ctrl.nand_ltcy_read_chsb_page  = options.nand_ltcy_read_chsb_page  
    if options.nand_ltcy_read_clsb_page  :
        nand_ctrl.nand_ltcy_read_clsb_page  = options.nand_ltcy_read_clsb_page  
    if options.nand_ltcy_program_chsb_page  :
        nand_ctrl.nand_ltcy_program_chsb_page  = options.nand_ltcy_program_chsb_page  
    if options.nand_ltcy_program_clsb_page  :
        nand_ctrl.nand_ltcy_program_clsb_page  = options.nand_ltcy_program_clsb_page  





 


    NandCtrl.append(nand_ctrl)


test_sys.NandCtrl = NandCtrl

#0x2d109000
test_sys.stats = iSSDStats(gic = test_sys.realview.gic , pio_addr=0x2d419000 , pio_size=0x4000 ,int_num=0)
test_sys.stats.pio = test_sys.membus.master
test_sys.stats.dma = test_sys.membus.slave

# EOT character on UART will end the simulation
test_sys.realview.uart.end_on_eot = True


print "num of NC = %d" % nand_ctrl_count
#print "numChannel = %d" % test_sys.NandCtrl0.numChannel  
#print "numPackages = %d" % test_sys.NandCtrl0.numPackages  
#print "numDies = %d" % test_sys.NandCtrl0.numDies  
#print "numPlanes = %d" % test_sys.NandCtrl0.numPlanes  
#print "numBlocks = %d" % test_sys.NandCtrl0.numBlocks  
#print "numPages = %d" % test_sys.NandCtrl0.numPages  
#print "Page size  = " + str(test_sys.NandCtrl0.DS) + "(" + str((1 << int(test_sys.NandCtrl0.DS) ) ) + ")"
print 
 
#os.system("cp -f dummy.rcS Uploadfile")
#test_sys.readfile = os.getcwd() + '/Uploadfile'


Simulation.setWorkCountOptions(test_sys, options)
Simulation.run(options, root, test_sys, FutureClass)

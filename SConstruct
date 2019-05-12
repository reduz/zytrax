import os
EnsureSConsVersion(0,14);

env = Environment(CPPPATH=['#/globals','#gui','#.'],ENV=os.environ)



env.ParseConfig("pkg-config gtkmm-3.0 --libs --cflags")
#env.ParseConfig("pkg-config lilv-0 --libs --cflags")
#env.ParseConfig("pkg-config suil-0 --libs --cflags")
env.Append(CXXFLAGS=["-g3"])
env.Append(CXXFLAGS=["-DWINDOWS_ENABLED"])

opts = Variables(ARGUMENTS)

detected_platform = "windows"

opts.Add(EnumVariable("platform","Platform to build",detected_platform,("windows","osx","x11")))
opts.Add(BoolVariable("enable_rtaudio","Use RtAudio as Sound Driver",True))

opts.Update(env)  # update environment
Help(opts.GenerateHelpText(env))  # generate help

if (env["enable_rtaudio"]):

	env.Append(CXXFLAGS=["-DRTAUDIO_ENABLED"])
	if (env["platform"]=="windows"):
		env.Append(CXXFLAGS=["-D__WINDOWS_WASAPI__"])
		env.Append(CXXFLAGS=["-D__WINDOWS_DS__"])
		#env.Append(CXXFLAGS=["-D__WINDOWS_ASIO__"])
		env.Append(LIBS=["dsound","mfplat","mfuuid","wmcodecdspuuid","ksuser"])



def add_sources(self, sources, filetype, lib_env = None, shared = False):
	import glob;
	import string;
	#if not lib_objects:
	if not lib_env:
		lib_env = self
	if type(filetype) == type(""):

		dir = self.Dir('.').abspath
		list = glob.glob(dir + "/"+filetype)
		for f in list:
			sources.append( self.Object(f) )
	else:
		for f in filetype:
			sources.append(self.Object(f))
			
		
env.__class__.add_sources=add_sources


Export('env')

env.libs=[]

SConscript('globals/SCsub');
SConscript('dsp/SCsub');
SConscript('engine/SCsub');
SConscript('gui/SCsub');
SConscript('drivers/SCsub');
SConscript('bin/SCsub');



EnsureSConsVersion(0,14);

env = Environment(CPPPATH=['#/globals','#gui','#.'])

env.ParseConfig("pkg-config gtkmm-3.0 --libs --cflags")
env.ParseConfig("pkg-config lilv-0 --libs --cflags")
env.ParseConfig("pkg-config suil-0 --libs --cflags")
env.Append(CXXFLAGS=["-g3"])

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
SConscript('engine/SCsub');
SConscript('drivers/SCsub');
SConscript('gui/SCsub');
SConscript('bin/SCsub');


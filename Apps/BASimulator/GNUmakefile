
# See /vol/bob/make/bob_head.mk for detailed comments.

# stop Bob picking up old stuff from /vol/weta/include as there are antique versions of tclap
ignoreold=1

##############################################################################
include /vol/bob/make/head.mk
##############################################################################



###{ Name of executable to be built.
###}
PRG1_CF		:= -DWETA

#Nasty but we need it to build BASimulator which uses a file in the BASim library which references a file back in BASim...
PRG1_CF += -I../../../ -I. -I/vol/bob/src/code/Figaro/work/acoull-breannanCollisions/
PRG1_CF += -I/vol/bob/src/code/Figaro/work/acoull-breannanCollisions/Apps/BASimulator/tclap-1.2.0/include/

PRG1		:= basimulator


PRG1_H		:= \
BASimulator.hh \
Problems/ProblemBase.hh \
Problems/BentTwisting.hh \
Problems/CollisionTest.hh \
Problems/CollisionTestTwo.hh \
Problems/RegressionTests.hh \
Problems/HairyBall.hh \
Problems/SHO.hh \
Problems/SerializationTests.hh \
Problems/ViscosityTests.hh \
tclap-1.2.0/tclap/include/SwitchArg.h

#PRG1_HDIR	:=./Problems

###}
###{ Names of source files for this executable.
PRG1_S		:= \
		BASimulator.cc \
		./Problems/BentTwisting.cc \
		./Problems/CollisionTest.cc \
		./Problems/CollisionTestTwo.cc \
		./Problems/ProblemBase.cc \
		./Problems/RegressionTests.cc \
		./Problems/HairyBall.cc \
		./Problems/SHO.cc \
		./Problems/SerializationTests.cc \
		./Problems/ViscosityTests.cc 

###}
###{ Linker specifications for libraries to use when
###  linking this executable.
PRG1_L		:= \
	-lWfigaro_dbg \
    -lglut

# add in the MKL libraries
PRG1_L        += -Wl,--start-group \
                  $(MKLROOT)/lib/em64t/libmkl_intel_lp64.a \
                  $(MKLROOT)/lib/em64t/libmkl_intel_thread.a \
                  $(MKLROOT)/lib/em64t/libmkl_core.a \
                 -Wl,--end-group -openmp -lpthread

#
##############################################################################
include /vol/bob/make/tail.mk
##############################################################################

# Stuff for Emacs:
# Local Variables:
# mode:makefile-gmake
# indent-tabs-mode:t
# tab-width:8
# End:

# Stuff for Vim [by default, this needs to be within the last 5 lines]:
# ex:set filetype=make tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab:
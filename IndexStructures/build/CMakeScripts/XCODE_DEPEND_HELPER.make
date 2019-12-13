# DO NOT EDIT
# This makefile makes sure all linkable targets are
# up-to-date with anything they link to
default:
	echo "Do not invoke directly"

# Rules to remove targets that are older than anything to which they
# link.  This forces Xcode to relink the targets from scratch.  It
# does not seem to check these dependencies itself.
PostBuild.IndexStructure.Debug:
/Users/max/CS/Courses/197/IndexStructures/build/Debug/IndexStructure:
	/bin/rm -f /Users/max/CS/Courses/197/IndexStructures/build/Debug/IndexStructure


PostBuild.IndexStructure.Release:
/Users/max/CS/Courses/197/IndexStructures/build/Release/IndexStructure:
	/bin/rm -f /Users/max/CS/Courses/197/IndexStructures/build/Release/IndexStructure


PostBuild.IndexStructure.MinSizeRel:
/Users/max/CS/Courses/197/IndexStructures/build/MinSizeRel/IndexStructure:
	/bin/rm -f /Users/max/CS/Courses/197/IndexStructures/build/MinSizeRel/IndexStructure


PostBuild.IndexStructure.RelWithDebInfo:
/Users/max/CS/Courses/197/IndexStructures/build/RelWithDebInfo/IndexStructure:
	/bin/rm -f /Users/max/CS/Courses/197/IndexStructures/build/RelWithDebInfo/IndexStructure




# For each target create a dummy ruleso the target does not have to exist

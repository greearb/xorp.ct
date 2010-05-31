#
# Monkey patch for os.path to include relpath if python version is < 2.6.
#
# Obtained from: http://code.activestate.com/recipes/302594/
#

import os

if not hasattr(os.path, "relpath"):
    def relpath(target, base=os.curdir):
        base_list = (os.path.abspath(base)).split(os.sep)
        target_list = (os.path.abspath(target)).split(os.sep)
        for i in range(min(len(base_list), len(target_list))):
            if base_list[i] <> target_list[i]: break
        else:
            i += 1
        rel_list = [os.pardir] * (len(base_list)-i) + target_list[i:]
        return os.path.join(*rel_list)
    os.path.relpath = relpath

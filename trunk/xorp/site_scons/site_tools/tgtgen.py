import SCons.Action
import SCons.Builder
import SCons.Scanner

tgtgen_action = SCons.Action.Action("$TGTGEN $_TGTGEN_INCFLAGS --output-dir ${TARGET.dir} ${SOURCE}")

def tgtgen_emitter(target, source, env):
    base,ext = SCons.Util.splitext(str(source[0]))
    base = base[base.rfind("/") + 1:] 
    cc = base + "_base.cc"
    hh = base + "_base.hh"
    xrls = base + ".xrls"
    t = [cc, hh, xrls]
    return (t, source)

tgtgen_scanner = SCons.Scanner.ClassicCPP("TGTGENScan",
                                  [ ".tgt",  "*.xif" ],
                                  "TGTGEN_CPPPATH",
                                  '^[ \t]*#[ \t]*(?:include|import)[ \t]*(<|")([^>"]+)(>|")')

tgtgen_builder = SCons.Builder.Builder(
    action = tgtgen_action,
    src_suffix = ".tgt",
    emitter = tgtgen_emitter,
    source_scanner =  tgtgen_scanner)

def generate(env):
    """Add Builder for XRL targets to the Environment ..."""

    env['BUILDERS']['TGTGEN'] = tgtgen_builder

    env['TGTGEN'] = "$xorp_sourcedir/xrl/scripts/tgt-gen"
    env['TGTGEN_CPPPATH'] = []
    env['_TGTGEN_INCFLAGS'] = '$( ${_concat(INCPREFIX, TGTGEN_CPPPATH, INCSUFFIX, __env__, RDirs, TARGET, SOURCE)} $)'

def exists(env):
    return True

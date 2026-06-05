import subprocess
from SCons.Script import Import

# This tells Python to pull the 'env' object that PlatformIO injected
Import("env")

def after_build(source, target, env):
    print("\n>>> [Hook] Build finished successfully. Regenerating compile_commands.json...")
    # Calls pio run -t compiledb for the current environment
    subprocess.run(["pio", "run", "-t", "compiledb", "-e", env["PIOENV"]], shell=True)

# Register the post-action callback to trigger after the program binary is built
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", after_build)
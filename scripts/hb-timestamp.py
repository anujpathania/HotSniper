import sim


def hook_magic_timestamp(core, a):
    """Get femtosecond precision time since program startup"""
    time = sim.stats.time() # gets it from subsecond_time.h

    return time

sim.util.register_command(0x123, hook_magic_timestamp)

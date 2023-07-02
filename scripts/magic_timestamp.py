# Expose hook for accessing simulation time.

import sim


def hook_timestamp(core, a):
    return sim.stats.time()


sim.util.register_command(0x123, hook_timestamp)

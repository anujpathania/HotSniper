# Expose hook for accessing simulation time.

import sim


def hook_timestamp(core, a):
    print("[MAGIC] recieved heartbeat at {time}".format(time=sim.stats.time()))

    return sim.stats.time()


sim.util.register_command(0x123, hook_timestamp)

import sim

def hook_perforation_rate(core, a):
    pr = sim.stats.get("scheduler", 0, "perforation_rate")

    print("[MAGIC] recieved perforation rate: {perforation_rate}".format(perforation_rate=pr))

    return pr

sim.util.register_command(0x125, hook_perforation_rate)

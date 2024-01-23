import sim

def hook_perforation_rate(core, a):
    return sim.stats.get("scheduler", 0, "perforation_rate")
    
sim.util.register_command(0x125, hook_perforation_rate)
